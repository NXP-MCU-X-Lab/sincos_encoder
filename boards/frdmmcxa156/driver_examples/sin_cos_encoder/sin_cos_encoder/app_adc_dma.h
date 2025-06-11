/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _ADC_DMA_TIMER_H_
#define _ADC_DMA_TIMER_H_

#include <stdint.h>
#include <stdbool.h>

/* Hardware Configuration */
#define ADC_DMA_ADC_BASE         ADC0
#define ADC_DMA_ADC_CHANNEL      0U      /* ADC0_A0 - P2_0 */
#define ADC_DMA_COMMAND_ID       5U      
#define ADC_DMA_TRIGGER_ID       0U      

#define ADC_DMA_LPTMR_BASE       LPTMR0  

#define ADC_DMA_CONTROLLER       DMA0
#define ADC_DMA_CHANNEL          2U      
#define ADC_DMA_REQUEST          kDma0RequestMuxAdc0FifoRequest

#define ADC_DMA_BUFFER_LENGTH    128


/* ADC DMA result structure */
typedef struct {
    uint32_t samples[ADC_DMA_BUFFER_LENGTH]; 
    volatile bool transferComplete;
} adc_dma_timer_result_t;

/* Function prototypes */

/**
 * @brief Initialize ADC DMA Timer system
 * @return 0 on success, negative on error
 */
int ADC_DMA_Timer_Init(void);

/**
 * @brief Set sampling rate
 * @param sampleRateHz Desired sampling rate in Hz (100-16000)
 * @return 0 on success, negative on error
 */
int ADC_DMA_Timer_SetSampleRate(uint32_t sampleRateHz);

/**
 * @brief Start a single ADC DMA transfer (128 samples)
 * @return 0 on success, negative on error
 */
int ADC_DMA_Timer_StartTransfer(void);

/**
 * @brief Check if current transfer is complete
 * @return true if transfer complete, false otherwise
 */
bool ADC_DMA_Timer_IsComplete(void);

/**
 * @brief Get results from the last completed transfer
 * @return Pointer to result structure
 */
const adc_dma_timer_result_t* ADC_DMA_Timer_GetResults(void);

/**
 * @brief Print results from the last completed transfer
 */
void ADC_DMA_Timer_PrintResults(void);

#endif /* _ADC_DMA_TIMER_H_ */
