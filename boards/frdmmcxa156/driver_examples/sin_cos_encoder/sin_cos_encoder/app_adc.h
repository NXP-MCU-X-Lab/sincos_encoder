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
#define ADC0_CHANNEL0     5U
#define ADC0_CHANNEL1     8U
#define ADC0_CHANNEL2     10U

/* ADC1 Channels */
#define ADC1_CHANNEL0     6U
#define ADC1_CHANNEL1     9U
#define ADC1_CHANNEL2     11U

/* Command IDs */
#define ADC0_CMD_ID0      1U
#define ADC0_CMD_ID1      2U
#define ADC0_CMD_ID2      3U

#define ADC1_CMD_ID0      1U
#define ADC1_CMD_ID1      2U
#define ADC1_CMD_ID2      3U

/* Trigger IDs */
#define ADC_TRIGGER_ID   0U

/* ADC channel count */
#define ADC_CHANNEL_COUNT 3U

/* Maximum string length for result formatting */
#define ADC_RESULT_STRING_MAX_LEN 512

/* ADC result structure */
typedef struct {
    lpadc_conv_result_t adc0Results[ADC_CHANNEL_COUNT]; /* ADC0 conversion results */
    lpadc_conv_result_t adc1Results[ADC_CHANNEL_COUNT]; /* ADC1 conversion results */
    bool dataReady;                                     /* Flag indicating new data is available */
} adc_results_t;

/* Function prototypes */
void ADC_Init(void);
void ADC_StartConversion(void);

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
 * @brief Check if new ADC data is ready
 * 
 * @return bool True if new data is available
 */
bool ADC_IsDataReady(void);

/**
 * @brief Clear data ready flag
 */
void ADC_ClearDataReadyFlag(void);

/**
 * @brief Format ADC results into a readable string
 * 
 * @param results Pointer to ADC results structure
 * @param buffer Output buffer for formatted string
 * @param bufferSize Size of the output buffer
 * @return int Number of characters written to buffer (excluding null terminator), or negative value on error
 */
int ADC_ShowResult(const adc_results_t *results, char *buffer, size_t bufferSize);

#endif /* _APP_ADC_H_ */
