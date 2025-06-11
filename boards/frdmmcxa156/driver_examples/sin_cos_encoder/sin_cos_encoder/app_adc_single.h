/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _APP_ADC_H_
#define _APP_ADC_H_

#include "fsl_lpadc.h"
#include <stdbool.h>

/* ADC0 Channels */
#define ADC0_CHANNEL0     0U  /* ADC0_A0 - P2_0 */
#define ADC0_CHANNEL1     1U  /* ADC0_A1 - P2_1 */
#define ADC0_CHANNEL2     4U  /* ADC0_A4 - P2_2 */

/* ADC1 Channels */
#define ADC1_CHANNEL0     4U  /* ADC1_A4 - P2_3 */
#define ADC1_CHANNEL1     0U  /* ADC1_A0 - P2_4 */
#define ADC1_CHANNEL2     1U  /* ADC1_A1 - P2_5 */


/* Trigger IDs */
#define ADC_NORMAL_TRIGGER_ID   0U
#define ADC_FAST_TRIGGER_ID     1U

/* ADC channel count */
#define ADC_CHANNEL_COUNT 3

/* Maximum string length for result formatting */
#define ADC_RESULT_STRING_MAX_LEN 512


/* ADC Hardware Average Configuration */
#define ADC_HW_AVERAGE_MODE kLPADC_HardwareAverageCount1


/* ADC result structure */
typedef struct {
    lpadc_conv_result_t adc0Results[ADC_CHANNEL_COUNT]; /* ADC0 conversion results */
    lpadc_conv_result_t adc1Results[ADC_CHANNEL_COUNT]; /* ADC1 conversion results */
    bool dataReady;                                     /* Flag indicating new data is available */
} adc_results_t;

/* Single channel result structure */
typedef struct {
    uint16_t adc0Value;  /* ADC0 channel 0 value */
    uint16_t adc1Value;  /* ADC1 channel 0 value */
} adc_fast_result_t;

/* Function prototypes */
void ADC_Single_Init(void);
void ADC_StartSingleConversion(void);

/**
 * @brief Update ADC results and return pointer to results
 * 
 * @return const adc_results_t* Pointer to updated ADC results
 */
const adc_results_t* ADC_UpdateAndGetResults(void);

/**
 * @brief Get pointer to latest ADC results without performing a new conversion
 * 
 * @return const adc_results_t* Pointer to latest ADC results
 */
const adc_results_t* ADC_GetLatestResults(void);

/**
 * @brief Format ADC results into a readable string
 * 
 * @param results Pointer to ADC results structure
 * @param buffer Output buffer for formatted string
 * @param bufferSize Size of the output buffer
 * @return int Number of characters written to buffer (excluding null terminator), or negative value on error
 */
int ADC_SingleShowResult(const adc_results_t *results, char *buffer, size_t bufferSize);


/**
 * @brief Start fast conversion and wait for results
 * 
 * @return adc_fast_result_t Structure containing both ADC channel 0 values
 */
adc_fast_result_t ADC_StartAndGetFastResults(void);


void APP_ADC_EnableHW_Trigger(ADC_Type *base, uint8_t trid_id, bool enable);

#endif /* _APP_ADC_H_ */
