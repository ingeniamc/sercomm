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

#ifndef PUBLIC_SERCOMM_COMMS_H_
#define PUBLIC_SERCOMM_COMMS_H_

#include "common.h"
#include "types.h"

SER_BEGIN_DECL

/**
 * @file sercomm/comms.h
 * @brief Communications.
 * @defgroup SER_COMMS Communications
 * @ingroup SER
 * @{
 */

/** Use for infinite timeout. */
#define SER_NO_TIMEOUT  0

/** Byte sizes. */
typedef enum
{
    /** 8 */
    SER_BYTESZ_8 = 0,
    /** 7 */
    SER_BYTESZ_7,
    /** 6 */
    SER_BYTESZ_6,
    /** 5 */
    SER_BYTESZ_5
} ser_bytesz_t;

/** Parity types. */
typedef enum
{
    /** None */
    SER_PAR_NONE = 0,
    /** Odd */
    SER_PAR_ODD,
    /** Even */
    SER_PAR_EVEN,
    /** Mark */
    SER_PAR_MARK,
    /** Space */
    SER_PAR_SPACE
} ser_parity_t;

/** Stop bits. */
typedef enum
{
    /** 1 */
    SER_STOPB_ONE = 0,
    /** 1.5 */
    SER_STOPB_ONE5,
    /** 2 */
    SER_STOPB_TWO
} ser_stopbits_t;

/** Serial queues. */
typedef enum
{
    /** Input queue */
    SER_QUEUE_IN,
    /** Output queue */
    SER_QUEUE_OUT,
    /** Input&Output queues */
    SER_QUEUE_ALL
} ser_queue_t;

/** Serial port options. */
typedef struct
{
    /** Port name (e.g. COM1, /dev/ttyS0, etc.) */
    const char *port;
    /** Baudrate (e.g. 9600, 115200, ...) */
    uint32_t baudrate;
    /** Byte size */
    ser_bytesz_t bytesz;
    /** Parity */
    ser_parity_t parity;
    /** Stop bits */
    ser_stopbits_t stopbits;
    /** Timeouts (ms) - see #SER_NO_TIMEOUT */
    struct
    {
        /** Read */
        int32_t rd;
        /** Write */
        int32_t wr;
    } timeouts;
} ser_opts_t;

/** Initializer for serial port configuration structure. */
#define SER_OPTS_INIT { NULL, \
                        0, \
                        SER_BYTESZ_8, \
                        SER_PAR_NONE, \
                        SER_STOPB_ONE, \
                        { \
                            0, \
                            0, \
                        } \
                      }

/**
 * Open serial port.
 *
 * @param [in] ser
 *      Unopened library instance.
 * @param [in] opts
 *      Options (name, baudrate, etc.)
 *
 * @return
 *      0 on success, error code otherwise.
 *
 * @see
 *      ser_close
 */
SER_EXPORT int32_t ser_open(ser_t *ser, const ser_opts_t *opts);

/**
 * Close serial port.
 *
 * @param [in] ser
 *      Opened library instance.
 *
 * @see
 *      ser_open
 */
SER_EXPORT void ser_close(ser_t *ser);

/**
 * Flush serial port queue/s.
 *
 * @param [in] ser
 *      Opened library instance.
 * @param [in] queue
 *      Queue to be flushed.
 *
 * @return
 *      0 on success, error code otherwise.
 */
SER_EXPORT int32_t ser_flush(ser_t *ser, ser_queue_t queue);

/**
 * Obtain the available number of bytes ready to be read.
 *
 * @param [in] ser
 *      Opened library instance.
 * @param [out] available
 *      Where number of available bytes will be stored.
 *
 * @return
 *      0 on success, error code otherwise.
 */
SER_EXPORT int32_t ser_available(ser_t *ser, size_t *available);

/**
 * Wait until serial port is ready to be read.
 *
 * @param [in] ser
 *      Opened library instance.
 *
 * @return
 *      0 on success, error code otherwise.
 */
SER_EXPORT int32_t ser_read_wait(ser_t *ser);

/**
 * Read from serial port.
 *
 * @param [in] ser
 *      Opened library instance.
 * @param [out] buf
 *      Output buffer.
 * @param [in] sz
 *      Number of bytes to read.
 * @param [in] recvd
 *      Number of received bytes (optional).
 *
 * @return
 *      0 on success, error code otherwise.
 *
 * @see
 *      ser_write
 */
SER_EXPORT int32_t ser_read(ser_t *ser, void *buf, size_t sz, size_t *recvd);

/**
 * Write to serial port.
 *
 * @param [in] ser
 *      Opened library instance.
 * @param [in] buf
 *      Buffer with the data to be written.
 * @param [in] sz
 *      Number of bytes to write.
 * @param [in] sent
 *      Number of actual written bytes (optional).
 *
 * @return
 *      0 on success, error code otherwise.
 *
 * @see
 *      ser_read
 */
SER_EXPORT int32_t ser_write(ser_t *ser, const void *buf, size_t sz,
                             size_t *sent);

/** @} */

SER_END_DECL

#endif
