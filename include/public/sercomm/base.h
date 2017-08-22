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
