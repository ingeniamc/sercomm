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
