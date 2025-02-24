/*
 * Copyright (c) 2025, Jack Grogan
 * SPDX-License-Identifier: Apache-2.0
 */

#include "generic_i2c.h"
#include "sdkconfig.h"
#if defined (CONFIG_BLUERETRO_SYSTEM_GENERIC_I2C)
#include <string.h>
#include "soc/io_mux_reg.h"
#include "esp_private/periph_ctrl.h"
#include "esp_intr_alloc.h"
#include <soc/i2c_periph.h>
#include <esp32/rom/ets_sys.h>
#include <esp32/rom/gpio.h>
#include "hal/i2c_ll.h"
#include "hal/clk_gate_ll.h"
#include "hal/misc.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "system/intr.h"
#include "system/gpio.h"
#include "system/delay.h"
#include "zephyr/atomic.h"
#include "zephyr/types.h"
#include "tools/util.h"
#include "tools/devcrypto.h"
#include "adapter/adapter.h"
#include "adapter/config.h"
#include "adapter/wired/generic.h"

#define I2C0_INTR_NUM 19
#define I2C1_INTR_NUM 20

#define I2C0_INTR_MASK (1 << I2C0_INTR_NUM)
#define I2C1_INTR_MASK (1 << I2C1_INTR_NUM)

#define P1_SCL_PIN 22
#define P1_SDA_PIN 21

#define P1_SCL_MASK (1 << P1_SCL_PIN)
#define P1_SDA_MASK (1 << P1_SDA_PIN)

#define GEN_I2C_PORT_MAX 1

#define I2C_ADDR_BASE 0x50

#define I2C_SLAVE_BUF_LEN   16

struct gen_i2c_in_packet {
    uint8_t len;
    uint8_t index;
    uint8_t driver;
    uint8_t data[13];
} __packed;

struct gen_i2c_out_packet {
    uint8_t len;
    uint8_t data[2];
    uint8_t reserved[5];
} __packed;

struct gen_ctrl_port {
    i2c_dev_t *hw;
    uint32_t id;
    i2c_port_t port;
    uint32_t scl_pin;
    uint32_t sda_pin;
    uint32_t scl_mask;
    uint32_t sda_mask;
    uint32_t fifo_addr;
    uint8_t sda_out_sig;
    uint8_t sda_in_sig;
    uint8_t scl_out_sig;
    uint8_t scl_in_sig;
    uint8_t module;
    uint8_t *reg;
    uint8_t tmp[32];
};

static struct gen_ctrl_port gen_ctrl_ports[GEN_I2C_PORT_MAX] = {
    {
        .hw             = &I2C0,
        .id             = 0,
        .port           = I2C_NUM_0,
        .scl_pin        = P1_SCL_PIN,
        .sda_pin        = P1_SDA_PIN,
        .scl_mask       = P1_SCL_MASK,
        .sda_mask       = P1_SDA_MASK,
        .fifo_addr      = 0x6001301c,
        .sda_out_sig    = I2CEXT0_SDA_OUT_IDX,
        .sda_in_sig     = I2CEXT0_SDA_IN_IDX,
        .scl_out_sig    = I2CEXT0_SCL_OUT_IDX,
        .scl_in_sig     = I2CEXT0_SCL_IN_IDX,
        .module         = PERIPH_I2C0_MODULE,
    },
#if GEN_I2C_PORT_MAX > 1
    // {
    //     .hw = &I2C1,
    //     .id = 1,
    //     .scl_pin = P2_SCL_PIN,
    //     .sda_pin = P2_SDA_PIN,
    //     .scl_mask = P2_SCL_MASK,
    //     .sda_mask = P2_SDA_MASK,
    //     .fifo_addr = 0x6002701c,
    //     .sda_out_sig = I2CEXT1_SDA_OUT_IDX,
    //     .sda_in_sig = I2CEXT1_SDA_IN_IDX,
    //     .scl_out_sig = I2CEXT1_SCL_OUT_IDX,
    //     .scl_in_sig = I2CEXT1_SCL_IN_IDX,
    //     .module = PERIPH_I2C1_MODULE,
    // },
#endif /* GEN_I2C_PORT_MAX > 1 */
};

static inline void gen_i2c_rx_cb(struct gen_ctrl_port* p, const uint8_t *buffer, uint32_t len) {
    if (len < sizeof(struct gen_i2c_out_packet)) {
        return;
    }
    const struct gen_i2c_out_packet* packet = (const struct gen_i2c_out_packet*)buffer;
    if (packet->len != sizeof(struct gen_i2c_out_packet)) {
        return;
    }
    struct raw_fb fb_data = {0};
    fb_data.header.wired_id = p->id;
    fb_data.header.type = FB_TYPE_RUMBLE;
    fb_data.header.data_len = sizeof(packet->data);
    memcpy(fb_data.data, packet->data, sizeof(packet->data));
    adapter_q_fb(&fb_data);
}

static inline uint8_t gen_i2c_tx_cb(struct gen_ctrl_port* p, uint8_t *buffer, uint8_t available_len) {
    if (available_len < sizeof(struct gen_i2c_in_packet)) {
        return 0;
    }
    struct gen_i2c_in_packet* packet = (struct gen_i2c_in_packet*)buffer;
    packet->len = sizeof(struct gen_i2c_in_packet);
    packet->driver = 0x01;
    packet->index = 0x00;
    memcpy(packet->data, wired_adapter.data[p->id].output, sizeof(packet->data));
    return sizeof(struct gen_i2c_in_packet);
}

static inline void i2c_irq_handler(void *arg) {
    struct gen_ctrl_port *p = (struct gen_ctrl_port *)arg;
    uint32_t intr_status = p->hw->int_status.val;

    if (intr_status & I2C_TRANS_COMPLETE_INT_ENA) {
        // --- RX Handling ---
        if (intr_status & I2C_RXFIFO_FULL_INT_ST) {
            uint32_t rx_count = p->hw->status_reg.rx_fifo_cnt;
            if (rx_count > 0) {
                uint8_t rx_data[I2C_SLAVE_BUF_LEN] = {0};
                uint8_t bytes_to_read = rx_count < I2C_SLAVE_BUF_LEN ? rx_count : I2C_SLAVE_BUF_LEN;
                i2c_ll_read_rxfifo(p->hw, rx_data, bytes_to_read);
                gen_i2c_rx_cb(p, rx_data, bytes_to_read);
            }
            p->hw->int_clr.val = I2C_RXFIFO_FULL_INT_CLR;
        }

        // --- TX Handling ---
        if (intr_status & I2C_TXFIFO_EMPTY_INT_ST) {
            uint8_t tx_fill = p->hw->status_reg.tx_fifo_cnt; // bytes already in FIFO
            uint8_t tx_avail = SOC_I2C_FIFO_LEN - tx_fill;
            if (tx_avail > 0) {
                uint8_t tx_data[I2C_SLAVE_BUF_LEN] = {0};
                uint8_t write_len = gen_i2c_tx_cb(p, tx_data, tx_avail);
                i2c_ll_write_txfifo(p->hw, tx_data, write_len);
            }
            p->hw->int_clr.val = I2C_TXFIFO_EMPTY_INT_CLR;
        }
    }
}

static unsigned isr_dispatch(unsigned cause) { 
    if (cause & I2C0_INTR_MASK) {
        i2c_irq_handler((void *)&gen_ctrl_ports[0]);
    }
    return 0;
}

static uint8_t i2c_clk_alloc[] = {
#if SOC_I2C_SUPPORT_APB
    0,                                                                /*!< I2C APB clock characteristic*/
#endif
#if SOC_I2C_SUPPORT_XTAL
    0,                                                               /*!< I2C XTAL characteristic*/
#endif
#if SOC_I2C_SUPPORT_RTC
    I2C_SCLK_SRC_FLAG_LIGHT_SLEEP | I2C_SCLK_SRC_FLAG_AWARE_DFS,      /*!< I2C 20M RTC characteristic*/
#endif
#if SOC_I2C_SUPPORT_REF_TICK
    I2C_SCLK_SRC_FLAG_AWARE_DFS,                                 /*!< I2C REF_TICK characteristic*/
#endif
};

static i2c_clock_source_t get_clk_src(const uint32_t clk_flags, const uint32_t clk_speed) {
    i2c_clock_source_t clk_srcs[] = SOC_I2C_CLKS;
    for (size_t i = 0; i < sizeof(clk_srcs) / sizeof(clk_srcs[0]); i++) {
        if (((clk_flags & i2c_clk_alloc[i]) == clk_flags) && (clk_speed <= (APB_CLK_FREQ / 20))) { 
            // I2C SCL clock frequency should not larger than clock source frequency/20
            return clk_srcs[i];
        }
    }
    return 0;     // flag invalid;
}
#endif /* defined(CONFIG_BLUERETRO_SYSTEM_GENERIC_I2C) */

void gen_i2c_init(uint32_t package) {
#if defined (CONFIG_BLUERETRO_SYSTEM_GENERIC_I2C)
    ets_printf("Initializing generic I2C\n");
    for (i2c_port_t i = 0; i < GEN_I2C_PORT_MAX; i++) {
        struct gen_ctrl_port *p = &gen_ctrl_ports[i];

        /* Data */
        gpio_set_level_iram(p->sda_pin, 1);
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG_IRAM[p->sda_pin], PIN_FUNC_GPIO);
        gpio_set_direction_iram(p->sda_pin, GPIO_MODE_INPUT_OUTPUT_OD);
        gpio_set_pull_mode_iram(p->sda_pin, GPIO_PULLUP_ONLY);
        gpio_matrix_out(p->sda_pin, p->sda_out_sig, false, false);
        gpio_matrix_in(p->sda_pin, p->sda_in_sig, false);

        /* Clock */
        gpio_set_level_iram(p->scl_pin, 1);
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG_IRAM[p->scl_pin], PIN_FUNC_GPIO);
        gpio_set_direction_iram(p->scl_pin, GPIO_MODE_INPUT_OUTPUT_OD);
        gpio_matrix_out(p->scl_pin, p->scl_out_sig, false, false);
        gpio_matrix_in(p->scl_pin, p->scl_in_sig, false);
        gpio_set_pull_mode_iram(p->scl_pin, GPIO_PULLUP_ONLY);

        periph_ll_enable_clk_clear_rst(p->module);

        i2c_clock_source_t clk_source = get_clk_src(0, 400 * 1000);
        if (clk_source == 0) {
            ets_printf("Invalid clock source\n");
            return;
        }

        i2c_ll_clear_intr_mask(p->hw, I2C_LL_INTR_MASK);
        i2c_ll_slave_init(p->hw);
        i2c_ll_slave_enable_auto_start(p->hw, true);
        i2c_ll_set_source_clk(p->hw, clk_source);
        i2c_ll_set_slave_addr(p->hw, I2C_ADDR_BASE + i, false);
        i2c_ll_set_rxfifo_full_thr(p->hw, 8);
        i2c_ll_set_txfifo_empty_thr(p->hw, 5);
        i2c_ll_set_sda_timing(p->hw, 10, 10);
        i2c_ll_set_tout(p->hw, 32000);
        i2c_ll_enable_intr_mask(p->hw,  I2C_TRANS_COMPLETE_INT_ENA | 
                                        I2C_RXFIFO_FULL_INT_ENA | 
                                        I2C_TXFIFO_EMPTY_INT_ENA);
        i2c_ll_slave_enable_rx_it(p->hw);
        i2c_ll_update(p->hw);

        intexc_alloc_iram(ETS_I2C_EXT0_INTR_SOURCE, I2C0_INTR_NUM, isr_dispatch);
    }
#endif /* defined(CONFIG_BLUERETRO_SYSTEM_GENERIC_I2C) */
}

void gen_i2c_port_cfg(uint16_t mask) {
#if defined (CONFIG_BLUERETRO_SYSTEM_GENERIC_I2C)
    ets_printf("Configuring generic I2C ports\n");
    for (uint32_t i = 0; i < GEN_I2C_PORT_MAX; i++) {
        struct gen_ctrl_port *p = &gen_ctrl_ports[i];
        i2c_ll_set_slave_addr(p->hw, I2C_ADDR_BASE + i, false);
        mask >>= 1;
    }
#endif /* defined(CONFIG_BLUERETRO_SYSTEM_GENERIC_I2C) */
}