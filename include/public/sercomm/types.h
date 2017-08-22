/*
 * Copyright (C) the libsercomm contributors. All rights reserved.
 *
 * This file is part of libsercomm, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifndef PUBLIC_SERCOMM_TYPES_H_
#define PUBLIC_SERCOMM_TYPES_H_

#include <stdlib.h>
#include <stdint.h>

SER_BEGIN_DECL

/**
 * @file sercomm/types.h
 * @brief Base types.
 * @ingroup SER
 * @{
 */

/** Library instance. */
typedef struct ser ser_t;

/*
 * Library error codes.
 */

/** General failure. */
#define SER_EFAIL       -1
/** Invalid input values. */
#define SER_EINVAL      -2
/** Device was disconnected. */
#define SER_EDISCONN    -3
/** Operation timed out. */
#define SER_ETIMEDOUT   -4
/** No such device. */
#define SER_ENODEV      -5
/** Device is busy. */
#define SER_EBUSY       -6
/** Feature not supported. */
#define SER_ENOTSUP     -7
/** Buffer empty. */
#define SER_EEMPTY     -8

/** @} */

SER_END_DECL

#endif
