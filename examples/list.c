/**
 * @example list.c
 * Device listing.
 */

#include <stdio.h>
#include <sercomm/sercomm.h>

int main(void)
{
    ser_dev_list_t *lst;

    lst = ser_dev_list_get();
    if (lst == NULL)
    {
        printf("Could not get devices: %s\n", sererr_last());
    }
    else
    {
        ser_dev_list_t *item;

        printf("Currently connected devices:\n");
        ser_dev_list_foreach(item, lst)
        {
            printf("\t%s", item->dev.path);

            if (item->dev.vid != 0)
            {
                printf(", 0x%04x:", item->dev.vid);
            }

            if (item->dev.pid != 0)
            {
                printf("0x%04x", item->dev.pid);
            }

            printf("\n");
        }

        ser_dev_list_destroy(lst);
    }

    return 0;
}
