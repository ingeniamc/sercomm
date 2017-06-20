/**
 * @example monitor.c
 * Device monitoring.
 */

#include <stdio.h>
#include <sercomm/sercomm.h>

void on_event(void *ctx, ser_dev_evt_t evt, const ser_dev_t *dev)
{
    (void)ctx;

    switch (evt)
    {
        case SER_DEV_EVT_ADDED:
            printf("Device added: ");
            break;
        case SER_DEV_EVT_REMOVED:
            printf("Device removed: ");
            break;
    }

    printf("%s, 0x%04x:0x%04x\n", dev->path, dev->vid, dev->pid);
}

int main(void)
{
    int r = 0;

    ser_dev_mon_t *mon;

    mon = ser_dev_monitor_init(on_event, NULL);
    if (mon == NULL)
    {
        fprintf(stderr, "Could not initialize device monitor: %s\n",
                sererr_last());
        r = 1;
    }
    else
    {
        printf("Press ENTER to stop monitoring\n");
        getchar();

        ser_dev_monitor_stop(mon);
    }

    return r;
}
