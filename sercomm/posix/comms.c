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

#include "public/sercomm/comms.h"

#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>

#ifdef __linux__
# include <linux/serial.h>
#endif

#if defined(__MACH__) && defined(__APPLE__)
# include <AvailabilityMacros.h>
# include <IOKit/serial/ioss.h>
#endif

#include "sercomm/err.h"
#include "sercomm/posix/types.h"
#include "sercomm/posix/time.h"

/*******************************************************************************
 * Private
 ******************************************************************************/

/* ioctl for input queue size (macOS) */
#if defined(__MACH__) && defined(__APPLE__)
# ifndef TIOCINQ
#  define TIOCINQ FIONREAD
# endif
#endif

/** Operation type. */
typedef enum
{
    /** Read */
    SER_OP_RD,
    /** Write */
    SER_OP_WR
} ser_op_t;

/**
 * Convert errno codes to sercomm errors.
 *
 * @param [in] code
 *      Errno code.
 *
 * @returns
 *      sercomm error code.
 */
static int32_t error_set(int code)
{
    int32_t r;

    switch (code)
    {
        case 0:
            r = 0;
            break;
        case ENOENT:
            sererr_set("No such device");
            r = SER_ENODEV;
            break;
        case EBUSY:
            sererr_set("Device is busy");
            r = SER_EBUSY;
            break;
        case EIO:
        case ENXIO:
            sererr_set("Device was disconnected");
            r = SER_EDISCONN;
            break;
        case EAGAIN:
            sererr_set("No bytes available");
            r = SER_EEMPTY;
            break;
        default:
            sererr_set("%s", strerror(errno));
            r = SER_EFAIL;
            break;
    }

    return r;
}

/**
 * Configure port.
 *
 * @param [in] ser
 *      Closed library instance.
 * @param [in] opts
 *      Port options.
 *
 * @return
 *      0 on success, error code otherwise.
 */
static int32_t port_configure(ser_t *ser, const ser_opts_t *opts)
{
    int32_t r = 0;

    struct termios tios;
    bool custom_baudrate = false;
    speed_t speed;

    /* store current attributes */
    if (tcgetattr(ser->fd, &ser->tios_old) < 0)
    {
        r = error_set(errno);
        goto out;
    }

    /* configure: common */
    memset(&tios, 0, sizeof(tios));

    tios.c_cflag = CREAD | CLOCAL;

    /* configure: baudrate */
    switch (opts->baudrate)
    {
#ifdef B0
      case 0:
          speed = B0;
          break;
#endif
#ifdef B50
      case 50:
          speed = B50;
          break;
#endif
#ifdef B75
      case 75:
          speed = B75;
          break;
#endif
#ifdef B110
      case 110:
          speed = B110;
          break;
#endif
#ifdef B134
      case 134:
          speed = B134;
          break;
#endif
#ifdef B150
      case 150:
          speed = B150;
          break;
#endif
#ifdef B200
      case 200:
          speed = B200;
          break;
#endif
#ifdef B300
      case 300:
          speed = B300;
          break;
#endif
#ifdef B600
      case 600:
          speed = B600;
          break;
#endif
#ifdef B1200
      case 1200:
          speed = B1200;
          break;
#endif
#ifdef B1800
      case 1800:
          speed = B1800;
          break;
#endif
#ifdef B2400
      case 2400:
          speed = B2400;
          break;
#endif
#ifdef B4800
      case 4800:
          speed = B4800;
          break;
#endif
#ifdef B7200
      case 7200:
          speed = B7200;
          break;
#endif
#ifdef B9600
      case 9600:
          speed = B9600;
          break;
#endif
#ifdef B14400
      case 14400:
          speed = B14400;
          break;
#endif
#ifdef B19200
      case 19200:
          speed = B19200;
          break;
#endif
#ifdef B28800
      case 28800:
          speed = B28800;
          break;
#endif
#ifdef B57600
      case 57600:
          speed = B57600;
          break;
#endif
#ifdef B76800
      case 76800:
          speed = B76800;
          break;
#endif
#ifdef B38400
      case 38400:
          speed = B38400;
          break;
#endif
#ifdef B115200
      case 115200:
          speed = B115200;
          break;
#endif
#ifdef B128000
      case 128000:
          speed = B128000;
          break;
#endif
#ifdef B153600
      case 153600:
          speed = B153600;
          break;
#endif
#ifdef B230400
      case 230400:
          speed = B230400;
          break;
#endif
#ifdef B256000
      case 256000:
          speed = B256000;
          break;
#endif
#ifdef B460800
      case 460800:
          speed = B460800;
          break;
#endif
#ifdef B576000
      case 576000:
          speed = B576000;
          break;
#endif
#ifdef B921600
      case 921600:
          speed = B921600;
          break;
#endif
#ifdef B1000000
      case 1000000:
          speed = B1000000;
          break;
#endif
#ifdef B1152000
      case 1152000:
          speed = B1152000;
          break;
#endif
#ifdef B1500000
      case 1500000:
          speed = B1500000;
          break;
#endif
#ifdef B2000000
      case 2000000:
          speed = B2000000;
          break;
#endif
#ifdef B2500000
      case 2500000:
          speed = B2500000;
          break;
#endif
#ifdef B3000000
      case 3000000:
          speed = B3000000;
          break;
#endif
#ifdef B3500000
      case 3500000:
          speed = B3500000;
          break;
#endif
#ifdef B4000000
      case 4000000:
          speed = B4000000;
          break;
#endif
      default:
          custom_baudrate = true;
    }

    if (custom_baudrate == false)
    {
        (void)cfsetispeed(&tios, speed);
        (void)cfsetospeed(&tios, speed);
    }
    else
    {
#if defined(__MACH__) && defined(__APPLE__)
        /* Mac OS X (>= 10.4) */
        speed = (speed_t)opts->baudrate;

        if (ioctl(ser->fd, IOSSIOSPEED, &speed, 1) < 0)
        {
            r = error_set(errno);
            goto out;
        }
#elif defined(__linux__)
        /* Linux */
        struct serial_struct lser;

        if (ioctl (ser->fd, TIOCGSERIAL, &lser) < 0)
        {
            r = error_set(errno);
            goto out;
        }

        /* set custom divisor and update flags */
        lser.custom_divisor = lser.baud_base / (int)opts->baudrate;
        lser.flags &= ~ASYNC_SPD_MASK;
        lser.flags |= ASYNC_SPD_CUST;

        if (ioctl (ser->fd, TIOCSSERIAL, &lser) < 0)
        {
            r = error_set(errno);
            goto out;
        }
#else
        sererr_set("Custom baudrates unsupported");
        r = SER_ENOTSUP;
        goto out;
#endif
    }

    /* configure: byte size */
    switch (opts->bytesz)
    {
        case SER_BYTESZ_8:
            tios.c_cflag |= CS8;
            break;
        case SER_BYTESZ_7:
            tios.c_cflag |= CS7;
            break;
        case SER_BYTESZ_6:
            tios.c_cflag |= CS6;
            break;
        case SER_BYTESZ_5:
            tios.c_cflag |= CS5;
            break;
        default:
            sererr_set("Invalid byte size");
            r = SER_EINVAL;
            goto out;
    }

    /* configure: parity */
    switch (opts->parity)
    {
        case SER_PAR_NONE:
            break;
        case SER_PAR_ODD:
            tios.c_cflag |= (PARENB | PARODD);
            break;
        case SER_PAR_EVEN:
            tios.c_cflag |= (PARENB);
            break;
#ifdef CMSPAR
        case SER_PAR_MARK:
            tios.c_cflag |= (PARENB | CMSPAR | PARODD);
            break;
        case SER_PAR_SPACE:
            tios.c_cflag |= (PARENB | CMSPAR);
            break;
#else
        case SER_PAR_MARK:
        case SER_PAR_SPACE:
            sererr_set("Unsupported mark or space parity");
            r = SER_ENOTSUP;
            goto out;
#endif
        default:
            sererr_set("Invalid parity type");
            r = SER_EINVAL;
            goto out;
    }

    /* configure: stop bits */
    switch (opts->stopbits)
    {
        case SER_STOPB_ONE:
            break;
        case SER_STOPB_ONE5:
            sererr_set("Unsupported number of stop bits");
            r = SER_ENOTSUP;
            goto out;
        case SER_STOPB_TWO:
            tios.c_cflag |= CSTOPB;
            break;
        default:
            sererr_set("Invalid number of stop bits");
            r = SER_EINVAL;
            goto out;
    }

    /* configure: timeouts (store values for select) */
    ser->timeouts.rd = (int)opts->timeouts.rd;
    ser->timeouts.wr = (int)opts->timeouts.rd;

    tios.c_cc[VMIN] = 1;
    tios.c_cc[VTIME] = 0;

    /* apply new attributes (after flushing) */
    if (tcsetattr(ser->fd, TCSAFLUSH, &tios) < 0)
    {
        r = error_set(errno);
    }

out:
    return r;
}

/**
 * Restore port initial configuration
 *
 * @param [in] ser
 *      Opened library instance.
 */
void port_restore(ser_t *ser)
{
    (void)tcsetattr(ser->fd, TCSANOW, &ser->tios_old);
}

/**
 * Wait until serial port is ready for operation.
 *
 * @param [in] ser
 *      Opened library instance.
 * @param [in] op
 *      Operation to wait for (read or write).
 * @param [in, out] timeout
 *      Maximum wait time (NOTE: will be updated with remaining time).
 *
 * @return
 *      0 on success, error code otherwise.
 */
static int32_t port_wait_ready(ser_t *ser, ser_op_t op, int *timeout)
{
    int32_t r;

    fd_set fds;
    fd_set *rfds = NULL;
    fd_set *wfds = NULL;

    int s;

    /* setup file descriptor set */
    FD_ZERO(&fds);
    FD_SET(ser->fd, &fds);

    if (op == SER_OP_RD)
    {
        rfds = &fds;
    }
    else
    {
        wfds = &fds;
    }

    /* wait until read or write is available */
    if (*timeout > 0)
    {
        struct timeval tv;
        struct timespec start;
        struct timespec end;
        struct timespec diff;

        tv.tv_sec = *timeout / 1000;
        tv.tv_usec = (*timeout * 1000) % 1000000;

        if (clock_gettime(CLOCK_MONOTONIC, &start) < 0)
        {
            r = error_set(errno);
            goto out;
        }

        s = select(ser->fd + 1, rfds, wfds, NULL, &tv);

        if (clock_gettime(CLOCK_MONOTONIC, &end) < 0)
        {
            r = error_set(errno);
            goto out;
        }

        diff = clock__diff(&end, &start);
        *timeout = (int32_t)((diff.tv_sec * 1000) + (diff.tv_nsec / 1000000));
    }
    else
    {
        s = select(ser->fd + 1, rfds, wfds, NULL, NULL);
    }

    /* check select results */
    if (s > 0)
    {
        r = 0;
    }
    else if (s == 0)
    {
        sererr_set("Operation timed out");
        r = SER_ETIMEDOUT;
    }
    else
    {
        sererr_set("%s", strerror(errno));
        r = SER_EFAIL;
    }

out:
    return r;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

int32_t ser_open(ser_t *ser, const ser_opts_t *opts)
{
    int32_t r = 0;

    /* open port */
    ser->fd = open(opts->port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (ser->fd < 0)
    {
        r = error_set(errno);
        goto out;
    }

    /* configure port */
    r = port_configure(ser, opts);
    if (r < 0)
    {
        goto cleanup;
    }

    goto out;

cleanup:
    close(ser->fd);

out:
    return r;
}

void ser_close(ser_t *ser)
{
    /* restore port settings, then close */
    port_restore(ser);
    close(ser->fd);
}

int32_t ser_flush(ser_t *ser, ser_queue_t queue)
{
    int32_t r = 0;

    int queue_;

    /* map to the corresponding termios queue */
    switch (queue)
    {
        case SER_QUEUE_IN:
            queue_ = TCIFLUSH;
            break;
        case SER_QUEUE_OUT:
            queue_ = TCOFLUSH;
            break;
        case SER_QUEUE_ALL:
            queue_ = TCIOFLUSH;
            break;
        default:
            sererr_set("Invalid queue specified");
            r = SER_EINVAL;
            break;
    }

    /* flush */
    if (r == 0)
    {
        if (tcflush(ser->fd, queue_) < 0)
        {
            r = error_set(errno);
        }
    }

    return r;
}

int32_t ser_available(ser_t *ser, size_t *available)
{
    int32_t r;

    int cinq = 0;

    if (ioctl(ser->fd, TIOCINQ, &cinq) < 0)
    {
        r = error_set(errno);
    }
    else
    {
        *available = (size_t)cinq;
        r = 0;
    }

    return r;
}

int32_t ser_read_wait(ser_t *ser)
{
    int timeout = ser->timeouts.rd;

    /* wait until read is ready */
    return port_wait_ready(ser, SER_OP_RD, &timeout);
}

int32_t ser_read(ser_t *ser, void *buf, size_t sz, size_t *recvd)
{
    int32_t r = 0;

    ssize_t recvd_;

    recvd_ = read(ser->fd, buf, sz);

    if (recvd_ > 0)
    {
        /* optionally store read bytes */
        if (recvd != NULL)
        {
            *recvd = (size_t)recvd_;
        }
    }
    else if (recvd_ == 0)
    {
        r = error_set(EIO);
    }
    else
    {
        r = error_set(errno);
    }

    return r;
}

int32_t ser_write(ser_t *ser, const void *buf, size_t sz, size_t *sent)
{
    int32_t r;

    size_t sent_ = 0;
    const uint8_t *bufc = buf;
    int timeout = ser->timeouts.wr;
    bool stop;

    stop = false;
    while (stop == false)
    {
        /* wait until write is available */
        r = port_wait_ready(ser, SER_OP_WR, &timeout);
        if (r < 0)
        {
            stop = true;
        }
        else
        {
            ssize_t sent_now;

            /* write remaining bytes */
            sent_now = write(ser->fd, bufc + sent_, sz - sent_);

            sent_ += (size_t)sent_now;

            /* write available but no data written: device disconnected */
            if ((sent_now < 1) && (errno != EAGAIN))
            {
                error_set(EIO);
                stop = true;
            }
            else
            {
                /* finished */
                if (sent_ == sz)
                {
                    r = 0;
                    stop = true;
                }
            }
        }
    }

    /* optionally store sent bytes */
    if (sent != NULL)
    {
        *sent = sent_;
    }

    return r;
}
