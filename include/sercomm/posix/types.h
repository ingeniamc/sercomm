/*
 * Copyright (C) the libsercomm contributors. All rights reserved.
 *
 * This file is part of libsercomm, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifndef SERCOMM_POSIX_TYPES_H_
#define SERCOMM_POSIX_TYPES_H_

#include <termios.h>

/** Library instance (POSIX). */
struct ser
{
    /** Previous serial port settings (restored on close) */
    struct termios tios_old;
    /** Serial port file descriptor */
    int fd;
    /** Timeouts */
    struct
    {
        /** Read */
        int rd;
        /** Write */
        int wr;
    } timeouts;
};

#endif
