/*
 * Copyright (C) the libsercomm contributors. All rights reserved.
 *
 * This file is part of libsercomm, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifndef PUBLIC_SERCOMM_BASE_H_
#define PUBLIC_SERCOMM_BASE_H_

#include "common.h"
#include "types.h"

SER_BEGIN_DECL

/**
 * @file sercomm/base.h
 * @brief Base functions.
 * @defgroup SER_BASE Base
 * @ingroup SER
 * @{
 */

/**
 * Obtain library version.
 *
 * @return
 *      Library version (e.g. "1.0.0")
 */
SER_EXPORT const char *ser_version(void);

/**
 * Create a new library instance.
 *
 * @return
 *      A new library instance (NULL if it could not be created)
 *
 * @see
 *      ser_destroy
 */
SER_EXPORT ser_t *ser_create(void);

/**
 * Destroy a library instance.
 *
 * @param [in] inst
 *      Valid library instance.
 *
 * @see
 *      ser_create
 */
SER_EXPORT void ser_destroy(ser_t *ser);

/** @} */

SER_END_DECL

#endif
