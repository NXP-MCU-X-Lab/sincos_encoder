/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "app_atan.h"
#include "app_timer.h"
#include "fsl_debug_console.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>


int ATAN_Init(void)
{
    /* No specific initialization needed */
    return 0;
}

float ATAN_Fast(float y, float x)
{
    /* Handle special cases */
    if (x == 0.0f && y == 0.0f) {
        return 0.0f; /* undefined, but return 0 */
    }
    
    if (x == 0.0f) {
        return (y > 0.0f) ? M_PI_2 : -M_PI_2;
    }
    
    /* Ensure input is in [-1, +1] */
    bool swap = fabsf(x) < fabsf(y);
    float atan_input = (swap ? x : y) / (swap ? y : x);

    /* Constants for polynomial approximation - original values preserved */
    const float a1  =  0.99997726f;
    const float a3  = -0.33262347f;
    const float a5  =  0.19354346f;
    const float a7  = -0.11643287f;
    const float a9  =  0.05265332f;
    const float a11 = -0.01172120f;

    /* Calculate polynomial approximation */
    float x_sq = atan_input * atan_input;
    float res = atan_input * (a1 + x_sq * (a3 + x_sq * (a5 + x_sq * (a7 + x_sq * (a9 + x_sq * a11)))));

    /* If swapped, adjust atan output - original logic preserved */
    res = swap ? (atan_input >= 0.0f ? M_PI_2 : -M_PI_2) - res : res;
    
    /* Adjust quadrants - original logic preserved */
    if (x < 0.0f) {
        res = (y >= 0.0f) ? M_PI + res : -M_PI + res;
    }

    return res;
}


/* Test points covering various quadrants and edge cases */
#define NUM_TEST_POINTS 10
static const float test_x[NUM_TEST_POINTS] = {1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, -1.0f, 0.5f, 100.0f};
static const float test_y[NUM_TEST_POINTS] = {1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, -1.0f, 0.866f, 0.1f};

void ATAN_RunBenchmark(void)
{
    float std_result, fast_result;
    float std_time, fast_time;
    float max_error = 0.0f, avg_error = 0.0f;
    float error;
    int i, j;
    
    printf("ATAN2 Benchmark - Standard vs SVML-Style Optimized\r\n");
    printf("---------------------------------------\r\n");
    printf("| Point |    X    |    Y    | Standard ATAN2 | Fast ATAN2 | Error (rad) | Error (deg) |\r\n");
    printf("|-------|---------|---------|----------------|------------|-------------|-------------|\r\n");
    
    /* Test accuracy on predefined points */
    for (i = 0; i < NUM_TEST_POINTS; i++) {
        std_result = atan2f(test_y[i], test_x[i]);
        fast_result = ATAN_Fast(test_y[i], test_x[i]);
        
        error = fabsf(std_result - fast_result);
        avg_error += error;
        if (error > max_error) max_error = error;
        
        printf("| %5d | %7.3f | %7.3f | %14.6f | %10.6f | %11.8f | %11.6f |\r\n", 
               i, test_x[i], test_y[i], std_result, fast_result, error, error * 180.0f / M_PI);
    }
    
    avg_error /= NUM_TEST_POINTS;
    
    printf("---------------------------------------\r\n");
    printf("Average Error: %.8f rad (%.6f deg)\r\n", avg_error, avg_error * 180.0f / M_PI);
    printf("Maximum Error: %.8f rad (%.6f deg)\r\n", max_error, max_error * 180.0f / M_PI);
    
    /* Single call timing measurement */
    printf("\r\nSingle Call Timing Measurement:\r\n");
    float x = 0.5f, y = 0.866f; // 30 degree angle
    
    // Standard atan2 single call
    TIMER_Start();
    std_result = atan2f(y, x);
    std_time = TIMER_Stop();
    
    // Fast atan2 single call
    TIMER_Start();
    fast_result = ATAN_Fast(y, x);
    fast_time = TIMER_Stop();
    
    printf("Standard atan2 single call: %.3f us\r\n", std_time);
    printf("Fast atan2 single call:     %.3f us\r\n", fast_time);
    printf("Single call speed ratio:    %.1fx\r\n", std_time / fast_time);
    
    /* Benchmark performance (1000 iterations) */
    printf("\r\nPerformance Benchmark (1000 iterations):\r\n");
    
    /* Standard atan2 benchmark */
    TIMER_Start();
    for (j = 0; j < 1000; j++) {
        for (i = 0; i < NUM_TEST_POINTS; i++) {
            std_result = atan2f(test_y[i], test_x[i]);
            /* Prevent optimization */
            if (std_result > 1000.0f) printf(".");
        }
    }
    std_time = TIMER_Stop();
    
    /* Fast atan2 benchmark */
    TIMER_Start();
    for (j = 0; j < 1000; j++) {
        for (i = 0; i < NUM_TEST_POINTS; i++) {
            fast_result = ATAN_Fast(test_y[i], test_x[i]);
            /* Prevent optimization */
            if (fast_result > 1000.0f) printf(".");
        }
    }
    fast_time = TIMER_Stop();
    
    printf("Standard atan2: %.1f us\r\n", std_time);
    printf("Fast atan2:     %.1f us\r\n", fast_time);
    printf("Speed improvement: %.1fx\r\n", std_time / fast_time);
    
}

