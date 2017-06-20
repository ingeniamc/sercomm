/*
 * Copyright (C) the libsercomm contributors. All rights reserved.
 *
 * This file is part of libsercomm, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include "sercomm/posix/time.h"

/*******************************************************************************
 * Internal
 ******************************************************************************/

#ifdef MAC_OS_X_VERSION_MAX_ALLOWED
#if MAC_OS_X_VERSION_MAX_ALLOWED < 101200
#include <errno.h>
#include <mach/clock.h>
#include <mach/mach.h>

int clock_gettime(clockid_t clk_id, struct timespec *res)
{
    int r;

    (void)clk_id;

    if (res == NULL)
    {
        errno = EFAULT;
        r = -1;
    }
    else
    {
        kern_return_t kr;
        clock_serv_t clock;

        kr = host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &clock);
        if (kr != KERN_SUCCESS)
        {
            errno = EIO;
            r = -1;
        }
        else
        {
            mach_timespec_t mts;

            kr = clock_get_time(clock, &mts);
            if (kr != KERN_SUCCESS)
            {
                errno = EIO;
                r = -1;
            }
            else
            {
                (void)mach_port_deallocate(mach_task_self(), clock);

                res->tv_sec = mts.tv_sec;
                res->tv_nsec = mts.tv_nsec;

                r = 0;
            }
        }
    }

    return r;
}
#endif
#endif

struct timespec clock__diff(const struct timespec *a, const struct timespec *b)
{
    struct timespec diff;

    diff.tv_sec = a->tv_sec - b->tv_sec;
    diff.tv_nsec = a->tv_nsec - b->tv_nsec;

    if (diff.tv_nsec < 0)
    {
        diff.tv_sec--;
        diff.tv_nsec += 1000000000L;
    }

    return diff;
}
