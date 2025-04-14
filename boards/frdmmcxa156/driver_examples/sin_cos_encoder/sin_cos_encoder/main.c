/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pin_mux.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_clock.h"
#include "fsl_gpio.h"
#include "fsl_inputmux.h"

#include "app_lpuart.h"
#include "app_adc.h"
#include "app_timer.h"
#include "app_crc.h"
#include "app_atan.h"
#include "app_eqdc.h"


/* Test message */
static uint8_t txbuff[] = "LPUART1 high speed test\r\n";

/* LED GPIO definitions for FRDM-MCXA156 - P3_12 */
#define LED_GPIO        GPIO3
#define LED_PIN         12U     /* Pin 12 on port 3 */
#define LED_PORT        3U      /* Port 3 */

/* Setup FRO clock trimming for high-speed accuracy */
void app_FircAutoTrim(uint32_t osc_frq)
{
    CLOCK_SetupExtClocking(osc_frq);              // Enable the 8MHz external crystal
    SCG0->FIRCTCFG |= SCG_FIRCTCFG_TRIMSRC(2);     // Select the external crystal(SOSC) as the source
    SCG0->FIRCTCFG |= SCG_FIRCTCFG_TRIMDIV((osc_frq/1000000)-1);     // Divide the SOSC to 1MHz
    SCG0->FIRCCSR  &= ~SCG_FIRCCSR_LK(1);          // Unlock the FIRCCSR register
    SCG0->FIRCCSR  |= SCG_FIRCCSR_FIRCTREN(1);     // Enable auto trim
    SCG0->FIRCCSR  |= SCG_FIRCCSR_FIRCTRUP(1);     // Enable update
    
    /* Wait for FIRC to be valid */
    while(!(SCG0->FIRCCSR & SCG_FIRCCSR_FIRCVLD_MASK));
    
    /* Wait for no error */
    while(SCG0->FIRCCSR & SCG_FIRCCSR_FIRCERR_MASK);
    
    /* Wait for FIRC auto trim locked */
    while(!(SCG0->FIRCCSR & SCG_FIRCCSR_TRIM_LOCK_MASK));
    
    SCG0->FIRCCSR |= SCG_FIRCCSR_LK(1);            // Lock the FIRCCSR register
}

/* Initialize LED */
void LED_Init(void)
{
    gpio_pin_config_t led_config = {
        kGPIO_DigitalOutput,
        0,
    };
    
    /* Enable GPIO clock */
    CLOCK_EnableClock(kCLOCK_GateGPIO3);
    
    /* Initialize GPIO pin */
    GPIO_PinInit(LED_GPIO, LED_PIN, &led_config);
    
    /* Turn off LED initially */
    GPIO_PinWrite(LED_GPIO, LED_PIN, 1);
}

/* Initialize SysTick for 10Hz interrupt */
void SysTick_Init(void)
{
    /* Get system clock frequency */
    uint32_t systemClock = CLOCK_GetCoreSysClkFreq();
    
    /* Configure SysTick for 10Hz (100ms) */
    SysTick_Config(systemClock / 10);
}

int main(void)
{
    uint32_t freq;
    
    /* Trim FRO for high-speed accuracy */
    app_FircAutoTrim(20*1000*1000);
    
    /* Initialize board hardware */
    BOARD_InitPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    
    /* Initialize ADC */
    ADC_Init();
    
    /* Initialize LED */
    LED_Init();
    
    /* Initialize SysTick for 10Hz interrupt */
    SysTick_Init();

    /* Print debug info */
    printf("CoreClock: %d Hz\r\n", CLOCK_GetCoreSysClkFreq());
    
    /* Initialize high precision timer */
    TIMER_Init();
    
    printf("MCXA156 Demo Application\r\n");
    printf("Core System Clocks:\r\n");
    freq = CLOCK_GetFreq(kCLOCK_CoreSysClk);
    printf("  Core Clock:     %8u Hz (%3u MHz)\r\n", freq, freq / 1000000U);

    freq = CLOCK_GetFreq(kCLOCK_BusClk);
    printf("  Bus Clock:      %8u Hz (%3u MHz)\r\n", freq, freq / 1000000U);
    printf("========================\r\n");
    printf("Available commands:\r\n");
    printf("  a - ADC Demo\r\n");
    printf("  f - Fast ADC Demo\r\n");
    printf("  u - UART Demo\r\n");
    printf("  t - Tamagawa CRC Demo\r\n");
    printf("  n - ATAN2 Benchmark\r\n");
    printf("  e - EQDC Encoder Demo\r\n");
    printf("> ");
    

    /* Main loop */
    while (1)
    {
        char key = GETCHAR();
        if (key != 0xFF)
        {
            switch(key)
            {
                case 'a':
                    /* ADC DEMO */
                    printf("\r\n--- ADC DEMO ---\r\n");
                    {
                        const adc_results_t *results;
                        char resultBuffer[ADC_RESULT_STRING_MAX_LEN];
                        float conversionTime;
                        
                        APP_ADC_EnableHW_Trigger(ADC0, ADC_NORMAL_TRIGGER_ID, true);
                        APP_ADC_EnableHW_Trigger(ADC0, ADC_FAST_TRIGGER_ID, false);
                        
                        TIMER_Start();
                        ADC_StartConversion();
                        results = ADC_UpdateAndGetResults();
                        conversionTime = TIMER_Stop();
                        
                        ADC_ShowResult(results, resultBuffer, sizeof(resultBuffer));
                        printf("\r\n%s\r\n", resultBuffer);
                        printf("\r\nADC Conversion Time: %.1f us\r\n", conversionTime);
                    }
                    break;
                    
                case 'f':
                    /* FAST CONVERSION DEMO */
                    printf("\r\n--- FAST ADC DEMO ---\r\n");
                    {
                        adc_fast_result_t fastResult;
                        float fastConversionTime;
                        
                        APP_ADC_EnableHW_Trigger(ADC0, ADC_NORMAL_TRIGGER_ID, false);
                        APP_ADC_EnableHW_Trigger(ADC0, ADC_FAST_TRIGGER_ID, true);
                        
                        TIMER_Start();
                        fastResult = ADC_StartAndGetFastResults();
                        fastConversionTime = TIMER_Stop();
                        
                        printf("ADC0 Channel %d Value: %d\r\n", ADC0_CHANNEL0, fastResult.adc0Value);
                        printf("ADC1 Channel %d Value: %d\r\n", ADC1_CHANNEL0, fastResult.adc1Value);
                        printf("\r\nFast ADC Conversion Time: %.1f us\r\n", fastConversionTime);
                    }
                    break;
                    
                case 'u':
                    /* UART DEMO */
                    printf("\r\n--- UART DEMO ---\r\n");
                
                    /* Initialize LPUART1 with high baudrate */
                    LPUART1_Init(2.5*1000*1000);
                
                    printf("UART1: %d Hz\r\n", CLOCK_GetLpuartClkFreq(1));
                    printf("LPUART1 initialized for high-speed operation\r\n");
                
                    LPUART1_SendData(txbuff, sizeof(txbuff) - 1);
                    printf("UART test message sent\r\n");
                    break;
                case 't':
                    /* TAMAGAWA CRC DEMO */
                
                    /* Initialize CRC module */
                    APP_CRC_Init();
                
                    printf("\r\n--- TAMAGAWA CRC DEMO ---\r\n");
                    APP_CRC_RunTamagawaTest();
                    break;
                case 'n':
                    /* ATAN2 DEMO */
                    printf("\r\n--- ATAN2 BENCHMARK ---\r\n");
                    ATAN_RunBenchmark();
                    break;
                case 'e':
                    /* EQDC DEMO */
                    printf("\r\n--- EQDC ENCODER DEMO ---\r\n");
                    APP_EQDC_Init();
                    printf("EQDC values will be printed every 500ms. Press any key to return.\r\n");
                    
                    while(1) {
                        APP_EQDC_ShowStatus();
                        
                        /* Delay for 500ms */
                        SDK_DelayAtLeastUs(500000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
                    }
                    break;
            }
            
            printf("\r\n> ");
        }
    }


}




/* SysTick interrupt handler */
void SysTick_Handler(void)
{
    /* Toggle LED */
    GPIO_PortToggle(LED_GPIO, 1u << LED_PIN);
}

void HardFault_Handler(void)
{
    printf("HardFault_Handler\r\n");
    while(1);
}
