/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "fsl_lpadc.h"
#include "fsl_inputmux.h"
#include "fsl_clock.h"
#include "fsl_reset.h"
#include "app_adc.h"
#include <string.h>
#include <stdio.h>

/* ADC result values storage */
static adc_results_t adcResults;

/* Fast conversion command configuration */
static lpadc_conv_command_config_t cmdConfigFast0;
static lpadc_conv_command_config_t cmdConfigFast1;
static lpadc_conv_trigger_config_t triggerConfigFast;

/* ADC configuration helper function */
static void ConfigureADC(ADC_Type *base, uint32_t channel0, uint32_t channel1, uint32_t channel2)
{
    lpadc_conv_command_config_t cmdConfig;
    lpadc_conv_trigger_config_t triggerConfig;
    
    /* Configure commands */
    LPADC_GetDefaultConvCommandConfig(&cmdConfig);
    
    cmdConfig.conversionResolutionMode = kLPADC_ConversionResolutionHigh;
    cmdConfig.hardwareAverageMode = ADC_HW_AVERAGE_MODE;
    
    /* Command 1 - Channel 0 */
    cmdConfig.channelNumber = channel0;
    cmdConfig.chainedNextCommandNumber = 2; // Chain to command 2
    LPADC_SetConvCommandConfig(base, 1U, &cmdConfig);
    
    /* Command 2 - Channel 1 */
    cmdConfig.channelNumber = channel1;
    cmdConfig.chainedNextCommandNumber = 3U; // Chain to command 3
    LPADC_SetConvCommandConfig(base, 2U, &cmdConfig);
    
    /* Command 3 - Channel 2 */
    cmdConfig.channelNumber = channel2;
    cmdConfig.chainedNextCommandNumber = 0U; // End of chain
    LPADC_SetConvCommandConfig(base, 3U, &cmdConfig);

    /* Configure trigger */
    LPADC_GetDefaultConvTriggerConfig(&triggerConfig);
    triggerConfig.targetCommandId = 1U;      // Target first command
    triggerConfig.enableHardwareTrigger = true;
    LPADC_SetConvTriggerConfig(base, ADC_NORMAL_TRIGGER_ID, &triggerConfig);
}

void APP_ADC_EnableHW_Trigger(ADC_Type *base, uint8_t trid_id, bool enable)
{
    if(enable)
    {
        base->TCTRL[trid_id] |= ADC_TCTRL_HTEN_MASK;
    }
    else
    {
        base->TCTRL[trid_id] &= ~ADC_TCTRL_HTEN_MASK;
    }
}

/* Configure fast conversion commands (only channel 0) */
static void ConfigureFastConversion(void)
{
    /* Configure fast command for ADC0 - Channel 0 only */
    LPADC_GetDefaultConvCommandConfig(&cmdConfigFast0);
    cmdConfigFast0.channelNumber = ADC0_CHANNEL0;
    cmdConfigFast0.conversionResolutionMode = kLPADC_ConversionResolutionHigh;
    cmdConfigFast0.hardwareAverageMode = ADC_HW_AVERAGE_MODE;
    cmdConfigFast0.chainedNextCommandNumber = 0U; // No chaining
    LPADC_SetConvCommandConfig(ADC0, 4U, &cmdConfigFast0); // Use command ID 4 for fast conversion
    
    /* Configure fast command for ADC1 - Channel 0 only */
    LPADC_GetDefaultConvCommandConfig(&cmdConfigFast1);
    cmdConfigFast1.channelNumber = ADC1_CHANNEL0;
    cmdConfigFast1.conversionResolutionMode = kLPADC_ConversionResolutionHigh;
    cmdConfigFast1.hardwareAverageMode = ADC_HW_AVERAGE_MODE;
    cmdConfigFast1.chainedNextCommandNumber = 0U; // No chaining
    LPADC_SetConvCommandConfig(ADC1, 4U, &cmdConfigFast1); // Use command ID 4 for fast conversion
    
    /* Configure fast trigger (using trigger ID 1) */
    LPADC_GetDefaultConvTriggerConfig(&triggerConfigFast);
    triggerConfigFast.targetCommandId = 4U;      // Target fast command
    triggerConfigFast.enableHardwareTrigger = true;
    LPADC_SetConvTriggerConfig(ADC0, ADC_FAST_TRIGGER_ID, &triggerConfigFast);
    LPADC_SetConvTriggerConfig(ADC1, ADC_FAST_TRIGGER_ID, &triggerConfigFast);
    
    /* Configure additional INPUTMUX for fast trigger */
    INPUTMUX_Init(INPUTMUX0);
    INPUTMUX_AttachSignal(INPUTMUX0, 1, kINPUTMUX_ArmTxevToAdc0Trigger);
    INPUTMUX_AttachSignal(INPUTMUX0, 1, kINPUTMUX_ArmTxevToAdc1Trigger);
    INPUTMUX_Deinit(INPUTMUX0);
}

void ADC_Init(void)
{
    lpadc_config_t adcConfigStruct;
    
    /* Initialize result structure */
    memset(&adcResults, 0, sizeof(adcResults));
    adcResults.dataReady = false;
    
    /* Release peripheral reset for ADC0 and ADC1 */
    RESET_ReleasePeripheralReset(kADC0_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kADC1_RST_SHIFT_RSTn);

    /* Set clock source and divider for ADC0 and ADC1 */
    CLOCK_SetClockDiv(kCLOCK_DivADC0, 1U);
    CLOCK_SetClockDiv(kCLOCK_DivADC1, 1U);
    CLOCK_AttachClk(kCLK_IN_to_ADC0);
    CLOCK_AttachClk(kCLK_IN_to_ADC1);

    /* Initialize INPUTMUX */
    INPUTMUX_Init(INPUTMUX0);
    
    /* Configure ARM_TXEV as trigger source for both ADCs */
    INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_ArmTxevToAdc0Trigger);
    INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_ArmTxevToAdc1Trigger);
    
    /* Deinitialize INPUTMUX to apply configuration */
    INPUTMUX_Deinit(INPUTMUX0);

    /* Initialize ADC configuration */
    LPADC_GetDefaultConfig(&adcConfigStruct);
    adcConfigStruct.enableAnalogPreliminary = true;
    adcConfigStruct.powerLevelMode = kLPADC_PowerLevelAlt4;
    adcConfigStruct.referenceVoltageSource = kLPADC_ReferenceVoltageAlt3; /* VDDA */
    adcConfigStruct.conversionAverageMode = kLPADC_ConversionAverage1024; 
    
    /* Initialize both ADCs with the same configuration */
    LPADC_Init(ADC0, &adcConfigStruct);
    LPADC_Init(ADC1, &adcConfigStruct);

    /* Perform auto-calibration for both ADCs */
    LPADC_DoAutoCalibration(ADC0);
    LPADC_DoAutoCalibration(ADC1);

    /* Configure ADC0 channels and command chain */
    ConfigureADC(ADC0, ADC0_CHANNEL0, ADC0_CHANNEL1, ADC0_CHANNEL2);
    
    /* Configure ADC1 channels and command chain */
    ConfigureADC(ADC1, ADC1_CHANNEL0, ADC1_CHANNEL1, ADC1_CHANNEL2);
    
    /* Configure fast conversion commands */
    ConfigureFastConversion();

    /* Get hardware average count as string */
    const char *hwAvgStr;
    switch(ADC_HW_AVERAGE_MODE) {
        case kLPADC_HardwareAverageCount1:  hwAvgStr = "1 sample"; break;
        case kLPADC_HardwareAverageCount2:  hwAvgStr = "2 samples"; break;
        case kLPADC_HardwareAverageCount4:  hwAvgStr = "4 samples"; break;
        case kLPADC_HardwareAverageCount8:  hwAvgStr = "8 samples"; break;
        case kLPADC_HardwareAverageCount16: hwAvgStr = "16 samples"; break;
        case kLPADC_HardwareAverageCount32: hwAvgStr = "32 samples"; break;
        case kLPADC_HardwareAverageCount64: hwAvgStr = "64 samples"; break;
        case kLPADC_HardwareAverageCount128: hwAvgStr = "128 samples"; break;
        default: hwAvgStr = "Unknown"; break;
    }

    PRINTF("ADC initialization complete\r\n");
    PRINTF("ADC0 channels: %d, %d, %d\r\n", ADC0_CHANNEL0, ADC0_CHANNEL1, ADC0_CHANNEL2);
    PRINTF("ADC1 channels: %d, %d, %d\r\n", ADC1_CHANNEL0, ADC1_CHANNEL1, ADC1_CHANNEL2);
    PRINTF("Hardware average: %s\r\n", hwAvgStr);
    PRINTF("ADC resolution: 16-bit\r\n");
    PRINTF("Fast conversion configured for channels %d and %d\r\n", ADC0_CHANNEL0, ADC1_CHANNEL0);
}

void ADC_StartConversion(void)
{
    /* Clear data ready flag before starting new conversion */
    adcResults.dataReady = false;
    
    /* Generate ARM_TXEV to trigger both ADCs simultaneously */
    __SEV();
}


/* Helper function to get ADC results */
static void GetADCResults(ADC_Type *base, lpadc_conv_result_t *results)
{
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
        /* Wait for conversion result */
        while (!LPADC_GetConvResult(base, &results[i])) {}
    }
}

const adc_results_t* ADC_UpdateAndGetResults(void)
{
    /* Get results from both ADCs */
    GetADCResults(ADC0, adcResults.adc0Results);
    GetADCResults(ADC1, adcResults.adc1Results);
    
    /* Set data ready flag */
    adcResults.dataReady = true;
    
    /* Return pointer to results */
    return &adcResults;
}

const adc_results_t* ADC_GetLatestResults(void)
{
    return &adcResults;
}


/* Start fast conversion and wait for results using direct register access */
adc_fast_result_t ADC_StartAndGetFastResults(void)
{
    __SEV();
    
    adc_fast_result_t result = {0};
    uint32_t tmp32;
    
    /* Get ADC0 channel 0 result - direct register access */
    while (1) {
        tmp32 = ADC0->RESFIFO;
        if ((tmp32 & ADC_RESFIFO_VALID_MASK) == ADC_RESFIFO_VALID_MASK) {
            break; /* Valid data available */
        }
    }
    result.adc0Value = (uint16_t)(tmp32 & ADC_RESFIFO_D_MASK);
    
    /* Get ADC1 channel 0 result - direct register access */
    while (1) {
        tmp32 = ADC1->RESFIFO;
        if ((tmp32 & ADC_RESFIFO_VALID_MASK) == ADC_RESFIFO_VALID_MASK) {
            break; /* Valid data available */
        }
    }
    result.adc1Value = (uint16_t)(tmp32 & ADC_RESFIFO_D_MASK);
    
    return result;
}

int ADC_ShowResult(const adc_results_t *results, char *buffer, size_t bufferSize)
{
    int written = 0;
    uint32_t channelNumbers[2][ADC_CHANNEL_COUNT] = {
        {ADC0_CHANNEL0, ADC0_CHANNEL1, ADC0_CHANNEL2},
        {ADC1_CHANNEL0, ADC1_CHANNEL1, ADC1_CHANNEL2}
    };
    
    /* Check parameters */
    if (!results || !buffer || bufferSize == 0) {
        return -1;
    }
    
    /* Clear buffer */
    buffer[0] = '\0';
    
    /* Format ADC0 results */
    written += snprintf(buffer + written, bufferSize - written, "ADC0 Results:\r\n");
    
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
        written += snprintf(buffer + written, bufferSize - written,
                          "Channel %d - Value: %d, CmdID: %lu\r\n",
                          channelNumbers[0][i],
                          results->adc0Results[i].convValue,
                          results->adc0Results[i].commandIdSource);
    }
    
    /* Format ADC1 results */
    written += snprintf(buffer + written, bufferSize - written, "\r\nADC1 Results:\r\n");
    
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
        written += snprintf(buffer + written, bufferSize - written,
                          "Channel %d - Value: %d, CmdID: %lu\r\n",
                          channelNumbers[1][i],
                          results->adc1Results[i].convValue,
                          results->adc1Results[i].commandIdSource);
    }
    
    /* Ensure buffer is null-terminated */
    if (written >= bufferSize) {
        buffer[bufferSize - 1] = '\0';
        return bufferSize - 1;
    }
    
    return written;
}
