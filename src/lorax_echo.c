#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>

#include "become_daemon.h"
#include "error.h"
#include "message.h"
#include "misc.h"
#include "socket.h"

int use_syslog = 0;

struct Options
{
  int help;
  int verbose;
  int daemon;
  int systemd;
  char rundir[64];
  char pidfile[64];
  char *rx_sock_name;
  struct sockaddr_un rx_sock;
  int rx_sock_fd;
  int receive_fd;
};

void
print_usage(char *progname)
{
    printf("Usage: %s [OPTIONS] listen_socket\n", progname);
    printf("Version: %s\n\n", VERSION);
    printf("A server that listening on a Unix Domain Socket that will echo what was sent.\n\n");

    printf("listen_socket: the full path of the socket to listen on\n");

    printf("OPTIONS:\n\
-h, --help     print these options\n\
-v, --verbose  more log output\n\
-s, --systemd  create a pid file in the /run/lorax directory\n\
-d, --daemon   daemonize the process\n");
}

int
parse_options(struct Options *opts, int argc, char *argv[])
{
    int c;
    int option_index;
    int err;

    memset(opts, 0, sizeof(struct Options));

    err = 0;

    static struct option long_options[] =
    {
        {"help",                     no_argument, 0, 'h'},
        {"verbose",                  no_argument, 0, 'v'},
        {"systemd",                  no_argument, 0, 's'},
        {"daemon",                   no_argument, 0, 'd'},
        {0,                                    0, 0,   0}
    };

    while(1)
    {
        option_index = 0;
        c = getopt_long(argc, argv, "hvsd", long_options, &option_index);

        if(c == -1)
            break;

        switch(c)
        {
        case 0:
            if(strcmp("help", long_options[option_index].name) == 0)
                opts->help = 1;
            if(strcmp("verbose", long_options[option_index].name) == 0)
                opts->verbose = 1;
            if(strcmp("daemon", long_options[option_index].name) == 0)
                opts->daemon = 1;
            if(strcmp("systemd", long_options[option_index].name) == 0)
                opts->systemd = 1;
            break;
        case 'h':
            opts->help = 1;
            break;
        case 'v':
            opts->verbose = 1;
            break;
        case 's':
            opts->systemd = 1;
            break;
        case 'd':
            opts->daemon = 1;
            break;
        }
    }

    if((argc - optind) != 1)
    {
        err_output("not enough input arguments\n");
        return 1;
    }

    opts->rx_sock_name = argv[argc-1];

    sprintf(opts->rundir, "/run/lorax");
    sprintf(opts->pidfile, "%s/echo.pid", opts->rundir);

    if(opts->daemon)
    {
        use_syslog = 1;
        openlog("lorax_echo", 0, LOG_DAEMON);
    }

    return err || opts->help;
}

int echo(struct Options *opts)
{
    uint8_t buf[1024];
    struct sockaddr_un sock_source;
    ssize_t received_bytes;
    if(socket_unix_receive(opts->receive_fd, buf, 1024, &received_bytes, &sock_source))
    {
        return 1;
    }

    info_output("received %d bytes from address %s\n", received_bytes, sock_source.sun_path);
    print_hex(buf, received_bytes);

    if(socket_unix_send(opts->receive_fd, &sock_source, buf, received_bytes))
    {
        return 2;
    }

    info_output("sent %d bytes to address %s\n", received_bytes, sock_source.sun_path);
    print_hex(buf, received_bytes);

    return 0;
}

int
main(int argc, char *argv[])
{
    struct Options opts;
    int err;
    err = 0;

    if(parse_options(&opts, argc, argv))
    {
        print_usage(argv[0]);
        return 1;
    }

    if(opts.daemon)
    {
        err = become_daemon();
        if(err)
        {
            err_output("lorax: error becoming daemon: %d\n", err);
            return 2;
        }
        info_output("daemon started pid=%ld", getpid());
    }

    if(opts.systemd)
    {
        if(access(opts.rundir, F_OK ))
        {
            if(mkdir(opts.rundir, S_IRUSR | S_IXUSR | S_IWUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
            {
                errno_output("creating directory %s\n", opts.rundir);
            }
        }
        if(write_pidfile(opts.pidfile))
        {
            errno_output("unable to write pid file %s\n", opts.pidfile);
        }
    }

    if(socket_unix_bind(opts.rx_sock_name, &opts.receive_fd, &opts.rx_sock))
    {
        err = 2;
    }

    while(err == 0)
    {
        err = echo(&opts);
    }

    return err;
}
