/*
 * Copyright (C) the libsercomm contributors. All rights reserved.
 *
 * This file is part of libsercomm, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifndef SERCOMM_WIN_ERR_H_
#define SERCOMM_WIN_ERR_H_

#include <Windows.h>

 /**
 * Set library last error with Windows details providing error code.
 *
 * @param [in] code
 *      Error code.
 */
void werr_setc(DWORD code);

/**
 * Set library last error with Windows details (from last error).
 */
void werr_set(void);

#endif
