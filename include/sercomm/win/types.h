/*
 * Copyright (C) the libsercomm contributors. All rights reserved.
 *
 * This file is part of libsercomm, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifndef SERCOMM_WIN_TYPES_H_
#define SERCOMM_WIN_TYPES_H_

#include <Windows.h>

/** Library instance (Windows). */
struct ser
{
    /** Serial port handle */
    HANDLE hnd;
    /** Old port settings */
    DCB dcb_old;
    /** Old port timeouts */
    COMMTIMEOUTS timeouts_old;
    /** Timeouts */
    struct
    {
        /** Read */
        DWORD rd;
        /** Write */
        DWORD wr;
    } timeouts;
};

#endif
