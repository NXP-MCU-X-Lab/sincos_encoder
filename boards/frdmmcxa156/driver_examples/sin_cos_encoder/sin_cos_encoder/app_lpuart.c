/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "app_lpuart.h"
#include "fsl_clock.h"
#include "fsl_lpuart.h"
#include "fsl_lpuart_edma.h"


#define LPUART_TX_DMA_CHANNEL       0U
#define LPUART_RX_DMA_CHANNEL       1U
#define DEMO_LPUART_TX_EDMA_CHANNEL kDma0RequestLPUART1Tx
#define DEMO_LPUART_RX_EDMA_CHANNEL kDma0RequestLPUART1Rx
#define ECHO_BUFFER_LENGTH 8


/* LPUART user callback */
void LPUART_UserCallback(LPUART_Type *base, lpuart_edma_handle_t *handle, status_t status, void *userData);

lpuart_edma_handle_t g_lpuartEdmaHandle;
static edma_handle_t g_lpuartTxEdmaHandle;
static edma_handle_t g_lpuartRxEdmaHandle;
static lpuart_transfer_t receiveXfer;
    
    
AT_NONCACHEABLE_SECTION_INIT(uint8_t g_tipString[]) = "LPUART EDMA example\r\nSend back received data\r\nEcho every 8 characters\r\n";
AT_NONCACHEABLE_SECTION_INIT(uint8_t g_rxBuffer[ECHO_BUFFER_LENGTH]) = {0};

void LPUART_UserCallback(LPUART_Type *base, lpuart_edma_handle_t *handle, status_t status, void *userData)
{
    userData = userData;

    if (kStatus_LPUART_TxIdle == status)
    {
        printf("dma tx complete\r\n");
    }

    if (kStatus_LPUART_RxIdle == status)
    {
        printf("rx data, size:%d\r\n", ECHO_BUFFER_LENGTH);
        
        LPUART_ReceiveEDMA(LPUART1, &g_lpuartEdmaHandle, &receiveXfer);
    }
}


void LPUART1_Init(uint32_t baudRate)
{
    lpuart_config_t lpuartConfig;
    edma_config_t edmaConfig = {0};
    lpuart_transfer_t sendXfer;
    
    /* Configure clock source and divider for high speed */
    CLOCK_SetClockDiv(kCLOCK_DivLPUART1, 1U);
    CLOCK_AttachClk(kCLK_IN_to_LPUART1);
    
    RESET_ReleasePeripheralReset(kDMA_RST_SHIFT_RSTn);
    
    /* Get default configuration */
    LPUART_GetDefaultConfig(&lpuartConfig);
    
    /* Configure for high speed */
    lpuartConfig.baudRate_Bps = baudRate;
    lpuartConfig.enableTx = true;
    lpuartConfig.enableRx = true;
    
    /* Initialize LPUART1 */
    LPUART_Init(LPUART1, &lpuartConfig, CLOCK_GetLpuartClkFreq(1));
    
    /* Init the EDMA module */
    EDMA_GetDefaultConfig(&edmaConfig);
    EDMA_Init(DMA0, &edmaConfig);
    
    EDMA_CreateHandle(&g_lpuartTxEdmaHandle, DMA0, LPUART_TX_DMA_CHANNEL);
    EDMA_CreateHandle(&g_lpuartRxEdmaHandle, DMA0, LPUART_RX_DMA_CHANNEL);

    EDMA_SetChannelMux(DMA0, LPUART_TX_DMA_CHANNEL, DEMO_LPUART_TX_EDMA_CHANNEL);
    EDMA_SetChannelMux(DMA0, LPUART_RX_DMA_CHANNEL, DEMO_LPUART_RX_EDMA_CHANNEL);

    /* Create LPUART DMA handle. */
    LPUART_TransferCreateHandleEDMA(LPUART1, &g_lpuartEdmaHandle, LPUART_UserCallback, NULL, &g_lpuartTxEdmaHandle, &g_lpuartRxEdmaHandle);
                                    
    /* Start to echo. */
    receiveXfer.data     = g_rxBuffer;
    receiveXfer.dataSize = ECHO_BUFFER_LENGTH;

    LPUART_ReceiveEDMA(LPUART1, &g_lpuartEdmaHandle, &receiveXfer);
}

void LPUART1_SendData(const uint8_t *data, uint32_t length)
{
    lpuart_transfer_t sendXfer;
    sendXfer.data        = (void*)data;
    sendXfer.dataSize    = length;
    LPUART_SendEDMA(LPUART1, &g_lpuartEdmaHandle, &sendXfer);
    
//    /* Send data using blocking method */
//    LPUART_WriteBlocking(LPUART1, data, length);
}


