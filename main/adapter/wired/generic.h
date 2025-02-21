/*
 * Copyright (c) 2025, Jack Grogan
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIRED_GENERIC_H_
#define _WIRED_GENERIC_H_
#include "adapter/adapter.h"

void gen_meta_init(struct wired_ctrl *ctrl_data);
void gen_init_buffer(int32_t dev_mode, struct wired_data *wired_data);
void gen_from_generic(int32_t dev_mode, struct wired_ctrl *ctrl_data, struct wired_data *wired_data);
void gen_gen_turbo_mask(struct wired_data *wired_data);
void gen_fb_to_generic(int32_t dev_mode, struct raw_fb *raw_fb_data, struct generic_fb *fb_data);

#endif /* _WIRED_GENERIC_H_ */
