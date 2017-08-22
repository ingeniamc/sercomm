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

#include "public/sercomm/dev.h"

#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <pthread.h>
#include <libudev.h>

#include "sercomm/err.h"
#include "sercomm/posix/time.h"

/*
 * References:
 *      - "libudev and Sysfs Tutorial", http://www.signal11.us/oss/udev/
 */

/*******************************************************************************
 * Private
 ******************************************************************************/

/** Timeout for device monitor initialization (s) */
#define DEV_MON_INIT_TIMEOUT    1

/** Serial device monitor structure (Linux). */
struct ser_dev_mon
{
    /** Monitoring thread */
    pthread_t td;
    /** Initialization synchronization mutex */
    pthread_mutex_t init_m;
    /** Initialized finished condition */
    pthread_cond_t init_cond;
    /** Callback */
    ser_dev_on_event_t on_event;
    /** Callback context */
    void *ctx;
    /** Termination pipe */
    int pfd[2];
};

/**
 * Check if a udev list entry is a device.
 *
 * @note
 *      A sysfs path is from a device if it contains the 'device' folder.
 *
 * @param [in] uentry
 *      Libudev list entry
 *
 * @return
 *      true if the entry is from a device, false otherwise.
 */
static bool udev_list_entry_is_device(struct udev_list_entry *uentry)
{
    bool is_device = false;
    const char *sysfs_path;
    char *full_path;

    /* obtain sysfs path */
    sysfs_path = udev_list_entry_get_name(uentry);

    /* allocate space for sysfs path + '/devices' */
    full_path = malloc(PATH_MAX);
    if (full_path == NULL)
    {
        sererr_set("%s", strerror(errno));
    }
    else
    {
        DIR *dir;

        /* append '/device' to the given path */
        strncpy(full_path, sysfs_path, PATH_MAX);
        strncat(full_path, "/device", PATH_MAX);

        /* try to open the directory */
        dir = opendir(full_path);
        if (dir != NULL)
        {
            is_device = true;
            closedir(dir);
        }

        free(full_path);
    }

    return is_device;
}

/**
 * Obtain device properties from libudev device.
 *
 * @param [out] dev
 *      libsercomm device.
 * @param [in] udev
 *      libudev device.
 */
static void dev_properties_get_from_udev(ser_dev_t *dev,
                                         struct udev_device *udev)
{
    const char *node;
    const char *vid;
    const char *pid;

    memset(dev, 0, sizeof(*dev));

    /* node */
    node = udev_device_get_devnode(udev);
    if (node != NULL)
    {
        strncpy(dev->path, node, sizeof(dev->path));
    }

    /* vendor ID (for USB devices) */
    vid = udev_device_get_property_value(udev, "ID_VENDOR_ID");
    if (vid != NULL)
    {
        dev->vid = (uint16_t)strtoul(vid, NULL, 16);
    }

    /* product ID (for USB devices) */
    pid = udev_device_get_property_value(udev, "ID_MODEL_ID");
    if (pid != NULL)
    {
        dev->pid = (uint16_t)strtoul(pid, NULL, 16);
    }
}

/**
 * Device monitor thread.
 *
 * @param [in] args
 *      Arguments (monitor instance, ser_dev_mon_t *).
 */
static void *ser_dev_monitor(void *args)
{
    ser_dev_mon_t *mon = args;

    struct udev *uinst;
    struct udev_monitor *umon;
    int ufd;
    int maxfd;
    bool stop;

    /* create udev instance */
    uinst = udev_new();
    if (uinst == NULL)
    {
        sererr_set("%s", strerror(errno));
        goto out;
    }

    /* create udev monitor, configure it to match tty subsystem */
    umon = udev_monitor_new_from_netlink(uinst, "udev");
    if (umon == NULL)
    {
        sererr_set("%s", strerror(errno));
        goto cleanup_uinst;
    }

    udev_monitor_filter_add_match_subsystem_devtype(umon, "tty", NULL);
    udev_monitor_enable_receiving(umon);

    ufd = udev_monitor_get_fd(umon);

    /* obtain maximum file descriptor of udev, term. pipe (for select) */
    if (ufd > mon->pfd[0])
    {
        maxfd = ufd;
    }
    else
    {
        maxfd = mon->pfd[0];
    }

    /* signal successful initialization */
    pthread_mutex_lock(&mon->init_m);
    pthread_cond_signal(&mon->init_cond);
    pthread_mutex_unlock(&mon->init_m);

    /* watch for added/removed devices */
    stop = false;
    while (stop == false)
    {
        fd_set fds;
        int s;

        FD_ZERO(&fds);
        FD_SET(ufd, &fds);
        FD_SET(mon->pfd[0], &fds);

        s = select(maxfd + 1, &fds, NULL, NULL, NULL);

        if (s <= 0)
        {
            sererr_set("%s", strerror(errno));
            goto cleanup_umon;
        }
        else
        {
            /* we must terminate */
            if (FD_ISSET(mon->pfd[0], &fds) != 0)
            {
                stop = true;
            }
            /* a device was added/removed */
            else
            {
                struct udev_device *udev;

                udev = udev_monitor_receive_device(umon);
                if (udev != NULL)
                {
                    const char *action;
                    bool action_supported = true;
                    ser_dev_evt_t evt;
                    ser_dev_t dev;

                    action = udev_device_get_action(udev);
                    if (strcmp(action, "add") == 0)
                    {
                        evt = SER_DEV_EVT_ADDED;
                    }
                    else if (strcmp(action, "remove") == 0)
                    {
                        evt = SER_DEV_EVT_REMOVED;
                    }
                    else
                    {
                        action_supported = false;
                    }

                    if (action_supported == true)
                    {
                        /* obtain device properties, call */
                        dev_properties_get_from_udev(&dev, udev);
                        mon->on_event(mon->ctx, evt, &dev);
                    }

                    udev_device_unref(udev);
                }
            }
        }
    }

cleanup_umon:
    udev_monitor_unref(umon);

cleanup_uinst:
    udev_unref(uinst);

out:
    close(mon->pfd[0]);
    return NULL;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

ser_dev_list_t *ser_dev_list_get()
{
    ser_dev_list_t *lst = NULL;
    ser_dev_list_t *prev;

    struct udev *uinst;
    struct udev_enumerate *uenum;
    struct udev_list_entry *uentries;
    struct udev_list_entry *uentry;

    /* create libudev instance */
    uinst = udev_new();
    if (uinst == NULL)
    {
        sererr_set("%s", strerror(errno));
        goto out;
    }

    /* create libudev enumerate */
    uenum = udev_enumerate_new(uinst);
    if (uenum == NULL)
    {
        sererr_set("%s", strerror(errno));
        goto cleanup_uinst;
    }

    /* scan for devices in the 'tty' subsystem */
    udev_enumerate_add_match_subsystem(uenum, "tty");
    udev_enumerate_scan_devices(uenum);
    uentries = udev_enumerate_get_list_entry(uenum);

    /* iterate over all matches, only process devices */
    udev_list_entry_foreach(uentry, uentries)
    {
        if (udev_list_entry_is_device(uentry) == true)
        {
            const char *path;
            struct udev_device *udev;

            /* allocate new list entry */
            prev = lst;
            lst = calloc(1U, sizeof(*lst));
            if (lst == NULL)
            {
                sererr_set("%s", strerror(errno));
                ser_dev_list_destroy(prev);
                goto cleanup_uenum;
            }

            /* get device properties */
            path = udev_list_entry_get_name(uentry);
            udev = udev_device_new_from_syspath(uinst, path);

            dev_properties_get_from_udev(&lst->dev, udev);

            lst->next = prev;
            udev_device_unref(udev);
        }
    }

    if (lst == NULL)
    {
        sererr_set("No devices found");
    }

cleanup_uenum:
    udev_enumerate_unref(uenum);

cleanup_uinst:
    udev_unref(uinst);

out:
    return lst;
}

void ser_dev_list_destroy(ser_dev_list_t *lst)
{
    ser_dev_list_t *curr;

    curr = lst;
    while (curr != NULL)
    {
        ser_dev_list_t *tmp;

        tmp = curr->next;
        free(curr);
        curr = tmp;
    }
}

ser_dev_mon_t *ser_dev_monitor_init(ser_dev_on_event_t on_event, void *ctx)
{
    ser_dev_mon_t *mon = NULL;
    bool initialized = false;
    int pr;
    struct timespec timeout;

    /* allocate monitor resources */
    mon = calloc(1U, sizeof(*mon));
    if (mon == NULL)
    {
        sererr_set("%s", strerror(errno));
        goto out;
    }

    /* create synchronization items to verify successful initialization */
    pr = pthread_mutex_init(&mon->init_m, NULL);
    if (pr != 0)
    {
        sererr_set("%s", strerror(pr));
        goto out;
    }

    pr = pthread_cond_init(&mon->init_cond, NULL);
    if (pr != 0)
    {
        sererr_set("%s", strerror(pr));
        goto cleanup_m;
    }

    /* create termination pipe */
    if (pipe(mon->pfd) < 0)
    {
        sererr_set("%s", strerror(errno));
        goto cleanup_pfd;
    }

    /* create device monitor thread, wait for initialization */
    pthread_mutex_lock(&mon->init_m);

    mon->on_event = on_event;
    mon->ctx = ctx;

    pr = pthread_create(&mon->td, NULL, ser_dev_monitor, mon);
    if (pr != 0)
    {
        sererr_set("%s", strerror(pr));
        goto cleanup_init;
    }

    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += DEV_MON_INIT_TIMEOUT;

    pr = pthread_cond_timedwait(&mon->init_cond, &mon->init_m, &timeout);
    if (pr != 0)
    {
        close(mon->pfd[1]);
        pthread_join(mon->td, NULL);
    }
    else
    {
        initialized = true;
    }

cleanup_init:
    pthread_mutex_unlock(&mon->init_m);

cleanup_pfd:
    pthread_cond_destroy(&mon->init_cond);

cleanup_m:
    pthread_mutex_destroy(&mon->init_m);

out:
    if (initialized == false)
    {
        free(mon);
        mon = NULL;
    }

    return mon;
}

void ser_dev_monitor_stop(ser_dev_mon_t *mon)
{
    close(mon->pfd[1]);
    pthread_join(mon->td, NULL);

    free(mon);
}
