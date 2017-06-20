/*
 * Copyright (C) the libsercomm contributors. All rights reserved.
 *
 * This file is part of libsercomm, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include "sercomm/win/err.h"
#include "sercomm/err.h"

/*******************************************************************************
 * Internal
 ******************************************************************************/

void werr_setc(DWORD code)
{
    LPTSTR buf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        code,
        LANG_USER_DEFAULT,
        (LPTSTR)&buf,
        0, NULL);

    sererr_set("windows: %s", buf);

    LocalFree(buf);
}

void werr_set()
{
    werr_setc(GetLastError());
}