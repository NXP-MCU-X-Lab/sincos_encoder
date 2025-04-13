/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "app_eqdc.h"
#include "fsl_debug_console.h"
#include "fsl_inputmux.h"
#include "fsl_inputmux_connections.h"
#include "fsl_reset.h"
#include "fsl_eqdc.h"

/* Initialize EQDC module */
void APP_EQDC_Init(void)
{
    eqdc_config_t sEqdcConfig;
    
    /* Reset the EQDC module */
    RESET_PeripheralReset(kQDC0_RST_SHIFT_RSTn);

    /* Initialize INPUTMUX for EQDC signals */
    INPUTMUX_Init(DEMO_INPUTMUX);
    INPUTMUX_AttachSignal(DEMO_INPUTMUX, 0, kINPUTMUX_TrigIn10ToQdc0Phasea);
    INPUTMUX_AttachSignal(DEMO_INPUTMUX, 0, kINPUTMUX_TrigIn9ToQdc0Phaseb);
    INPUTMUX_AttachSignal(DEMO_INPUTMUX, 0, kINPUTMUX_TrigIn4ToQdc0Index);
    
    /* Configure EQDC */
    EQDC_GetDefaultConfig(&sEqdcConfig);
    sEqdcConfig.positionModulusValue = DEMO_ENCODER_DISK_LINE;
    
    /* Initialize EQDC */
    EQDC_Init(DEMO_EQDC, &sEqdcConfig);
    EQDC_SetOperateMode(DEMO_EQDC, kEQDC_QuadratureDecodeOperationMode);
    EQDC_DoSoftwareLoadInitialPositionValue(DEMO_EQDC);
    
    printf("EQDC initialized in quadrature decode mode\r\n");
}

/* Show EQDC status */
void APP_EQDC_ShowStatus(void)
{
    /* This read operation captures all position counters to hold registers */
    uint32_t position = EQDC_GetPosition(DEMO_EQDC);
    
    printf("EQDC Status:\r\n");
    printf("  Current position: %lu\r\n", position);
    printf("  Position difference: %d\r\n", (int16_t)EQDC_GetHoldPositionDifference(DEMO_EQDC));
    printf("  Revolution count: %u\r\n", EQDC_GetHoldRevolution(DEMO_EQDC));
}
