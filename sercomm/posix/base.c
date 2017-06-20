/*
 * Copyright (C) the libsercomm contributors. All rights reserved.
 *
 * This file is part of libsercomm, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include "public/sercomm/base.h"

#include <string.h>
#include <errno.h>

#include "sercomm/err.h"
#include "sercomm/posix/types.h"

/*******************************************************************************
 * Public
 ******************************************************************************/

ser_t *ser_create()
{
    ser_t *ser;

    ser = calloc(1U, sizeof(*ser));
    if (ser == NULL)
    {
        sererr_set("%s", strerror(errno));
    }

    return ser;
}

void ser_destroy(ser_t *ser)
{
    free(ser);
}
