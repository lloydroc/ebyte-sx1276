#include "config.h"
#include "options_lorax.h"
#include "packet.h"
#include "lorax.h"
#include "become_daemon.h"

struct OptionsTx opts;

int
main(int argc, char *argv[])
{
    int err;
    options_lorax_init(&opts);
    err = options_lorax_parse(&opts, argc, argv);
    if(err || opts.help)
    {
        options_lorax_usage(argv[0]);
        return err;
    }

    lorax_e32_init(&opts);

    if(lorax_e32_register(&opts))
    {
        err_output("main: unable to register to the e32\n");
        err = 1;
        goto cleanup;
    }

    if(opts.daemon)
    {
        err = become_daemon();
        if(err)
        {
            err_output("lorax: error becoming daemon: %d\n", err);
            goto cleanup;
        }
        if(mkdir("/run/lorax", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))
        {
            return 2;
            errno_output("creating directory /run/lorax\n");
        }
        if(write_pidfile("/run/lorax/lorax.pid"))
        {
            errno_output("unable to write pid file\n");
        }
        info_output("daemon started pid=%ld", getpid());
    }

    if(lorax_e32_poll(&opts))
    {
        err_output("main: poll failed\n");
        err = 2;
        goto cleanup;
    }

cleanup:
    lorax_e32_destroy();
    return 0;
}