/**
 * @example watcher.c
 * Print all incoming data until no more bytes are received whithin the
 * given timeout.
 */

#include <stdio.h>
#include <sercomm/sercomm.h>

int32_t run(const char *port, uint32_t baudrate, int32_t timeout)
{
    int32_t r = 0;

    ser_t *ser;
    ser_opts_t opts = SER_OPTS_INIT;

    char c;

    /* create instance */
    ser = ser_create();
    if (ser == NULL)
    {
        fprintf(stderr, "Could not create library instance: %s\n",
                sererr_last());
        r = 1;
        goto out;
    }

    /* open port (using default, 8N1) */
    opts.port = port;
    opts.baudrate = baudrate;
    opts.timeouts.rd = timeout;

    r = ser_open(ser, &opts);
    if (r < 0)
    {
        fprintf(stderr, "Could not open port: %s\n", sererr_last());
        goto cleanup_ser;
    }

    /* read until no more bytes are received whithin the given timeout */
    while (1)
    {
        r = ser_read(ser, &c, sizeof(c), NULL);
        if (r == SER_EEMPTY)
        {
            r = ser_read_wait(ser);
            if (r == SER_ETIMEDOUT)
            {
                break;
            }
            else if (r < 0)
            {
                fprintf(stderr, "Error while waiting: %s\n", sererr_last());
                goto cleanup_close;
            }
        }
        else if (r < 0)
        {
            fprintf(stderr, "Could not read: %s\n", sererr_last());
            goto cleanup_close;
        }
        else
        {
            printf("%c", c);
            fflush(stdout);
        }
    }

    printf("\nDone!\n");

cleanup_close:
    ser_close(ser);

cleanup_ser:
    ser_destroy(ser);

out:
    return r;
}

int main(int argc, char *argv[])
{
    char *port;
    uint32_t baudrate;
    int32_t timeout = 1000;

    /* parse options */
    if (argc < 3)
    {
        fprintf(stderr, "Usage: listen PORT BAUDRATE [TIMEOUT]\n");
        return 1;
    }

    port = argv[1];
    baudrate = (uint32_t)strtoul(argv[2], NULL, 0);

    if (argc == 4)
    {
        timeout = (int32_t)strtol(argv[3], NULL, 0);
    }

    return (int)run(port, baudrate, timeout);
}
