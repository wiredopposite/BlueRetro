/*
 * Copyright (c) 2025, Jack Grogan
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _GENERIC_I2C_H_
#define _GENERIC_I2C_H_

#include <stdint.h>

void gen_i2c_init(uint32_t package);
void gen_i2c_port_cfg(uint16_t mask);

#endif /* _GENERIC_I2C_H_ */