/*
 * MIT License
 *
 * Copyright (c) 2017 Ingenia-CAT S.L.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SERCOMM_POSIX_TIME_H_
#define SERCOMM_POSIX_TIME_H_

#include <time.h>

#if defined(__MACH__) && defined(__APPLE__)
#include <AvailabilityMacros.h>
#endif

#ifdef MAC_OS_X_VERSION_MAX_ALLOWED
#if MAC_OS_X_VERSION_MAX_ALLOWED < 101200
/*
 * MacOS X versions < 10.12 do not provide clock_gettime. This module provides
 * an implementation of clock_gettime and the definition of the clockid_t type.
 * Time is always retrieved from the Mach Kernel SYSTEM_TIME clock. Therefore,
 * clk_id is just provided for compatibility purposes but in practice it is
 * ignored. Note that it may not behave exactly in the same manner as detailed
 * in the POSIX specification.
 */

typedef enum
{
    CLOCK_REALTIME = 0,
    CLOCK_MONOTONIC = 6,
    CLOCK_MONOTONIC_RAW = 4,
    CLOCK_MONOTONIC_RAW_APPROX = 5,
    CLOCK_UPTIME_RAW = 8,
    CLOCK_UPTIME_RAW_APPROX = 9,
    CLOCK_PROCESS_CPUTIME_ID = 12,
    CLOCK_THREAD_CPUTIME_ID = 16
} clockid_t;

int clock_gettime(clockid_t clk_id, struct timespec *ts);
#endif
#endif

/**
 * Subtract two times (a - b).
 *
 * @param [in] a
 *      Time.
 * @param [in] b
 *      Time.
 *
 * @return Subtracted time.
 */
struct timespec clock__diff(const struct timespec *a, const struct timespec *b);

#endif
