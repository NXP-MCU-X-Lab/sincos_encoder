/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _APP_CRC_H_
#define _APP_CRC_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize CRC module for demo
 * 
 * @return true if successful, false otherwise
 */
bool APP_CRC_Init(void);

/**
 * @brief Run Tamagawa protocol CRC performance test
 * 
 * Compares hardware vs software CRC calculation performance for Tamagawa protocol
 */
void APP_CRC_RunTamagawaTest(void);

#endif /* _APP_CRC_H_ */
