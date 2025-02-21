/*
 * Copyright (c) 2025, Jack Grogan
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _GENERIC_SPI_H_
#define _GENERIC_SPI_H_

#include <stdint.h>

void gen_spi_init(uint32_t package);
void gen_spi_port_cfg(uint16_t mask);

#endif /* _GENERIC_SPI_H_ */