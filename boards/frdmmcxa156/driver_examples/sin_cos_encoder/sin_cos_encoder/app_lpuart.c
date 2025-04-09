/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "app_lpuart.h"
#include "fsl_clock.h"
#include "fsl_lpuart.h"

/* Receive buffer */
#define RX_BUFFER_SIZE 64
static uint8_t rxBuffer[RX_BUFFER_SIZE];
static volatile uint32_t rxIndex = 0;

/* LPUART1 interrupt handler */
void LPUART1_IRQHandler(void)
{
    uint8_t data;
    
    /* Check if receive interrupt */
    if ((kLPUART_RxDataRegFullFlag) & LPUART_GetStatusFlags(LPUART1))
    {
        /* Read received data */
        data = LPUART_ReadByte(LPUART1);
        
        /* Echo received data immediately */
        LPUART_WriteByte(LPUART1, data);
        
        /* Store to receive buffer */
        if (rxIndex < RX_BUFFER_SIZE)
        {
            rxBuffer[rxIndex++] = data;
        }
        
        /* Reset buffer if newline received */
        if (data == '\r' || data == '\n')
        {
            rxIndex = 0;
        }
    }
    
    /* Handle errors */
    uint32_t status = LPUART_GetStatusFlags(LPUART1);
    if (status & (kLPUART_RxOverrunFlag | kLPUART_NoiseErrorFlag | 
                 kLPUART_FramingErrorFlag | kLPUART_ParityErrorFlag))
    {
        /* Clear error flags */
        LPUART_ClearStatusFlags(LPUART1, status & (kLPUART_RxOverrunFlag | 
                               kLPUART_NoiseErrorFlag | kLPUART_FramingErrorFlag | 
                               kLPUART_ParityErrorFlag));
    }
}

void LPUART1_Init(uint32_t baudRate)
{
    lpuart_config_t config;
    
    /* Configure clock source and divider for high speed */
    CLOCK_SetClockDiv(kCLOCK_DivLPUART1, 1U);
    CLOCK_AttachClk(kFRO_HF_DIV_to_LPUART1);
    
    /* Get default configuration */
    LPUART_GetDefaultConfig(&config);
    
    /* Configure for high speed */
    config.baudRate_Bps = baudRate;
    config.enableTx = true;
    config.enableRx = true;
    
    /* Initialize LPUART1 */
    LPUART_Init(LPUART1, &config, CLOCK_GetLpuartClkFreq(1));
    
    /* Enable RX interrupt */
    LPUART_EnableInterrupts(LPUART1, kLPUART_RxDataRegFullInterruptEnable);
    
    /* Enable NVIC interrupt */
    EnableIRQ(LPUART1_IRQn);
    
    /* Reset receive buffer */
    rxIndex = 0;
}

void LPUART1_SendData(const uint8_t *data, uint32_t length)
{
    /* Send data using blocking method */
    LPUART_WriteBlocking(LPUART1, data, length);
}

uint32_t LPUART1_GetClockFreq(void)
{
    /* Get LPUART1 clock frequency */
    return CLOCK_GetLpuartClkFreq(1);
}
