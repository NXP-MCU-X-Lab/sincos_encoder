/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _APP_LPUART_H_
#define _APP_LPUART_H_

#include "fsl_lpuart.h"
#include <stdint.h>

/**
 * @brief Initialize LPUART1 with high baudrate
 * 
 * @param baudRate Baudrate to use (typically 2.5Mbps for Tamagawa)
 */
void LPUART1_Init(uint32_t baudRate);

/**
 * @brief Send data through LPUART1
 * 
 * @param data Data buffer pointer
 * @param length Data length
 */
void LPUART1_SendData(const uint8_t *data, uint32_t length);


#endif /* _APP_LPUART_H_ */
