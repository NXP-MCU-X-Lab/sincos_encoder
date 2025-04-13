/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "app_crc.h"
#include "app_timer.h"
#include "fsl_crc.h"
#include "fsl_clock.h"
#include "fsl_reset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* CRC-8 polynomial: x^8 + x^2 + x + 1 (0x07) - used by Tamagawa */
#define CRC8_POLYNOMIAL      0x07
#define CRC8_INIT_VALUE      0x00
#define CRC8_XOR_VALUE       0x00


/* Table-based software implementation of CRC-8 for Tamagawa protocol */
static const uint8_t crc8_table[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

static uint8_t CRC_CalculateCRC8_Table(const uint8_t *data, uint32_t length) {
    uint8_t crc = CRC8_INIT_VALUE;
    
    for (uint32_t i = 0; i < length; i++) {
        crc = crc8_table[crc ^ data[i]];
    }
    
    return crc ^ CRC8_XOR_VALUE;
}

/* Hardware CRC-8 implementation with optimized initialization */
static uint8_t CRC_CalculateCRC8_HW(const uint8_t *data, uint32_t length) {
    uint16_t result;
    
    /* Write data to CRC engine */
    CRC_WriteData(CRC0, data, length);
    
    /* Get result and extract the high 8 bits (where our CRC-8 result should be) */
    result = CRC_Get16bitResult(CRC0);
    
    /* Return the high 8 bits of the 16-bit CRC result */
    return (uint8_t)((result >> 8) & 0xFF);
}

bool APP_CRC_Init(void) {
    /* Enable CRC clock */
    CLOCK_EnableClock(kCLOCK_Crc0);
    
    /* Release CRC from reset */
    RESET_ReleasePeripheralReset(kCRC0_RST_SHIFT_RSTn);
    
    crc_config_t config;
    
    /* Configure CRC hardware only once */
    config.polynomial = 0x0700U;  /* Shifted 8-bit polynomial 0x07 to 16-bit format */
    config.seed = 0x0000U;        /* Initial value 0x00 */
    config.reflectIn = false;     /* Tamagawa doesn't use input reflection */
    config.reflectOut = false;    /* Tamagawa doesn't use output reflection */
    config.complementChecksum = false; /* No complement needed */
    config.crcBits = kCrcBits16;  /* Use 16-bit CRC mode */
    config.crcResult = kCrcFinalChecksum;
    
    CRC_Init(CRC0, &config);
    
    return true;
}

void APP_CRC_RunTamagawaTest(void) {
    float tableTime, hwTime;
    uint8_t tableCrc, hwCrc;
    
    /* Test data - standard Tamagawa data request command */
    const uint8_t testData[] = {0x01, 0x05, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    const uint32_t testDataLen = sizeof(testData);
    
    printf("\r\n--- Tamagawa Protocol CRC-8 Test ---\r\n");
    printf("Testing command: ");
    for (uint32_t i = 0; i < testDataLen; i++) {
        printf("0x%02X ", testData[i]);
    }
    printf("\r\n\r\n");
    
    /* Calculate CRC with table-based method */
    TIMER_Start();
    tableCrc = CRC_CalculateCRC8_Table(testData, testDataLen);
    tableTime = TIMER_Stop();
    
    /* Calculate CRC with hardware method */
    TIMER_Start();
    hwCrc = CRC_CalculateCRC8_HW(testData, testDataLen);
    hwTime = TIMER_Stop();
    
    /* Print results */
    printf("CRC-8 Results:\r\n");
    printf("-------------\r\n");
    printf("1. Table-based SW: 0x%02X (%.2f us)\r\n", tableCrc, tableTime);
    printf("2. Hardware CRC:   0x%02X (%.2f us)\r\n\r\n", hwCrc, hwTime);
    
    /* Verify results match */
    printf("Verification:\r\n");
    printf("------------\r\n");
    if (tableCrc == hwCrc) {
        printf("- Implementations match\r\n");
    } else {
        printf("- Implementations don't match\r\n");
    }
    
    /* Performance comparison */
    printf("\r\nPerformance:\r\n");
    printf("-----------\r\n");
    printf("- HW vs Table: %.2fx (HW is %s)\r\n", 
           tableTime / hwTime, hwTime < tableTime ? "faster" : "slower");
    
    /* Run 1000 iterations to get more accurate timing */
    printf("\r\nPerformance test with 1000 iterations:\r\n");
    printf("------------------------------------\r\n");
    
    /* Table-based SW timing for 1000 iterations */
    TIMER_Start();
    for (uint32_t i = 0; i < 1000; i++) {
        tableCrc = CRC_CalculateCRC8_Table(testData, testDataLen);
    }
    tableTime = TIMER_Stop();
    
    /* Hardware timing for 1000 iterations */
    TIMER_Start();
    for (uint32_t i = 0; i < 1000; i++) {
        hwCrc = CRC_CalculateCRC8_HW(testData, testDataLen);
    }
    hwTime = TIMER_Stop();
    
    /* Print batch test results */
    printf("1. Table-based SW (1000x): %.2f us\r\n", tableTime);
    printf("2. Hardware CRC (1000x):   %.2f us\r\n\r\n", hwTime);
    
    printf("Speed comparison (1000 iterations):\r\n");
    printf("- HW vs Table: %.2fx (HW is %s)\r\n", 
           tableTime / hwTime, hwTime < tableTime ? "faster" : "slower");
    
}
