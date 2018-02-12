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

#include <Windows.h>

#include <string.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>
#include <dbt.h>

#include "sercomm/win/err.h"
#include "sercomm/err.h"

/*
 * References:
 *     - "Registering for Device Notification": https://msdn.microsoft.com/
 *        en-us/library/windows/desktop/aa363432(v=vs.85).aspx
 *     - "Using Windows Classes": https://msdn.microsoft.com/en-us/library/
 *        windows/desktop/ms633575(v=vs.85).aspx/
 *     - "Detecting Hardware Insertion and/or Removal":
 *        http://www.codeproject.com/Articles/14500/
 *        Detecting-Hardware-Insertion-and-or-Removal
 */

/*******************************************************************************
 * Private
 ******************************************************************************/

/** Dummy window class name. */
#define WND_CLASS_NAME "DeviceMonitor"

/** Buffer size for registry queries. */
#define REG_BUF_SZ      SER_DEV_PATH_SZ

/** Timeout for device monitor initialization (ms) */
#define DEV_MON_INIT_TIMEOUT    1000

/** Serial device monitor structure (Windows). */
struct ser_dev_mon
{
    /* Device monitor thread. */
    HANDLE td;
    /* Device monitor thread ID. */
    DWORD td_id;
    /** Initialization finished event */
    HANDLE init;
    /** Window class registered flag */
    int class_reg;
    /** Callback */
    ser_dev_on_event_t on_event;
    /** Callback context */
    void *ctx;
};

/**
 * Obtain device properties from a device broadcast device interface.
 *
 * @param [out] dev
 *     Device structure where device properties will be stored.
 * @param [in,out] bdintf
 *     Device broadcast device interface (NOTE: dbcc_name is modified).
 */
static void dev_properties_get_from_dev_bcast_devintf(
    ser_dev_t *dev,
    PDEV_BROADCAST_DEVICEINTERFACE bdintf)
{
    HDEVINFO dev_info_set;
    SP_DEVINFO_DATA dev_info_data;
    char *pos;

    memset(dev, 0, sizeof(*dev));

    /* prepare enumerator for 'SetupDiGetClassDevs', i.e. convert
     *     \\?\USB#VID_0000&PID_0000#00...#{00000000-0000-0000...}
     * to:
     *     \\?\USB\VID_0000&PID_0000\00...
     */
    pos = strstr(bdintf->dbcc_name, "{");
    if (pos != NULL)
    {
        pos[-1] = '\0';
    }

    do
    {
        pos = strstr(bdintf->dbcc_name, "#");
        if (pos != NULL)
        {
            *pos = '\\';
        }
    } while (pos != NULL);

    /* create a set for the device enumerator */
    dev_info_set = SetupDiGetClassDevs(
        NULL,
        &bdintf->dbcc_name[4],
        NULL,
        DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);

    /* if the enumerator is valid, obtain the port name from the registry */
    dev_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

    if (SetupDiEnumDeviceInfo(
        dev_info_set,
        0,
        &dev_info_data) == TRUE)
    {

        HKEY port_name_hkey;
        DWORD buf_sz;

        port_name_hkey = SetupDiOpenDevRegKey(
            dev_info_set,
            &dev_info_data,
            DICS_FLAG_GLOBAL,
            0,
            DIREG_DEV,
            KEY_READ);

        buf_sz = sizeof(dev->path);

        RegQueryValueEx(
            port_name_hkey,
            "PortName",
            NULL,
            NULL,
            (LPBYTE)dev->path,
            &buf_sz);

        RegCloseKey(port_name_hkey);
    }

    SetupDiDestroyDeviceInfoList(dev_info_set);

    /* vendor ID */
    pos = strstr(bdintf->dbcc_name, "VID_");
    if (pos != NULL)
    {
        dev->vid = (uint16_t)strtoul(&pos[4], NULL, 16);
    }

    /* product ID */
    pos = strstr(bdintf->dbcc_name, "PID_");
    if (pos != NULL)
    {
        dev->pid = (uint16_t)strtoul(&pos[4], NULL, 16);
    }

}

/**
 * Callback function for handling Window messages.
 *
 * @param [in] hWnd
 *     Window handle.
 * @param [in] message
 *     The received message.
 * @param [in] wParam
 *     Extended information provided by the sender.
 * @param [in] lParam
 *     Extended information provided by the sender.
 */
static LRESULT CALLBACK dev_mon_win_msg_proc(HWND hWnd, UINT message,
                                             WPARAM wParam, LPARAM lParam)
{
    LRESULT lr = 0;

    if (message == WM_DEVICECHANGE)
    {
        PDEV_BROADCAST_DEVICEINTERFACE bdintf =
            (PDEV_BROADCAST_DEVICEINTERFACE)lParam;

        if (bdintf != NULL)
        {
            if (bdintf->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
            {
                ser_dev_evt_t evt = SER_DEV_EVT_ADDED;
                int event_supported = 1;

                switch (wParam)
                {
                case DBT_DEVICEARRIVAL:
                    evt = SER_DEV_EVT_ADDED;
                    break;
                case DBT_DEVICEREMOVECOMPLETE:
                    evt = SER_DEV_EVT_REMOVED;
                    break;
                default:
                    event_supported = 0;
                }

                if (event_supported == 1)
                {
                    ser_dev_mon_t *mon;
                    ser_dev_t dev;

                    dev_properties_get_from_dev_bcast_devintf(&dev, bdintf);

                    mon = GetProp(hWnd, "mon");
                    mon->on_event(mon->ctx, evt, &dev);
                }
            }
        }
    }
    else
    {
        lr = DefWindowProc(hWnd, message, wParam, lParam);
    }

    return lr;
}

/**
 * Device monitor thread.
 *
 * @param [in] lpParam
 *      Arguments (monitor instance, ser_dev_mon_t *).
 */
static DWORD WINAPI ser_dev_monitor(LPVOID lpParam)
{
    ser_dev_mon_t *mon = lpParam;

    HWND wnd;
    HDEVNOTIFY dev_notifier;
    DEV_BROADCAST_DEVICEINTERFACE dev_filter;
    MSG msg;

    /* create dummy window (required to capture device change events) */
    wnd = CreateWindow(WND_CLASS_NAME, NULL, 0, 0, 0, 0, 0, NULL, NULL,
                       GetModuleHandle(NULL), NULL);
    if (wnd == NULL)
    {
        werr_set();
        goto out;
    }

    /* attach the monitor instance to the Window */
    if (SetProp(wnd, "mon", mon) == FALSE)
    {
        werr_set();
        goto cleanup_wnd;
    }

    /* setup device notification */
    ZeroMemory(&dev_filter, sizeof(dev_filter));

    dev_filter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    dev_filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    dev_filter.dbcc_classguid = GUID_DEVINTERFACE_COMPORT;

    dev_notifier = RegisterDeviceNotification(wnd, &dev_filter,
                                              DEVICE_NOTIFY_WINDOW_HANDLE);
    if (dev_notifier == NULL)
    {
        werr_set();
        goto cleanup_wnd;
    }

    /* signal successful initialization */
    SetEvent(mon->init);

    /* get and dispatch messages (until WM_QUIT) */
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterDeviceNotification(dev_notifier);
    RemoveProp(wnd, "mon");

cleanup_wnd:
    DestroyWindow(wnd);

out:
    return 0;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

ser_dev_list_t *ser_dev_list_get()
{
    ser_dev_list_t *lst = NULL;
    ser_dev_list_t *prev;

    HDEVINFO dev_info_set;
    SP_DEVINFO_DATA dev_info_data;
    DWORD dev_info_set_index;

    /* create set for ports (includes COM and LPT) */
    dev_info_set = SetupDiGetClassDevs(
        &GUID_DEVCLASS_PORTS,
        NULL,
        NULL,
        DIGCF_PRESENT);


    /* iterate over all available devices in the ports set */
    dev_info_set_index = 0;
    dev_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

    while (SetupDiEnumDeviceInfo(
        dev_info_set,
        dev_info_set_index,
        &dev_info_data) == TRUE)
    {

        HKEY port_name_hkey;
        char buf[REG_BUF_SZ];
        DWORD buf_sz;

        /* port name */
        port_name_hkey = SetupDiOpenDevRegKey(
            dev_info_set,
            &dev_info_data,
            DICS_FLAG_GLOBAL,
            0,
            DIREG_DEV,
            KEY_READ);

        buf_sz = sizeof(buf);

        if (RegQueryValueEx(
            port_name_hkey,
            "PortName",
            NULL,
            NULL,
            (LPBYTE)buf,
            &buf_sz) == EXIT_SUCCESS)
        {
            /* only COM ports, skip LPT */
            if (strcmp(buf, "LPT") != 0)
            {
                BOOL has_hw_id;

                /* allocate new list entry */
                prev = lst;
                lst = calloc(1U, sizeof(*lst));
                if (lst == NULL)
                {
                    sererr_set("%s", strerror(errno));
                    ser_dev_list_destroy(prev);
                    RegCloseKey(port_name_hkey);
                    goto cleanup_dev_info_set;
                }
                lst->next = prev;

                /* store port name */
                strncpy(lst->dev.path, buf, sizeof(lst->dev.path));

                /* hardware id (for USB devices) */
                has_hw_id = SetupDiGetDeviceRegistryProperty(
                    dev_info_set,
                    &dev_info_data,
                    SPDRP_HARDWAREID,
                    NULL,
                    (PBYTE)buf,
                    sizeof(buf),
                    &buf_sz);

                if ((has_hw_id == TRUE) && (buf_sz > 0))
                {
                    char *pos;

                    /* vendor ID */
                    pos = strstr(buf, "VID_");
                    if (pos != NULL)
                    {
                        lst->dev.vid = (uint16_t)strtoul(&pos[4], NULL, 16);
                    }

                    /* product ID */
                    pos = strstr(buf, "PID_");
                    if (pos != NULL)
                    {
                        lst->dev.pid = (uint16_t)strtoul(&pos[4], NULL, 16);
                    }
                }
            }
        }

        RegCloseKey(port_name_hkey);

        dev_info_set_index++;
    }

    if (lst == NULL)
    {
        sererr_set("No devices found");
    }

cleanup_dev_info_set:
    SetupDiDestroyDeviceInfoList(dev_info_set);

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
    WNDCLASS wndc;
    HINSTANCE inst;
    int initialized = 0;
    DWORD wr;

    /* allocate monitor resources */
    mon = calloc(1U, sizeof(*mon));
    if (mon == NULL)
    {
        sererr_set("%s", strerror(errno));
        goto out;
    }

    mon->on_event = on_event;
    mon->ctx = ctx;

    /* register dummy window class (required for device events) */
    inst = GetModuleHandle(NULL);

    wndc.style = 0;
    wndc.lpfnWndProc = dev_mon_win_msg_proc;
    wndc.cbClsExtra = 0;
    wndc.cbWndExtra = 0;
    wndc.hInstance = inst;
    wndc.hIcon = NULL;
    wndc.hCursor = NULL;
    wndc.hbrBackground = NULL;
    wndc.lpszMenuName = NULL;
    wndc.lpszClassName = WND_CLASS_NAME;

    if (RegisterClass(&wndc) == 0)
    {
        wr = GetLastError();
        if (wr == 0)
        {
            mon->class_reg = 1;
        }
        else if (wr == ERROR_CLASS_ALREADY_EXISTS)
        {
            mon->class_reg = 0;
        }
        else
        {
            werr_setc(wr);
            goto out;
        }
    }

    /* create initialization synchronization objects */
    mon->init = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (mon->init == NULL)
    {
        werr_set();
        goto cleanup_wndc;
    }

    /* create device monitoring thread, wait for initialization */
    mon->td = CreateThread(NULL, 0, ser_dev_monitor, mon, 0, &mon->td_id);
    if (mon->td == NULL)
    {
        werr_set();
        goto cleanup_init;
    }

    /* wait until initialized */
    wr = WaitForSingleObject(mon->init, DEV_MON_INIT_TIMEOUT);
    if (wr != WAIT_OBJECT_0)
    {
        if (wr != WAIT_TIMEOUT)
        {
            werr_set();
        }

        /* force termination */
        TerminateThread(mon->td, 0);
        goto cleanup_td;
    }

    initialized = 1;

    goto cleanup_init;

cleanup_td:
    CloseHandle(mon->td);

cleanup_init:
    CloseHandle(mon->init);

cleanup_wndc:
    if (mon->class_reg)
    {
        UnregisterClass(WND_CLASS_NAME, inst);
    }

out:
    if (initialized == 0)
    {
        free(mon);
        mon = NULL;
    }

    return mon;
}

void ser_dev_monitor_stop(ser_dev_mon_t *mon)
{
    PostThreadMessage(mon->td_id, WM_QUIT, 0, 0);

    WaitForSingleObject(mon->td, INFINITE);
    CloseHandle(mon->td);

    if (mon->class_reg)
    {
        UnregisterClass(WND_CLASS_NAME, GetModuleHandle(NULL));
    }

    free(mon);
}
