/*
 * Copyright (C) the libsercomm contributors. All rights reserved.
 *
 * This file is part of libcancomm, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include "public/sercomm/err.h"
#include "sercomm/err.h"

#ifdef SER_WITH_ERRDESC
#include <stdarg.h>
#include <stdio.h>

/*******************************************************************************
 * Internal
 ******************************************************************************/

/** Declare a variable thread-local. */
#ifdef __GNUC__
# define thread_local __thread
#elif __STDC_VERSION__ >= 201112L
# define thread_local _Thread_local
#elif defined(_MSC_VER)
# define thread_local __declspec(thread)
#else
# define thread_local
# warning Thread Local Storage (TLS) not available. Last error will be global.
#endif

/** Maximum error message size. */
#define ERR_SZ 256U

/** Global error description. */
static thread_local char err_last[ERR_SZ] = "Success";

void sererr_set(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vsnprintf(err_last, sizeof(err_last), fmt, args);
    va_end(args);
}
#endif

/*******************************************************************************
 * Public
 ******************************************************************************/

const char *sererr_last()
{
#ifdef SER_WITH_ERRDESC
    return (const char *)err_last;
#else
    return NULL;
#endif
}