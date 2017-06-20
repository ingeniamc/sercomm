/*
 * Copyright (C) the libsercomm contributors. All rights reserved.
 *
 * This file is part of libsercomm, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifndef INCLUDE_SERCOMM_COMMON_H_
#define INCLUDE_SERCOMM_COMMON_H_

#include "config.h"

/**
 * @file sercomm/common.h
 * @brief Common header.
 * @ingroup SER
 * @{
 */

#ifdef __cplusplus
/** Start declaration in C++ mode. */
# define SER_BEGIN_DECL extern "C" {
/** End declaration in C++ mode. */
# define SER_END_DECL }
#else
/** Start declaration in C mode. */
# define SER_BEGIN_DECL
/** End declaration in C mode. */
# define SER_END_DECL
#endif

/** Declare a public function exported for application use. */
#ifndef SER_STATIC
# if defined(SER_BUILDING) && __GNUC__ >= 4
#  define SER_EXPORT __attribute__((visibility("default")))
# elif defined(SER_BUILDING) && defined(_MSC_VER)
#  define SER_EXPORT __declspec(dllexport)
# elif defined(_MSC_VER)
#  define SER_EXPORT __declspec(dllimport)
# else
#  define SER_EXPORT
# endif
#else
# define SER_EXPORT
#endif

/** @} */

#endif
