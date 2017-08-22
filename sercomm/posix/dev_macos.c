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

#include <limits.h>
#include <pthread.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>

#include "sercomm/err.h"
#include "sercomm/posix/time.h"

/*
 * References:
 *      - "Introduction to Device File Access Guide for Serial I/O", Apple
 *        Computer Inc. (2005): https://developer.apple.com/library/content/
 *        documentation/DeviceDrivers/Conceptual/WorkingWSerial/WWSerial_Intro/
 *        WWSerial_Intro.html
 *      - "USB Notification Example", Apple Computer Inc. (2003):
 *        https://github.com/opensource-apple/IOUSBFamily/tree/master/Examples/
 *        Another%20USB%20Notification%20Example
 *
 * FIXME:
 *     - Sometimes the USB properties cannot be obtained when the device is
 *       removed. This is because device parents need to be obtained and
 *       sometimes they have already been removed from the registry tree. A
 *       solution is to keep an internal database of connected devices.
 */

/*******************************************************************************
 * Private
 ******************************************************************************/

/** Timeout for device monitor initialization (s) */
#define DEV_MON_INIT_TIMEOUT    1

/** Serial device monitor structure (macOS). */
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
    /** CoreFoundation run loop */
    CFRunLoopRef rl;
};

/**
 * Obtain device properties from IOKit device.
 *
 * @param [out] dev
 *     libsercomm device.
 * @param [in]
 *      IOKit device.
 */
static void dev_properties_get_from_io(ser_dev_t *dev, io_object_t io_dev)
{
    CFTypeRef cf_path;
    bool is_usb;
    IOReturn ir;
    io_object_t io_dev_curr;
    io_registry_entry_t io_dev_parent;
    io_name_t class;

    memset(dev, 0, sizeof(*dev));

    /* path */
    cf_path  = IORegistryEntryCreateCFProperty(io_dev,
                                               CFSTR(kIOCalloutDeviceKey),
                                               kCFAllocatorDefault, 0);
    if (cf_path != NULL)
    {
        CFStringGetCString(cf_path, dev->path, sizeof(dev->path),
                           kCFStringEncodingASCII);
        CFRelease(cf_path);
    }

    /* check if it is a USB device */
    is_usb = false;
    io_dev_curr = io_dev;
    ir = IOObjectGetClass(io_dev_curr, class);
    while ((is_usb == false) && (ir == kIOReturnSuccess))
    {
        if (strstr(class, "USB") != NULL)
        {
            is_usb = true;
        }
        else
        {
            ir = IORegistryEntryGetParentEntry(io_dev_curr, kIOServicePlane,
                                               &io_dev_parent);
            if (ir == kIOReturnSuccess)
            {
                io_dev_curr = io_dev_parent;
                ir = IOObjectGetClass(io_dev_curr, class);
            }
        }
    }

    /* obtain USB properties */
    if (is_usb == true)
    {
        CFTypeRef cf_prop;

        /* vendor ID */
        cf_prop = IORegistryEntryCreateCFProperty(io_dev_curr,
                                                  CFSTR("idVendor"),
                                                  kCFAllocatorDefault, 0);
        if (cf_prop != NULL)
        {
            CFNumberGetValue(cf_prop, kCFNumberSInt16Type, &dev->vid);
            CFRelease(cf_prop);
        }

        /* product ID */
        cf_prop = IORegistryEntryCreateCFProperty(io_dev_curr,
                                                  CFSTR("idProduct"),
                                                  kCFAllocatorDefault, 0);
        if (cf_prop != NULL)
        {
            CFNumberGetValue(cf_prop, kCFNumberSInt16Type, &dev->pid);
            CFRelease(cf_prop);
        }

        IOObjectRelease(io_dev_parent);
    }
}

/**
 * Device added/removed notifier.
 *
 * @param [in] mon
 *      Monitor instance.
 * @param [in] evt
 *      Event type.
 * @param [in] io_iter
 *      IOKit iterator with added/removed devices.
 */
static void dev_notify(ser_dev_mon_t *mon, ser_dev_evt_t evt,
                       io_iterator_t io_iter)
{
    io_object_t io_dev;

    io_dev = IOIteratorNext(io_iter);
    while (io_dev > 0)
    {
        ser_dev_t dev;

        dev_properties_get_from_io(&dev, io_dev);
        mon->on_event(mon->ctx, evt, &dev);

        IOObjectRelease(io_dev);
        io_dev = IOIteratorNext(io_iter);
    }
}

/**
 * Device added callback
 *
 * @param [in] refcon
 *      Reference context (monitor instance, ser_dev_mon_t *).
 * @param [in] iterator
 *      IOKit iterator with newly added devices.
 */
static void dev_added(void *refcon, io_iterator_t iterator)
{
    dev_notify(refcon, SER_DEV_EVT_ADDED, iterator);
}

/**
 * Device removed callback
 *
 * @param [in] refcon
 *      Reference context (monitor instance, ser_dev_mon_t *).
 * @param [in] iterator
 *      IOKit iterator with newly added devices.
 */
static void dev_removed(void *refcon, io_iterator_t iterator)
{
    dev_notify(refcon, SER_DEV_EVT_REMOVED, iterator);
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

    IONotificationPortRef io_notif_port_add, io_notif_port_rm;
    CFRunLoopSourceRef cf_rl_src_add, cf_rl_src_rm;
    CFMutableDictionaryRef cf_dict;
    IOReturn ir;
    io_iterator_t io_iter_add, io_iter_rm;
    io_object_t io_dev;

    /* obtain CF run loop and matching dict. for serial devices (retain as we
     * are using it twice) */
    mon->rl = CFRunLoopGetCurrent();
    cf_dict = IOServiceMatching(kIOSerialBSDServiceValue);
    CFRetain(cf_dict);

    /* create IOKit notification port (added devices) */
    io_notif_port_add = IONotificationPortCreate(kIOMasterPortDefault);
    if (io_notif_port_add == NULL)
    {
        sererr_set("Could not create IOKit notification port for added devs");
        goto out;
    }

    /* add 'added' notification port run loop event source to our run loop */
    cf_rl_src_add = IONotificationPortGetRunLoopSource(io_notif_port_add);
    CFRunLoopAddSource(mon->rl, cf_rl_src_add, kCFRunLoopDefaultMode);

    /* setup notification for 'device added' */
    ir = IOServiceAddMatchingNotification(io_notif_port_add,
                                          kIOFirstMatchNotification,
                                          cf_dict,
                                          dev_added,
                                          mon,
                                          &io_iter_add);
    if (ir != kIOReturnSuccess)
    {
        sererr_set("Could not add notification for added devs (0x%x)", ir);
        goto cleanup_add;
    }

    /* clear current devices (this will arm 'added' notifications) */
    io_dev = IOIteratorNext(io_iter_add);
    while (io_dev > 0)
    {
        IOObjectRelease(io_dev);
        io_dev = IOIteratorNext(io_iter_add);
    }

    /* create IOKit notification port (removed devices) */
    io_notif_port_rm = IONotificationPortCreate(kIOMasterPortDefault);
    if (io_notif_port_rm == NULL)
    {
        sererr_set("Could not create IOKit notification port for rem. devs");
        goto cleanup_add;
    }

    /* add 'removed' notification port run loop event source to our run loop */
    cf_rl_src_rm = IONotificationPortGetRunLoopSource(io_notif_port_rm);
    CFRunLoopAddSource(mon->rl, cf_rl_src_rm, kCFRunLoopDefaultMode);

    /* setup notification for 'device removed' */
    ir = IOServiceAddMatchingNotification(io_notif_port_rm,
                                          kIOTerminatedNotification,
                                          cf_dict,
                                          dev_removed,
                                          mon,
                                          &io_iter_rm);
    if (ir != kIOReturnSuccess)
    {
        sererr_set("Could not add notification for rem. devs (0x%x)", ir);
        goto cleanup_rm;
    }

    /* clear current devices (this will arm 'removed' notifications) */
    io_dev = IOIteratorNext(io_iter_rm);
    while (io_dev > 0)
    {
        IOObjectRelease(io_dev);
        io_dev = IOIteratorNext(io_iter_rm);
    }

    /* notify about successful initialization and enter CF run loop */
    pthread_mutex_lock(&mon->init_m);
    pthread_cond_signal(&mon->init_cond);
    pthread_mutex_unlock(&mon->init_m);

    CFRunLoopRun();

cleanup_rm:
    CFRunLoopRemoveSource(mon->rl, cf_rl_src_rm, kCFRunLoopDefaultMode);
    IONotificationPortDestroy(io_notif_port_rm);

cleanup_add:
    CFRunLoopRemoveSource(mon->rl, cf_rl_src_add, kCFRunLoopDefaultMode);
    IONotificationPortDestroy(io_notif_port_add);

out:
    return NULL;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

ser_dev_list_t *ser_dev_list_get()
{
    ser_dev_list_t *lst = NULL;
    ser_dev_list_t *prev;

    CFMutableDictionaryRef cf_dict;
    IOReturn ir;
    io_iterator_t io_iter;
    io_object_t io_dev;

    /* obtain matching dictionary for serial devices */
    cf_dict = IOServiceMatching(kIOSerialBSDServiceValue);

    /* obtain the list of matching devices */
    ir = IOServiceGetMatchingServices(kIOMasterPortDefault, cf_dict, &io_iter);
    if (ir != kIOReturnSuccess)
    {
        sererr_set("Could not obtain the list of matching dev. (0x%x)", ir);
        goto out;
    }

    /* iterate over all matching devices */
    io_dev = IOIteratorNext(io_iter);
    while (io_dev > 0)
    {
        /* allocate new list entry */
        prev = lst;
        lst = calloc(1, sizeof(*lst));
        if (lst == NULL)
        {
            sererr_set("%s", strerror(errno));
            ser_dev_list_destroy(prev);
            goto cleanup_io_iter;
        }

        /* get device properties */
        dev_properties_get_from_io(&lst->dev, io_dev);

        lst->next = prev;

        /* release device, get the next one */
        IOObjectRelease(io_dev);
        io_dev = IOIteratorNext(io_iter);
    }

    if (lst == NULL)
    {
        sererr_set("No devices found");
    }

cleanup_io_iter:
    IOObjectRelease(io_iter);

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
        pthread_join(mon->td, NULL);
    }
    else
    {
        initialized = true;
    }

cleanup_init:
    pthread_mutex_unlock(&mon->init_m);
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
    CFRunLoopStop(mon->rl);
    pthread_join(mon->td, NULL);

    free(mon);
}
