/*
 * Copyright (c) 2025, Jack Grogan
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <esp32/rom/ets_sys.h>
#include "zephyr/types.h"
#include "tools/util.h"
#include "adapter/config.h"
#include "adapter/wired/wired.h"
#include "generic.h"

#define GEN_JOY_MAX_AXES  4
#define GEN_TRIG_MAX_AXES 2

enum {
    GEN_D_UP = 0,
    GEN_D_DOWN,
    GEN_D_LEFT,
    GEN_D_RIGHT,
};

enum {
    GEN_A = 0,
    GEN_B,
    GEN_X,
    GEN_Y,
    GEN_L3,
    GEN_R3,
    GEN_BACK,
    GEN_START,
    GEN_LB,
    GEN_RB,
    GEN_SYS,
    GEN_MISC,
};

static DRAM_ATTR const struct ctrl_meta gen_axes_meta[ADAPTER_MAX_AXES] =
{
    {.size_min = -32768, .size_max = 32767, .neutral = 0, .abs_max = 0x7FFF, .abs_min = 0x8000},
    {.size_min = -32768, .size_max = 32767, .neutral = 0, .abs_max = 0x7FFF, .abs_min = 0x8000},
    {.size_min = -32768, .size_max = 32767, .neutral = 0, .abs_max = 0x7FFF, .abs_min = 0x8000},
    {.size_min = -32768, .size_max = 32767, .neutral = 0, .abs_max = 0x7FFF, .abs_min = 0x8000},
    {.size_min = 0, .size_max = 255, .neutral = 0, .abs_max = 255, .abs_min = 0},
    {.size_min = 0, .size_max = 255, .neutral = 0, .abs_max = 255, .abs_min = 0},
};

struct gen_gp_map {
    uint8_t  dpad;
    uint16_t buttons;
    uint8_t  trigger_axes[GEN_TRIG_MAX_AXES]; //l, r
    int16_t  joy_axes[GEN_JOY_MAX_AXES]; //lx, ly, rx, ry
} __packed;

static DRAM_ATTR const uint8_t gen_joy_axes_idx[GEN_JOY_MAX_AXES] = {
/*  AXIS_LX, AXIS_LY, AXIS_RX, AXIS_RY */
    0,       1,       2,       3,
};

static DRAM_ATTR const uint8_t gen_trig_axes_idx[GEN_TRIG_MAX_AXES] = {
/*  AXIS_L, AXIS_R  */
    0,       1,
};

static const uint32_t gen_mask[4] = {0xBBFF0FFF, 0x00000000, 0x00000000, BR_COMBO_MASK};
static const uint32_t gen_desc[4] = {0x110000FF, 0x00000000, 0x00000000, 0x00000000};
static DRAM_ATTR const uint32_t gen_btns_mask[32] = {
    0,                  0,                  0,                  0,
    0,                  0,                  0,                  0,
    BIT(GEN_D_LEFT),    BIT(GEN_D_RIGHT),   BIT(GEN_D_DOWN),    BIT(GEN_D_UP),
    0,                  0,                  0,                  0,
    BIT(GEN_X),         BIT(GEN_B),         BIT(GEN_A),         BIT(GEN_Y),
    BIT(GEN_START),     BIT(GEN_BACK),      BIT(GEN_SYS),       BIT(GEN_MISC),
    0,                  BIT(GEN_LB),        0,                  BIT(GEN_L3),
    0,                  BIT(GEN_RB),        0,                  BIT(GEN_R3),
};

void IRAM_ATTR gen_init_buffer(int32_t dev_mode, struct wired_data *wired_data) {
    switch (dev_mode) {
        default:
        {
            struct gen_gp_map *map = (struct gen_gp_map *)wired_data->output;

            memset(wired_data->output, 0, 32);
            map->dpad = 0;
            map->buttons = 0;
            for (uint32_t i = 0; i < GEN_JOY_MAX_AXES; i++) {
                map->joy_axes[gen_joy_axes_idx[i]] = gen_axes_meta[i].neutral;
            }
            for (uint32_t i = 0; i < GEN_TRIG_MAX_AXES; i++) {
                map->trigger_axes[gen_trig_axes_idx[i]] = gen_axes_meta[GEN_JOY_MAX_AXES + i].neutral;
            }
            memset(wired_data->output_mask, 0x00, sizeof(struct gen_gp_map));
            break;
        }
    }
}

void gen_meta_init(struct wired_ctrl *ctrl_data) {
    memset((void *)ctrl_data, 0, sizeof(*ctrl_data)*4);

    for (uint32_t i = 0; i < WIRED_MAX_DEV; i++) {
        for (uint32_t j = 0; j < ADAPTER_MAX_AXES; j++) {
            switch (config.out_cfg[i].dev_mode) {
                // case DEV_PAD_ALT:
                //     ctrl_data[i].mask = gen_mask;
                //     ctrl_data[i].desc = gen_desc;
                //     ctrl_data[i].axes[j].meta = &gen_axes_meta[j];
                //     break;
                default:
                    ctrl_data[i].mask = gen_mask;
                    ctrl_data[i].desc = gen_desc;
                    ctrl_data[i].axes[j].meta = &gen_axes_meta[j];
                    break;
            }
        }
    }
}

static inline int16_t gen_invert_axis(int16_t axis) {
    if (axis == INT16_MIN) {
        return INT16_MAX;
    }
    return -axis;
}

void gen_from_generic(int32_t dev_mode, struct wired_ctrl *ctrl_data, struct wired_data *wired_data) {
    if (!ctrl_data || !wired_data) {
        return;
    }
    struct gen_gp_map map_tmp;
    memcpy((void *)&map_tmp, wired_data->output, sizeof(map_tmp));

    // Map dpad
    for (uint32_t i = 8; i < 12; i++) {
        if ((ctrl_data->map_mask[0] & BIT(i)) && gen_btns_mask[i]) {
            if (ctrl_data->btns[0].value & generic_btns_mask[i]) {
                map_tmp.dpad |= (uint8_t)gen_btns_mask[i];
                wired_data->cnt_mask[i] = ctrl_data->btns[0].cnt_mask[i];
            } else {
                map_tmp.dpad &= ~(uint8_t)gen_btns_mask[i];
                wired_data->cnt_mask[i] = 0;
            }
        }
    }

    // Map buttons
    for (uint32_t i = 16; i < ARRAY_SIZE(generic_btns_mask); i++) {
        if ((ctrl_data->map_mask[0] & BIT(i)) && gen_btns_mask[i]) {
            if (ctrl_data->btns[0].value & generic_btns_mask[i]) {
                map_tmp.buttons |= (uint16_t)gen_btns_mask[i];
                wired_data->cnt_mask[i] = ctrl_data->btns[0].cnt_mask[i];
            } else {
                map_tmp.buttons &= ~(uint16_t)gen_btns_mask[i];
                wired_data->cnt_mask[i] = 0;
            }
        }
    }

    // struct raw_fb fb_data = {0};
    // fb_data.header.wired_id = 0;
    // fb_data.header.type = FB_TYPE_RUMBLE;
    // fb_data.header.data_len = 2;
    // if (map_tmp.buttons & BIT(GEN_A)) {
    //     fb_data.data[0] = 0xFF;
    //     fb_data.data[1] = 0xFF;
    // } else {
    //     fb_data.data[0] = 0x00;
    //     fb_data.data[1] = 0x00;
    // }
    // adapter_q_fb(&fb_data);

    // Map joysticks
    for (uint32_t i = 0; i < GEN_JOY_MAX_AXES; i++) {
        if (ctrl_data->map_mask[0] & (axis_to_btn_mask(i) & ctrl_data->desc[0])) {
            if (ctrl_data->axes[i].value > ctrl_data->axes[i].meta->size_max) {
                map_tmp.joy_axes[gen_joy_axes_idx[i]] = gen_axes_meta[i].size_max;
            } else if (ctrl_data->axes[i].value < ctrl_data->axes[i].meta->size_min) {
                map_tmp.joy_axes[gen_joy_axes_idx[i]] = gen_axes_meta[i].size_min;
            } else {
                map_tmp.joy_axes[gen_joy_axes_idx[i]] = ctrl_data->axes[i].value;
            }
            if (gen_joy_axes_idx[i] == 1 || gen_joy_axes_idx[i] == 3) {
                map_tmp.joy_axes[gen_joy_axes_idx[i]] = gen_invert_axis(map_tmp.joy_axes[gen_joy_axes_idx[i]]);
            }
        }
        wired_data->cnt_mask[axis_to_btn_id(i)] = ctrl_data->axes[i].cnt_mask;
    }

    // Map triggers
    for (uint32_t i = 0; i < GEN_TRIG_MAX_AXES; i++) {
        if (ctrl_data->map_mask[0] & (axis_to_btn_mask(i + GEN_JOY_MAX_AXES) & ctrl_data->desc[0])) {
            if (ctrl_data->axes[i + GEN_JOY_MAX_AXES].value > ctrl_data->axes[i + GEN_JOY_MAX_AXES].meta->size_max) {
                map_tmp.trigger_axes[gen_trig_axes_idx[i]] = gen_axes_meta[i + GEN_JOY_MAX_AXES].size_max;
            } else if (ctrl_data->axes[i + GEN_JOY_MAX_AXES].value < ctrl_data->axes[i + GEN_JOY_MAX_AXES].meta->size_min) {
                map_tmp.trigger_axes[gen_trig_axes_idx[i]] = gen_axes_meta[i + GEN_JOY_MAX_AXES].size_min;
            } else {
                map_tmp.trigger_axes[gen_trig_axes_idx[i]] = ctrl_data->axes[i + GEN_JOY_MAX_AXES].value;
            }
        }
        wired_data->cnt_mask[axis_to_btn_id(i + GEN_JOY_MAX_AXES)] = ctrl_data->axes[i + GEN_JOY_MAX_AXES].cnt_mask;
    }

    memcpy(wired_data->output, (void *)&map_tmp, sizeof(map_tmp));

#ifdef CONFIG_BLUERETRO_RAW_OUTPUT
    printf("{\"log_type\": \"wired_output\", \"axes\": [%d, %d, %d, %d], \"dpad\": %d, \"btns\": %d, \"triggers\": [%d, %d]}\n",
        map_tmp.joy_axes[gen_joy_axes_idx[0]], map_tmp.joy_axes[gen_joy_axes_idx[1]], map_tmp.joy_axes[gen_joy_axes_idx[2]],
        map_tmp.joy_axes[gen_joy_axes_idx[3]], map_tmp.dpad, map_tmp.buttons, 
        map_tmp.trigger_axes[gen_trig_axes_idx[0]], map_tmp.trigger_axes[gen_trig_axes_idx[1]]);
#endif
}

void gen_fb_to_generic(int32_t dev_mode, struct raw_fb *raw_fb_data, struct generic_fb *fb_data) {
    fb_data->wired_id = raw_fb_data->header.wired_id;
    fb_data->type = raw_fb_data->header.type;

    switch (fb_data->type) {
        case FB_TYPE_RUMBLE:
            fb_data->state = ((raw_fb_data->data[0] || raw_fb_data->data[1]) ? 1 : 0);
            fb_data->lf_pwr = raw_fb_data->data[0];
            fb_data->hf_pwr = raw_fb_data->data[1];
            break;
        case FB_TYPE_STATUS_LED:
            // fb_data->led = raw_fb_data->data[0];
            break;
    }
}

void IRAM_ATTR gen_gen_turbo_mask(struct wired_data *wired_data) {
    struct gen_gp_map *map_mask = (struct gen_gp_map *)wired_data->output_mask;

    map_mask->buttons = 0x0000;
    map_mask->dpad = 0x00;
    memset(map_mask->trigger_axes, 0xFF, sizeof(map_mask->trigger_axes));
    memset(map_mask->joy_axes, 0x00, sizeof(map_mask->joy_axes));

    // for (uint32_t i = 0; i < ARRAY_SIZE(gen_btns_mask); i++) {
    //     uint8_t mask = wired_data->cnt_mask[i] >> 1;

    //     if (gen_btns_mask[i] && mask) {
    //         if (wired_data->cnt_mask[i] & 1) {
    //             if (!(mask & wired_data->frame_cnt)) {
    //                 if (i < 4) {
    //                     map_mask->dpad |= gen_btns_mask[i];
    //                 } else {
    //                     map_mask->buttons |= gen_btns_mask[i];
    //                 }
    //             }
    //         } else {
    //             if (!((mask & wired_data->frame_cnt) == mask)) {
    //                 if (i < 4) {
    //                     map_mask->dpad |= gen_btns_mask[i];
    //                 } else {
    //                     map_mask->buttons |= gen_btns_mask[i];
    //                 }
    //             }
    //         }
    //     }
    // }

    // wired_gen_turbo_mask_axes8(wired_data, map_mask->joy_axes, GEN_JOY_MAX_AXES, gen_joy_axes_idx, gen_axes_meta);
}
