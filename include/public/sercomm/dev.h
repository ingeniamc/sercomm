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

#ifndef PUBLIC_SERCOMM_DEV_H_
#define PUBLIC_SERCOMM_DEV_H_

#include "common.h"
#include "types.h"

#include <string.h>

SER_BEGIN_DECL

/**
 * @file sercomm/dev.h
 * @brief Devices.
 * @defgroup SER_DEV Devices
 * @ingroup SER
 * @{
 */

/** Serial device monitor. */
typedef struct ser_dev_mon ser_dev_mon_t;

/** Serial device path maximum size. */
#define SER_DEV_PATH_SZ 128U

/** Serial device. */
typedef struct
{
    /** Path (e.g. COM1, /dev/ttyS0, ...) */
    char path[SER_DEV_PATH_SZ];
    /** Vendor ID (USB only) */
    uint16_t vid;
    /** Product ID (USB only) */
    uint16_t pid;
} ser_dev_t;

/** Serial devices list. */
typedef struct ser_dev_list ser_dev_list_t;

/** Serial devices list (structure). */
struct ser_dev_list
{
    /** Serial device */
    ser_dev_t dev;
    /** Next device */
    ser_dev_list_t *next;
};

/** Serial devices monitor event types. */
typedef enum
{
    /** Device added */
    SER_DEV_EVT_ADDED,
    /** Device removed */
    SER_DEV_EVT_REMOVED
} ser_dev_evt_t;

/** Event callback. */
typedef void (*ser_dev_on_event_t)(void *ctx, ser_dev_evt_t event,
                                   const ser_dev_t *dev);

/**
 * Obtain a list of devices.
 *
 * @return
 *      Device list (NULL if it could not be obtained)
 *
 * @see
 *      ser_dev_list_destroy
 */
SER_EXPORT ser_dev_list_t *ser_dev_list_get();

/**
 * Destroy a list of devices.
 *
 * @param [in] lst
 *      List of devices.
 *
 * @see
 *      ser_dev_list_get
 */
SER_EXPORT void ser_dev_list_destroy(ser_dev_list_t *lst);

/** Utility macro to iterate over a list of serial devices. */
#define ser_dev_list_foreach(item, lst) \
    for ((item) = (lst); (item) != NULL; (item) = (item)->next)

/**
 * Initialize the serial devices monitor.
 *
 * @param [in] on_event
 *      Callback function that will be called when a new serial device is
 *      connected or disconnected.
 * @param [in] ctx
 *      Context that will be passed to the callback function when called
 *      (optional).
 *
 * @return
 *      An instance of the device monitor (NULL if it could not be initialized).
 *
 * @see
 *      ser_dev_monitor_stop
 */
SER_EXPORT ser_dev_mon_t *ser_dev_monitor_init(ser_dev_on_event_t on_event,
                                               void *ctx);

/**
 * Stop the serial devices monitor.
 *
 * @param [in] mon
 *      Monitor instance.
 */
SER_EXPORT void ser_dev_monitor_stop(ser_dev_mon_t *mon);

/** @} */

SER_END_DECL

#endif
