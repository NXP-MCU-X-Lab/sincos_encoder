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

#include "app_lpuart.h"
#include "app_adc.h"
#include "app_timer.h"



/* Test message */
static uint8_t txbuff[] = "LPUART1 high speed test\r\n";

/* LED GPIO definitions for FRDM-MCXA156 - P3_12 */
#define LED_GPIO        GPIO3
#define LED_PIN         12U     /* Pin 12 on port 3 */
#define LED_PORT        3U      /* Port 3 */

/* Setup FRO clock trimming for high-speed accuracy */
void app_FircAutoTrim(void)
{
    CLOCK_SetupExtClocking(8000000U);              // Enable the 8MHz external crystal
    SCG0->FIRCTCFG |= SCG_FIRCTCFG_TRIMSRC(2);     // Select the external crystal(SOSC) as the source
    SCG0->FIRCTCFG |= SCG_FIRCTCFG_TRIMDIV(7);     // Divide the SOSC to 1MHz
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
    app_FircAutoTrim();
    
    /* Initialize board hardware */
    BOARD_InitPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    
    /* Initialize LED */
    LED_Init();
    
    /* Initialize SysTick for 10Hz interrupt */
    SysTick_Init();
    
    /* Initialize LPUART1 with high baudrate */
    LPUART1_Init(2500000);
    
    /* Send test message */
    LPUART1_SendData(txbuff, sizeof(txbuff) - 1);

    /* Print debug info */
    printf("CoreClock: %d Hz\r\n", CLOCK_GetCoreSysClkFreq());
    printf("UART1: %d Hz\r\n", LPUART1_GetClockFreq());
    printf("LPUART1 initialized for high-speed operation\r\n");
    
    /* Initialize ADC */
    ADC_Init();

    /* Initialize high precision timer */
    TIMER_Init();
    
    printf("MCXA156 Demo Application\r\n");
    printf("Core System Clocks:\r\n");
    freq = CLOCK_GetFreq(kCLOCK_CoreSysClk);
    printf("  Core Clock:     %8u Hz (%3u MHz)\r\n", freq, freq / 1000000U);
    
    freq = CLOCK_GetFreq(kCLOCK_BusClk);
    printf("  Bus Clock:      %8u Hz (%3u MHz)\r\n", freq, freq / 1000000U);
    printf("========================\r\n");
    printf("Press any key to start ADC conversion with timing measurement\r\n");
    

    /* Main loop */
    while (1)
    {
        if (GETCHAR() != 0xFF)
        {
            const adc_results_t *results;
            char resultBuffer[ADC_RESULT_STRING_MAX_LEN];
            float conversionTime;
            
            /* Start timer measurement */
            TIMER_Start();
            
            /* Start ADC conversion */
            ADC_StartConversion();
            
           // SDK_DelayAtLeastUs(5, CLOCK_GetCoreSysClkFreq());
                
            /* Get ADC results - zero-copy approach */
            results = ADC_UpdateAndGetResults();
            
            /* Stop timer and get elapsed time */
            conversionTime = TIMER_Stop();
            
            /* Format results as string */
            ADC_ShowResult(results, resultBuffer, sizeof(resultBuffer));
            
            /* Print formatted results */
            printf("\r\n%s\r\n", resultBuffer);
            
            /* Print conversion time with 0.1us precision */
            printf("\r\nADC Conversion Time: %.1f us\r\n", conversionTime);
            
            printf("\r\nPress any key to start another ADC conversion\r\n");
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
