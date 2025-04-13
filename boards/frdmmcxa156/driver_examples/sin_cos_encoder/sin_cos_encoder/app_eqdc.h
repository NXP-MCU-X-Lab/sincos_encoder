/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_EQDC_H_
#define APP_EQDC_H_

#include <stdint.h>
#include "fsl_eqdc.h"

/* EQDC definitions */
#define DEMO_EQDC     QDC0
#define DEMO_INPUTMUX INPUTMUX0

#define DEMO_ENCODER_DISK_LINE 0xFFFFFFFFU

/* Function prototypes */
void APP_EQDC_Init(void);
void APP_EQDC_ShowStatus(void);

#endif /* APP_EQDC_H_ */
