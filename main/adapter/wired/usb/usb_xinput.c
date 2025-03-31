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
#include "xinput.h"
#include "wired/usb/xinput_usb.h"

#define XINPUT_JOY_MAX_AXES  4
#define XINPUT_TRIG_MAX_AXES 2

enum {
    XINPUT_BTN0_D_UP = 0,
    XINPUT_BTN0_D_DOWN,
    XINPUT_BTN0_D_LEFT,
    XINPUT_BTN0_D_RIGHT,
    XINPUT_BTN0_START,
    XINPUT_BTN0_BACK,
    XINPUT_BTN0_L3,
    XINPUT_BTN0_R3,
};

enum {
    XINPUT_BTN1_LB = 0,
    XINPUT_BTN1_RB,
    XINPUT_BTN1_HOME,
    XINPUT_BTN1_A,
    XINPUT_BTN1_B,
    XINPUT_BTN1_X,
    XINPUT_BTN1_Y,
};

static DRAM_ATTR const struct ctrl_meta xinput_axes_meta[ADAPTER_MAX_AXES] = {
    {.size_min = -32768, .size_max = 32767, .neutral = 0, .abs_max = 0x7FFF, .abs_min = 0x8000},
    {.size_min = -32768, .size_max = 32767, .neutral = 0, .abs_max = 0x7FFF, .abs_min = 0x8000},
    {.size_min = -32768, .size_max = 32767, .neutral = 0, .abs_max = 0x7FFF, .abs_min = 0x8000},
    {.size_min = -32768, .size_max = 32767, .neutral = 0, .abs_max = 0x7FFF, .abs_min = 0x8000},
    {.size_min = 0, .size_max = 255, .neutral = 0, .abs_max = 255, .abs_min = 0},
    {.size_min = 0, .size_max = 255, .neutral = 0, .abs_max = 255, .abs_min = 0},
};

static DRAM_ATTR const uint8_t xinput_joy_axes_idx[XINPUT_JOY_MAX_AXES] = {
/*  AXIS_LX, AXIS_LY, AXIS_RX, AXIS_RY */
    0,       1,       2,       3,
};

static DRAM_ATTR const uint8_t xinput_trig_axes_idx[XINPUT_TRIG_MAX_AXES] = {
/*  AXIS_L, AXIS_R  */
    0,       1,
};

static const uint32_t xinput_mask[4] = {0xBBFF0FFF, 0x00000000, 0x00000000, BR_COMBO_MASK};
static const uint32_t xinput_desc[4] = {0x110000FF, 0x00000000, 0x00000000, 0x00000000};

static DRAM_ATTR const uint32_t xinput_btns0_mask[32] = {
    0,                          0,                          0,                          0,
    0,                          0,                          0,                          0,
    BIT(XINPUT_BTN0_D_LEFT),    BIT(XINPUT_BTN0_D_RIGHT),   BIT(XINPUT_BTN0_D_DOWN),    0,
    0,                          0,                          0,                          0,
    0,                          0,                          0,                          0,
    BIT(XINPUT_BTN0_START),     BIT(XINPUT_BTN0_BACK),      0,                          0,
    0,                          0,                          0,                          BIT(XINPUT_BTN0_L3),
    0,                          0,                          0,                          BIT(XINPUT_BTN0_R3),
};

static DRAM_ATTR const uint32_t xinput_btns1_mask[32] = {
    0,                  0,                      0,                      0,
    0,                  0,                      0,                      0,
    0,                  0,                      0,                      0,
    0,                  0,                      0,                      0,
    BIT(XINPUT_BTN1_X), BIT(XINPUT_BTN1_B),     BIT(XINPUT_BTN1_A),     BIT(XINPUT_BTN1_Y),
    0,                  0,                      BIT(XINPUT_BTN1_HOME),  0,
    0,                  BIT(XINPUT_BTN1_LB),    0,                      0,
    0,                  BIT(XINPUT_BTN1_RB),    0,                      0,
};

struct xinput_report_in {
    uint8_t report_id;
    uint8_t len;
    uint8_t buttons[2];
    uint8_t trigger_axes[XINPUT_TRIG_MAX_AXES];
    int16_t joy_axes[XINPUT_JOY_MAX_AXES];
    uint8_t reserved[6];
} __packed;

struct xinput_report_out {
    uint8_t report_id;
    uint8_t len;
    uint8_t led;
    uint8_t rumble_l;
    uint8_t rumble_r;
    uint8_t reserved[3];
} __packed;

void IRAM_ATTR usb_xinput_init_buffer(int32_t dev_mode, struct wired_data *wired_data) {
    struct xinput_report_in *report_in = (struct xinput_report_in *)wired_data->output;

    memset(wired_data->output, 0, sizeof(struct xinput_report_in));
    report_in->report_id = 0x00;
    report_in->len = sizeof(struct xinput_report_in);
    report_in->dpad = 0;
    report_in->buttons = 0;
    for (uint32_t i = 0; i < XINPUT_JOY_MAX_AXES; i++) {
        report_in->joy_axes[gen_joy_axes_idx[i]] = gen_axes_meta[i].neutral;
    }
    for (uint32_t i = 0; i < XINPUT_TRIG_MAX_AXES; i++) {
        report_in->trigger_axes[gen_trig_axes_idx[i]] = gen_axes_meta[XINPUT_JOY_MAX_AXES + i].neutral;
    }
    memset(wired_data->output_mask, 0x00, sizeof(struct gen_gp_map));
}

void usb_xinput_meta_init(struct wired_ctrl *ctrl_data) {
    memset((void *)ctrl_data, 0, sizeof(*ctrl_data)*4);
    for (uint32_t i = 0; i < WIRED_MAX_DEV; i++) {
        for (uint32_t j = 0; j < ADAPTER_MAX_AXES; j++) {
            ctrl_data[i].mask = xinput_mask;
            ctrl_data[i].desc = xinput_desc;
            ctrl_data[i].axes[j].meta = &xinput_axes_meta[j];
        }
    }
}

void usb_xinput_from_generic(int32_t dev_mode, struct wired_ctrl *ctrl_data, struct wired_data *wired_data) {
    if (!ctrl_data || !wired_data) {
        return;
    }
    struct xinput_report_in map_tmp;
    memcpy(&map_tmp, wired_data->output, sizeof(map_tmp));

    //buttons0
    for (uint32_t i = 8; i < ARRAY_SIZE(xinput_btns0_mask); i++) {
        if ((ctrl_data->map_mask[0] & BIT(i)) && xinput_btns0_mask[i]) {
            if (ctrl_data->btns[0].value & generic_btns_mask[i]) {
                map_tmp.dpad |= (uint8_t)xinput_btns0_mask[i];
                wired_data->cnt_mask[i] = ctrl_data->btns[0].cnt_mask[i];
            } else {
                map_tmp.dpad &= ~(uint8_t)xinput_btns0_mask[i];
                wired_data->cnt_mask[i] = 0;
            }
        }
    }
    //buttons1
    for (uint32_t i = 16; i < ARRAY_SIZE(xinput_btns1_mask); i++) {
        if ((ctrl_data->map_mask[1] & BIT(i)) && xinput_btns1_mask[i]) {
            if (ctrl_data->btns[1].value & generic_btns_mask[i]) {
                map_tmp.buttons |= (uint8_t)xinput_btns1_mask[i];
                wired_data->cnt_mask[i] = ctrl_data->btns[1].cnt_mask[i];
            } else {
                map_tmp.buttons &= ~(uint8_t)xinput_btns1_mask[i];
                wired_data->cnt_mask[i] = 0;
            }
        }
    }
    //joysticks
    for (uint32_t i = 0; i < XINPUT_JOY_MAX_AXES; i++) {
        if (ctrl_data->map_mask[0] & (axis_to_btn_mask(i) & ctrl_data->desc[0])) {
            if (ctrl_data->axes[i].value > ctrl_data->axes[i].meta->size_max) {
                map_tmp.joy_axes[xinput_joy_axes_idx[i]] = xinput_axes_meta[i].size_max;
            } else if (ctrl_data->axes[i].value < ctrl_data->axes[i].meta->size_min) {
                map_tmp.joy_axes[xinput_joy_axes_idx[i]] = xinput_axes_meta[i].size_min;
            } else {
                map_tmp.joy_axes[xinput_joy_axes_idx[i]] = ctrl_data->axes[i].value;
            }
        }
        wired_data->cnt_mask[axis_to_btn_id(i)] = ctrl_data->axes[i].cnt_mask;
    }
    //triggers
    for (uint32_t i = 0; i < XINPUT_TRIG_MAX_AXES; i++) {
        if (ctrl_data->map_mask[0] & (axis_to_btn_mask(i + XINPUT_JOY_MAX_AXES) & ctrl_data->desc[0])) {
            if (ctrl_data->axes[i + XINPUT_JOY_MAX_AXES].value > ctrl_data->axes[i + XINPUT_JOY_MAX_AXES].meta->size_max) {
                map_tmp.trigger_axes[gen_trig_axes_idx[i]] = xinput_axes_meta[i + XINPUT_JOY_MAX_AXES].size_max;
            } else if (ctrl_data->axes[i + XINPUT_JOY_MAX_AXES].value < ctrl_data->axes[i + XINPUT_JOY_MAX_AXES].meta->size_min) {
                map_tmp.trigger_axes[gen_trig_axes_idx[i]] = xinput_axes_meta[i + XINPUT_JOY_MAX_AXES].size_min;
            } else {
                map_tmp.trigger_axes[gen_trig_axes_idx[i]] = ctrl_data->axes[i + XINPUT_JOY_MAX_AXES].value;
            }
        }
        wired_data->cnt_mask[axis_to_btn_id(i + XINPUT_JOY_MAX_AXES)] = ctrl_data->axes[i + XINPUT_JOY_MAX_AXES].cnt_mask;
    }

    memcpy(wired_data->output, &map_tmp, sizeof(map_tmp));
    xinput_usb_force_update();
}

void usb_xinput_fb_to_generic(int32_t dev_mode, struct raw_fb *raw_fb_data, struct generic_fb *fb_data) {
    fb_data->wired_id = raw_fb_data->header.wired_id;
    fb_data->type = raw_fb_data->header.type;

    switch (fb_data->type) {
        case FB_TYPE_RUMBLE:
            fb_data->state = ((raw_fb_data->data[0] || raw_fb_data->data[1]) ? 1 : 0);
            fb_data->lf_pwr = raw_fb_data->data[0];
            fb_data->hf_pwr = raw_fb_data->data[1];
            break;
        case FB_TYPE_STATUS_LED:
            fb_data->led = raw_fb_data->data[0];
            break;
    }
}

void IRAM_ATTR usb_xinput_gen_turbo_mask(struct wired_data *wired_data) {
    struct xinput_report_in *map_mask = (struct xinput_report_in *)wired_data->output_mask;

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