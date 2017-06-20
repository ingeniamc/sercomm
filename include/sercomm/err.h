/*
 * Copyright (C) the libsercomm contributors. All rights reserved.
 *
 * This file is part of libsercomm, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifndef SERCOMM_ERR_H_
#define SERCOMM_ERR_H_

#ifdef SER_WITH_ERRDESC
/**
 * Set last error message.
 *
 * @param [in] fmt
 *      Format string.
 * @param [in] ...
 *      Arguments for format specification.
 */
void sererr_set(const char *fmt, ...);
#else
#define sererr_set(fmt, ...) (void)0
#endif

#endif
