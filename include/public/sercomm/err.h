/*
 * Copyright (C) the libsercomm contributors. All rights reserved.
 *
 * This file is part of libsercomm, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifndef PUBLIC_SERCOMM_ERR_H_
#define PUBLIC_SERCOMM_ERR_H_

#include "common.h"
#include "types.h"

SER_BEGIN_DECL

/**
 * @file sercomm/err.h
 * @brief Error reporting.
 * @defgroup SER_ERR Error reporting
 * @ingroup SER
 * @{
 */


/**
 * Obtain library last error details.
 *
 * @note
 *     If host target supports thread local storage (TLS) the last error
 *     description is kept on a per-thread basis.
 *
 * @return
 *      Last error details.
 */
SER_EXPORT const char *sererr_last(void);

/** @} */

SER_END_DECL

#endif
