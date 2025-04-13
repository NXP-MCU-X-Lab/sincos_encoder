/*
 * Copyright (c) 2023
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _APP_ATAN_H_
#define _APP_ATAN_H_

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* Define math constants if not available */
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923f
#endif

/* Define math constants if not available */
#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962f  /* pi/4 */
#endif

/**
 * @brief Initialize the ATAN2 demo module
 * 
 * @return 0 on success, non-zero on error
 */
int ATAN_Init(void);

/**
 * @brief Fast ATAN2 implementation using CORDIC algorithm
 * 
 * @param y Y coordinate
 * @param x X coordinate
 * @return float Result in radians (-PI to PI)
 */
float ATAN_Fast(float y, float x);

/**
 * @brief Run ATAN2 benchmark comparing standard math.h atan2 with optimized version
 */
void ATAN_RunBenchmark(void);

#endif /* _APP_ATAN_H_ */
