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

#include <string.h>
#include <errno.h>

#include "sercomm/err.h"
#include "sercomm/win/err.h"
#include "sercomm/win/types.h"

 /*
  * References:
  *     - "Serial Communications", Allen Denver, Microsoft Windows Developer
  *       Support (1995): https://msdn.microsoft.com/en-us/library/ff802693.aspx
  */

/*******************************************************************************
 * Private
 ******************************************************************************/

/** Port prefix */
#define PORT_PREFIX             "\\\\.\\"

/**
 * Map Windows error to sercomm error and set last error accordingly.
 *
 * @param [in] wr
 *     Pointer to error code (optional)
 *
 * @return
 *     Matching sercomm error code.
 */
int32_t werr(const DWORD *wr)
{
    DWORD wr_;

    if (wr == NULL)
    {
        wr_ = GetLastError();
    }
    else
    {
        wr_ = *wr;
    }

    switch (wr_)
    {
        case ERROR_FILE_NOT_FOUND:
            sererr_set("No such device");
            return SER_ENODEV;
        case ERROR_ACCESS_DENIED:
            sererr_set("Device is in use");
            return SER_EBUSY;
        case ERROR_INVALID_PARAMETER:
            sererr_set("Invalid parameter");
            return SER_EINVAL;
        case ERROR_BAD_COMMAND:
        case ERROR_GEN_FAILURE:
        case ERROR_OPERATION_ABORTED:
            sererr_set("Device was disconnected");
            return SER_EDISCONN;
        case WAIT_TIMEOUT:
            sererr_set("Operation timed out");
            return SER_ETIMEDOUT;
        default:
            werr_setc(wr_);
            return SER_EFAIL;
    }
}

/**
 * Configure port.
 *
 * @param [in] ser
 *      Closed library instance.
 * @param [in] opts
 *      Port configuration.
 *
 * @return
 *      0 on success, error code otherwise.
 */
static int32_t port_configure(ser_t *ser, const ser_opts_t *opts)
{
    int32_t r = 0;

    DCB dcb;
    COMMTIMEOUTS timeouts;

    /* store current state, timeouts */
    if (GetCommState(ser->hnd, &ser->dcb_old) == FALSE)
    {
        r = werr(NULL);
        goto out;
    }

    if (GetCommTimeouts(ser->hnd, &ser->timeouts_old) == FALSE)
    {
        r = werr(NULL);
        goto out;
    }

    /* configure port settings */
    memset(&dcb, 0, sizeof(dcb));

    dcb.DCBlength = sizeof(dcb);

    dcb.BaudRate = (DWORD)opts->baudrate;

    switch (opts->bytesz)
    {
        case SER_BYTESZ_8:
            dcb.ByteSize = 8;
            break;
        case SER_BYTESZ_7:
            dcb.ByteSize = 7;
            break;
        case SER_BYTESZ_6:
            dcb.ByteSize = 6;
            break;
        case SER_BYTESZ_5:
            dcb.ByteSize = 5;
            break;
        default:
            sererr_set("Invalid byte size");
            r = SER_EINVAL;
            goto out;
    }

    switch (opts->parity)
    {
        case SER_PAR_NONE:
            dcb.Parity = NOPARITY;
            break;
        case SER_PAR_ODD:
            dcb.Parity = ODDPARITY;
            break;
        case SER_PAR_EVEN:
            dcb.Parity = EVENPARITY;
            break;
        case SER_PAR_MARK:
            dcb.Parity = MARKPARITY;
            break;
        case SER_PAR_SPACE:
            dcb.Parity = SPACEPARITY;
            break;
        default:
            sererr_set("Invalid parity type");
            r = SER_EINVAL;
            goto out;
    }

    switch (opts->stopbits)
    {
        case SER_STOPB_ONE:
            dcb.StopBits = ONESTOPBIT;
            break;
        case SER_STOPB_ONE5:
            dcb.StopBits = ONE5STOPBITS;
            break;
        case SER_STOPB_TWO:
            dcb.StopBits = TWOSTOPBITS;
            break;
        default:
            sererr_set("Invalid number of stop bits");
            r = SER_EINVAL;
            goto out;
    }

    if (SetCommState(ser->hnd, &dcb) == FALSE)
    {
        r = werr(NULL);
        goto out;
    }

    /* configure port timeouts - only using WaitFor..., so 'disable' the Comms
     * timeouts */
    memset(&timeouts, 0, sizeof(timeouts));

    timeouts.ReadIntervalTimeout = MAXDWORD;

    if (SetCommTimeouts(ser->hnd, &timeouts) == FALSE)
    {
        r = werr(NULL);
        goto restore;
    }

    if (opts->timeouts.rd == SER_NO_TIMEOUT)
    {
        ser->timeouts.rd = INFINITE;
    }
    else
    {
        ser->timeouts.rd = (DWORD)opts->timeouts.rd;
    }

    if (opts->timeouts.wr == SER_NO_TIMEOUT)
    {
        ser->timeouts.wr = INFINITE;
    }
    else
    {
        ser->timeouts.wr = (DWORD)opts->timeouts.rd;
    }

    /* purge input buffer */
    (void)PurgeComm(ser->hnd, PURGE_RXCLEAR);

    goto out;

restore:
    SetCommState(ser->hnd, &ser->dcb_old);

out:
    return r;
}

/*******************************************************************************
* Public
******************************************************************************/

int32_t ser_open(ser_t *inst, const ser_opts_t *opts)
{
    int32_t r = 0;
    char *port;

    /* prepend '\\.\' to port name (required for > COM9) */
    port = malloc(sizeof(PORT_PREFIX) + strlen(opts->port));
    if (port == NULL)
    {
        sererr_set("%s", strerror(errno));
        goto out;
    }

    strcpy(port, PORT_PREFIX);
    strcat(port, opts->port);

    /* open port */
    inst->hnd = CreateFile(
        port,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

    free(port);

    if (inst->hnd == INVALID_HANDLE_VALUE)
    {
        r = werr(NULL);
        goto out;
    }

    /* configure port */
    r = port_configure(inst, opts);
    if (r < 0)
    {
        goto cleanup;
    }

    /* subscribe to reception events */
    if (SetCommMask(inst->hnd, EV_RXCHAR) == FALSE)
    {
        r = werr(NULL);
        goto cleanup;
    }

    goto out;

cleanup:
    CloseHandle(inst->hnd);

out:
    return r;
}

void ser_close(ser_t *inst)
{
    /* restore old settings, close port */
    SetCommState(inst->hnd, &inst->dcb_old);
    SetCommTimeouts(inst->hnd, &inst->timeouts_old);

    /* QUIRK: CloseHandle may raise an exception which may lead to crashes
     * when running, for example, from a Python wrapper. This situation has
     * only been observed on some virtual COM ports.
     */
    __try
    {
        CloseHandle(inst->hnd);
    }
    __except (EXCEPTION_CONTINUE_EXECUTION) {}
}

int32_t ser_flush(ser_t *inst, ser_queue_t flush)
{
    DWORD flags;

    /* map to the corresponding PurgeComm flags */
    switch (flush)
    {
        case SER_QUEUE_IN:
            flags = PURGE_RXCLEAR;
            break;
        case SER_QUEUE_OUT:
            flags = PURGE_TXCLEAR;
            break;
        case SER_QUEUE_ALL:
            flags = PURGE_RXCLEAR | PURGE_TXCLEAR;
            break;
        default:
            sererr_set("Invalid queue specified");
            return SER_EINVAL;
    }

    if (PurgeComm(inst->hnd, flags) == FALSE)
    {
        return werr(NULL);
    }

    return 0;
}

int32_t ser_available(ser_t *inst, size_t *available)
{
    int32_t r = 0;
    COMSTAT cs;

    if (ClearCommError(inst->hnd, NULL, &cs) == FALSE)
    {
        r = werr(NULL);
    }
    else
    {
        *available = (size_t)cs.cbInQue;
    }

    return r;
}

int32_t ser_read_wait(ser_t *inst)
{
    int32_t r = 0;

    OVERLAPPED ovs = { 0 };
    DWORD evts_mask;

    /* create event for status change */
    ovs.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ovs.hEvent == NULL)
    {
        r = werr(NULL);
        goto out;
    }

    /* try waiting for events */
    if (WaitCommEvent(inst->hnd, &evts_mask, &ovs) == FALSE)
    {
        DWORD wr;

        wr = GetLastError();

        /* no event was set, wait until some are received */
        if (wr == ERROR_IO_PENDING)
        {
            wr = WaitForSingleObject(ovs.hEvent, inst->timeouts.rd);
            switch (wr)
            {
                case WAIT_OBJECT_0:
                    break;
                default:
                    r = werr(&wr);
                    goto cleanup;
            }
        }
        else
        {
            r = werr(&wr);
            goto cleanup;
        }
    }

    /* obtain event mask, assert RX event is set */
    if (GetCommMask(inst->hnd, &evts_mask) == FALSE)
    {
        r = werr(NULL);
        goto cleanup;
    }

    if ((evts_mask & EV_RXCHAR) == 0)
    {
        sererr_set("Unexpected error (RX event not set)");
        r = SER_EFAIL;
        goto cleanup;
    }

cleanup:
    CloseHandle(ovs.hEvent);

out:
    return r;
}

int32_t ser_read(ser_t *inst, void *buf, size_t sz, size_t *recvd)
{
    int32_t r = 0;

    OVERLAPPED ovr = { 0 };
    DWORD recvd_ = 0;

    /* create event for the read completion */
    ovr.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ovr.hEvent == NULL)
    {
        werr_set();
        r = SER_EFAIL;
        goto out;
    }

    /* try read */
    if (ReadFile(inst->hnd, buf, (DWORD)sz, NULL, &ovr) == FALSE)
    {
        DWORD wr;

        wr = GetLastError();

        /* read pending, wait until completion */
        if (wr == ERROR_IO_PENDING)
        {
            wr = WaitForSingleObject(ovr.hEvent, inst->timeouts.rd);
            switch (wr)
            {
                case WAIT_OBJECT_0:
                    break;
                default:
                    r = werr(&wr);
                    goto cleanup;
            }
        }
        else
        {
            r = werr(&wr);
            goto cleanup;
        }
    }

    /* obtain results */
    if (GetOverlappedResult(inst->hnd, &ovr, &recvd_, FALSE) == FALSE)
    {
        r = werr(NULL);
        goto cleanup;
    }

    /* no more bytes available */
    if (recvd_ == 0)
    {
        sererr_set("No bytes available");
        r = SER_EEMPTY;
    }

    /* optionally store received bytes */
    if (recvd != NULL)
    {
        *recvd = (size_t)recvd_;
    }

cleanup:
    CloseHandle(ovr.hEvent);

out:
    return r;
}

int32_t ser_write(ser_t *inst, const void *buf, size_t sz, size_t *sent)
{
    int32_t r = 0;

    OVERLAPPED ovw = { 0 };
    DWORD sent_ = 0;

    /* create event for the write completion */
    ovw.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ovw.hEvent == NULL)
    {
        r = werr(NULL);
        goto out;
    }

    /* try write */
    if (WriteFile(inst->hnd, buf, (DWORD)sz, &sent_, &ovw) == FALSE)
    {
        DWORD wr;

        wr = GetLastError();
        if (wr == ERROR_IO_PENDING)
        {
            /* write is pending, wait until completion */
            wr = WaitForSingleObject(ovw.hEvent, inst->timeouts.wr);
            switch (wr)
            {
                case WAIT_OBJECT_0:
                    break;
                default:
                    r = werr(&wr);
                    goto cleanup;
            }
        }
        else
        {
            r = werr(&wr);
            goto cleanup;
        }
    }

    if (GetOverlappedResult(inst->hnd, &ovw, &sent_, FALSE) == FALSE)
    {
        r = werr(NULL);
        goto cleanup;
    }

    /* optionally store written bytes */
    if (sent != NULL)
    {
        *sent = (size_t)sent_;
    }

cleanup:
    CloseHandle(ovw.hEvent);

out:
    return r;
}
