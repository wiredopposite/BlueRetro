/*
 * Copyright (c) 2025, Jack Grogan
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ADAPTER_WIRED_USB_XINPUT_H_
#define _ADAPTER_WIRED_USB_XINPUT_H_
#include "adapter/adapter.h"

void usb_xinput_meta_init(struct wired_ctrl *ctrl_data);
void usb_xinput_init_buffer(int32_t dev_mode, struct wired_data *wired_data);
void usb_xinput_from_generic(int32_t dev_mode, struct wired_ctrl *ctrl_data, struct wired_data *wired_data);
void usb_xinput_gen_turbo_mask(struct wired_data *wired_data);
void usb_xinput_fb_to_generic(int32_t dev_mode, struct raw_fb *raw_fb_data, struct generic_fb *fb_data);

#endif /* _ADAPTER_WIRED_USB_XINPUT_H_ */
