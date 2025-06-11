/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "app_adc_dma.h"
#include "fsl_lpadc.h"
#include "fsl_edma.h"
#include "fsl_lptmr.h"
#include "fsl_clock.h"
#include "fsl_inputmux.h"
#include "fsl_debug_console.h"
#include <string.h>

/* Global variables */
static adc_dma_timer_result_t g_adcDmaResults;
static volatile bool g_systemInitialized = false;
static uint32_t g_currentSampleRate = 1000U; /* Default 1kHz */

/* DMA buffer - aligned for proper DMA operation */
AT_NONCACHEABLE_SECTION_ALIGN_INIT(uint32_t g_dmaBuffer[ADC_DMA_BUFFER_LENGTH], 32U) = {0x00U};

/* DMA interrupt handler */
void DMA_CH2_IRQHandler(void)
{
    if ((EDMA_GetChannelStatusFlags(ADC_DMA_CONTROLLER, ADC_DMA_CHANNEL) & kEDMA_InterruptFlag) != 0U)
    {
        EDMA_ClearChannelStatusFlags(ADC_DMA_CONTROLLER, ADC_DMA_CHANNEL, kEDMA_InterruptFlag);
        
        /* Copy data from DMA buffer to result buffer */
        memcpy(g_adcDmaResults.samples, g_dmaBuffer, sizeof(g_dmaBuffer));
        
        /* Update result structure */
        g_adcDmaResults.transferComplete = true;
        
        /* Stop LPTMR */
        LPTMR_StopTimer(ADC_DMA_LPTMR_BASE);
    }
}

/* Calculate timer period for desired sampling rate */
static uint32_t CalculateTimerPeriod(uint32_t sampleRateHz)
{
    /* Get actual LPTMR clock frequency */
    uint32_t lptmrClockHz = CLOCK_GetLptmrClkFreq();
    
    /* Calculate timer period (timer counts down from this value to 0) */
    uint32_t timerPeriod = (lptmrClockHz / sampleRateHz) - 1;
    
    /* Ensure minimum value (at least 1) */
    if (timerPeriod < 1) {
        timerPeriod = 1;
    }
    
    /* Ensure maximum value (16-bit timer) */
    if (timerPeriod > 0xFFFF) {
        timerPeriod = 0xFFFF;
    }
    
    return timerPeriod;
}

/* Initialize ADC */
static int InitializeADC(void)
{
    lpadc_config_t adcConfig;
    lpadc_conv_command_config_t cmdConfig;
    lpadc_conv_trigger_config_t triggerConfig;
    
    /* Set ADC clock */
    CLOCK_SetClockDiv(kCLOCK_DivADC0, 2u);
    CLOCK_AttachClk(kFRO_HF_to_ADC0);
    
    /* Initialize ADC */
    LPADC_GetDefaultConfig(&adcConfig);
    adcConfig.enableAnalogPreliminary = true;
    adcConfig.referenceVoltageSource = kLPADC_ReferenceVoltageAlt3;
    LPADC_Init(ADC_DMA_ADC_BASE, &adcConfig);
    
    /* Auto calibration */
    LPADC_DoAutoCalibration(ADC_DMA_ADC_BASE);
    
    /* Configure conversion command */
    LPADC_GetDefaultConvCommandConfig(&cmdConfig);
    cmdConfig.channelNumber = ADC_DMA_ADC_CHANNEL;
    cmdConfig.hardwareAverageMode = kLPADC_HardwareAverageCount1;
    cmdConfig.sampleTimeMode = kLPADC_SampleTimeADCK35;
    cmdConfig.conversionResolutionMode = kLPADC_ConversionResolutionHigh;
    LPADC_SetConvCommandConfig(ADC_DMA_ADC_BASE, ADC_DMA_COMMAND_ID, &cmdConfig);
    
    /* Configure trigger */
    LPADC_GetDefaultConvTriggerConfig(&triggerConfig);
    triggerConfig.targetCommandId = ADC_DMA_COMMAND_ID;
    triggerConfig.enableHardwareTrigger = true;
    LPADC_SetConvTriggerConfig(ADC_DMA_ADC_BASE, ADC_DMA_TRIGGER_ID, &triggerConfig);
    
    /* Enable FIFO DMA */
    LPADC_EnableFIFOWatermarkDMA(ADC_DMA_ADC_BASE, true);
    
    return 0;
}

/* Initialize LPTMR */
static int InitializeLPTMR(void)
{
    lptmr_config_t lptmrConfig;
    
    CLOCK_AttachClk(kFRO_HF_DIV_to_LPTMR0);
    CLOCK_SetClockDiv(kCLOCK_DivLPTMR0, 8);
    
    /* Initialize LPTMR */
    LPTMR_GetDefaultConfig(&lptmrConfig);
    lptmrConfig.prescalerClockSource = kLPTMR_PrescalerClock_3;
    
    LPTMR_Init(ADC_DMA_LPTMR_BASE, &lptmrConfig);
    
//    PRINTF("LPTMR clock:%dHz\r\n", CLOCK_GetLptmrClkFreq());
    return 0;
}

/* Initialize EDMA */
static int InitializeEDMA(void)
{
    edma_transfer_config_t transferConfig;
    edma_channel_config_t dmaChnlConfig;
    edma_config_t userConfig;
    
    /* Configure EDMA channel */
    dmaChnlConfig.channelDataSignExtensionBitPosition = 0U;
    dmaChnlConfig.channelPreemptionConfig.enableChannelPreemption = false;
    dmaChnlConfig.channelPreemptionConfig.enablePreemptAbility = true;
    dmaChnlConfig.channelRequestSource = ADC_DMA_REQUEST;
    dmaChnlConfig.protectionLevel = kEDMA_ChannelProtectionLevelUser;
    
    /* Initialize EDMA */
    EDMA_GetDefaultConfig(&userConfig);
    EDMA_Init(ADC_DMA_CONTROLLER, &userConfig);
    
    /* Configure transfer */
    void *srcAddr = (uint32_t *)&(ADC_DMA_ADC_BASE->RESFIFO);
    
    EDMA_PrepareTransfer(&transferConfig, 
                        srcAddr, sizeof(uint32_t), 
                        g_dmaBuffer, sizeof(g_dmaBuffer[0]), 
                        sizeof(g_dmaBuffer[0]),
                        sizeof(g_dmaBuffer), 
                        kEDMA_PeripheralToMemory);
    
    EDMA_SetTransferConfig(ADC_DMA_CONTROLLER, ADC_DMA_CHANNEL, &transferConfig, NULL);
    EDMA_InitChannel(ADC_DMA_CONTROLLER, ADC_DMA_CHANNEL, &dmaChnlConfig);
    
    /* Enable interrupt */
    EnableIRQ(DMA_CH2_IRQn);
    
    return 0;
}

/* Configure input multiplexer */
static void ConfigureInputMux(void)
{
    INPUTMUX_Init(INPUTMUX0);
    /* Connect LPTMR0 output to ADC0 trigger 0 */
    INPUTMUX_AttachSignal(INPUTMUX0, ADC_DMA_TRIGGER_ID, kINPUTMUX_Lptmr0ToAdc0Trigger);
    INPUTMUX_Deinit(INPUTMUX0);
}

/* Public functions */
int ADC_DMA_Timer_Init(void)
{
    if (g_systemInitialized) {
        return 0; /* Already initialized */
    }
    
    /* Initialize result structure */
    memset(&g_adcDmaResults, 0, sizeof(g_adcDmaResults));
    
    /* Configure input multiplexer */
    ConfigureInputMux();
    
    /* Initialize components */
    InitializeLPTMR();
    InitializeADC();
    InitializeEDMA();
    
    g_systemInitialized = true;
    return 0;
}

int ADC_DMA_Timer_SetSampleRate(uint32_t sampleRateHz)
{
    g_currentSampleRate = sampleRateHz;
    return 0;
}

int ADC_DMA_Timer_StartTransfer(void)
{
    if (!g_systemInitialized) {
        PRINTF("Error: System not initialized\r\n");
        return -1;
    }
    
    /* Reset buffers and flags */
    g_adcDmaResults.transferComplete = false;
    
    /* Configure timer period for current sample rate */
    uint32_t timerTicks = CalculateTimerPeriod(g_currentSampleRate);
    LPTMR_SetTimerPeriod(ADC_DMA_LPTMR_BASE, timerTicks);
    
    InitializeEDMA();
    
    /* Enable DMA channel */
    EDMA_EnableChannelRequest(ADC_DMA_CONTROLLER, ADC_DMA_CHANNEL);
    
    /* Start LPTMR to begin triggering */
    LPTMR_StartTimer(ADC_DMA_LPTMR_BASE);
    
    return 0;
}

bool ADC_DMA_Timer_IsComplete(void)
{
    return g_adcDmaResults.transferComplete;
}

const adc_dma_timer_result_t* ADC_DMA_Timer_GetResults(void)
{
    return &g_adcDmaResults;
}

void ADC_DMA_Timer_PrintResults(void)
{
    if (!ADC_DMA_Timer_IsComplete()) {
        PRINTF("Transfer not complete\r\n");
        return;
    }
    
    PRINTF("\r\n=== ADC Results (Rate: %dHz) ===\r\n", g_currentSampleRate);
    PRINTF("Samples: %d\r\n", ADC_DMA_BUFFER_LENGTH);
    
    /* Print all samples, 8 per line for readability */
    for (uint32_t i = 0; i < ADC_DMA_BUFFER_LENGTH; i++) {
        PRINTF("%5d ", (g_adcDmaResults.samples[i] & 0xFFFFU));
        
        /* New line every 8 samples */
        if ((i + 1) % 8 == 0) {
            PRINTF("\r\n");
        }
    }
    
    /* Add final newline if last line wasn't complete */
    if (ADC_DMA_BUFFER_LENGTH % 8 != 0) {
        PRINTF("\r\n");
    }
    
    PRINTF("===============================\r\n");
}
