#include "config.h"
#include "options_lorax.h"
#include "packet.h"
#include "lorax.h"
#include "become_daemon.h"

struct OptionsLorax opts;

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

    if(opts.daemon)
    {
        err = become_daemon();
        if(err)
        {
            err_output("lorax: error becoming daemon: %d\n", err);
            goto cleanup;
        }
        info_output("daemon started pid=%ld", getpid());
    }

    if(opts.verbose)
    {
        options_lorax_print(&opts);
    }

    if(opts.systemd)
    {
        if(access(opts.rundir, F_OK))
        {
            if(mkdir(opts.rundir, S_IRUSR | S_IXUSR | S_IWUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
            {
                errno_output("creating directory %s\n", opts.rundir);
            }
        }
        // FIXME
        //chmod("/run/lorax/messages", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if(write_pidfile(opts.pidfile))
        {
            errno_output("unable to write pid file %s\n", opts.pidfile);
        }
    }

    lorax_e32_init(&opts);

    if(lorax_e32_register(&opts))
    {
        err_output("main: unable to register to the e32\n");
        err = 1;
        goto cleanup;
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