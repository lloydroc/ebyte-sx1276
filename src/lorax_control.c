#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>

#include "error.h"
#include "control.h"
#include "misc.h"
#include "socket.h"

int use_syslog = 0;

struct Options
{
  int help;
  int verbose;
  char *txsock;
  char *rxsock;
  int receive_fd;
  int timeout_ms;

  char *command;
};

const char *
get_home_directory()
{
    const char * homedir;
    if ((homedir = getenv("HOME")) == NULL)
    {
        homedir = getpwuid(getuid())->pw_dir;
    }
    return homedir;
}

void
print_usage(char *progname)
{
    const char *homedir;

    homedir = get_home_directory();
    printf("Usage: %s [OPTIONS] COMMAND\n", progname);
    printf("Version: %s\n\n", VERSION);
    printf("A client to send control message to the lorax process. For example asking for the number of neighbors.\n\n");

    printf("COMMAND:\n\
myaddress\n\
neighbors\n\n");

    printf("OPTIONS:\n\
-h, --help\n\
-v, --verbose\n\
-t, --txsock [/run/lorax/control] lorax socket for control\n\
-r, --rxsock [%s/client.control] socket lorax will send the control message back to\n\
-T, --timeout [30000] how long wait for a response on milliseconds\n\
-s, --source-address TODO\n\
    ", homedir);
}

int
parse_options(struct Options *opts, int argc, char *argv[])
{
    int c;
    int option_index;
    int err;
    const char *homedir;

    memset(opts, 0, sizeof(struct Options));

    err = 0;
    opts->timeout_ms = 30000;

    static struct option long_options[] =
    {
        {"help",                     no_argument, 0, 'h'},
        {"verbose",                  no_argument, 0, 'v'},
        {"txsock",             required_argument, 0, 't'},
        {"rxsock",             required_argument, 0, 'r'},
        {"timeout",            required_argument, 0, 'T'},
        {0,                                    0, 0,   0}
    };

    while(1)
    {
        option_index = 0;
        c = getopt_long(argc, argv, "hvt:r:T:", long_options, &option_index);

        if(c == -1)
            break;

        switch(c)
        {
        case 0:
            if(strcmp("help", long_options[option_index].name) == 0)
                opts->help = 1;
            if(strcmp("verbose", long_options[option_index].name) == 0)
                opts->verbose = 1;
            else if(strcmp("txsock", long_options[option_index].name) == 0)
                opts->txsock = optarg;
            else if(strcmp("rxsock", long_options[option_index].name) == 0)
                opts->rxsock = optarg;
            else if(strcmp("timeout", long_options[option_index].name) == 0)
                opts->timeout_ms = atoi(optarg);
            break;
        case 'h':
            opts->help = 1;
            break;
        case 'v':
            opts->verbose = 1;
            break;
        case 't':
            opts->txsock = optarg;
            break;
        case 'r':
            opts->rxsock = optarg;
            break;
        }
    }

    if(opts->txsock == 0 || opts->rxsock == 0)
    {
        homedir = get_home_directory();
    }

    if(!opts->txsock)
    {
        opts->txsock = malloc(strlen(homedir)+64);
        sprintf(opts->txsock, "/run/lorax/control");
    }

    if(!opts->rxsock)
    {
        opts->rxsock = malloc(strlen(homedir)+64);
        sprintf(opts->rxsock, "%s/client.control", homedir);
    }

    if((argc - optind) != 1)
    {
        err = 1;
        err_output("no command specified\n");
    }

    return err || opts->help;
}

int poll_loop(struct Options *opts)
{
    struct pollfd pfd[1];
    struct sockaddr_un lorax_sock;
    uint8_t buf[64];
    ssize_t bytes_received;
    int ret, err;

    err = 0;
    pfd[0].fd = opts->receive_fd;
    pfd[0].events = POLLIN;

    if(opts->verbose)
        debug_output("waiting %d ms for response\n", opts->timeout_ms);

    ret = poll(pfd, 1, opts->timeout_ms);
    if(ret == 0)
    {
        err_output("timed out waiting for connection\n");
        err = 1;
    }
    else if(ret < 0)
    {
        errno_output("error polling\n");
        err = 2;
    }
    else if(pfd[0].revents & POLLIN)
    {
        if(socket_unix_receive(opts->receive_fd, buf, sizeof(buf), &bytes_received, &lorax_sock))
        {
            err_output("unable to receive from lorax socket\n");
            err = 3;
        }
        if(opts->verbose)
            debug_output("received %d bytes from %s\n", bytes_received, lorax_sock.sun_path);

        // TODO this logic is a bit clunky
        if(bytes_received && buf[0] == CONTROL_RESPONSE_ERROR)
        {
            err_output("received error response\n");
        }
        else if(bytes_received > 2 && buf[0] == CONTROL_RESPONSE_OK && buf[1] == CONTROL_REQUEST_GET_NEIGHBORS)
        {
            for(int i=0;i<buf[2];i++)
                info_output("%02x%02x%02x%02x%02x%02x\n",
                    buf[i*6+3+0],
                    buf[i*6+3+1],
                    buf[i*6+3+2],
                    buf[i*6+3+3],
                    buf[i*6+3+4],
                    buf[i*6+3+5]
                );
        }
        else if(bytes_received > 2 && buf[0] == CONTROL_RESPONSE_OK && buf[1] == CONTROL_REQUEST_GET_MY_ADDRESS)
        {
            info_output("%02x%02x%02x%02x%02x%02x\n",
                buf[2],
                buf[3],
                buf[4],
                buf[5],
                buf[6],
                buf[7]
            );
        }
        //info_output("Received message data: %s\n", buf);
    }
    else
    {
        err_output("unknown return code from poll\n");
        err = 4;
    }

    return err;
}

int
main(int argc, char *argv[])
{
    int err;
    err = 0;

    struct Options opts;
    struct sockaddr_un transmit_sock;
    struct sockaddr_un receive_sock;
    uint8_t *control_buf;
    size_t control_buf_len;
    char *command;

    if(parse_options(&opts, argc, argv))
    {
        print_usage(argv[0]);
        return 1;
    }

    command = argv[argc-1];

    if(opts.verbose)
    {
        debug_output("peer socket:  %s\n", opts.txsock);
        debug_output("bound socket: %s\n", opts.rxsock);
        debug_output("command:      %s\n", command);
    }

    if(strncmp("neighbor", command, 8) == 0)
    {
        control_buf = malloc(1);
        control_buf[0] = CONTROL_REQUEST_GET_NEIGHBORS;
        control_buf_len = 1;
    }
    else if(strncmp("myaddress", command, 10) == 0)
    {
        control_buf = malloc(1);
        control_buf[0] = CONTROL_REQUEST_GET_MY_ADDRESS;
        control_buf_len = 1;
    }
    else
    {
        err_output("invalid command %s\n", command);
        return 2;
    }

    if(socket_create_unix(opts.txsock, &transmit_sock))
    {
        err_output("unable to open socket %s\n", opts.txsock);
    }

    if(socket_unix_bind(opts.rxsock, &opts.receive_fd, &receive_sock))
    {
        err_output("unable to bind to socket %s\n", opts.rxsock);
        err = 2;
    }

    if(socket_unix_send(opts.receive_fd, &transmit_sock, (uint8_t *)control_buf, control_buf_len))
    {
        err_output("unable to transmit message\n");
        err = 3;
    }

    if(!err)
        err = poll_loop(&opts);

    if(close(opts.receive_fd))
    {
        warn_output("unable to close socket %s\n", opts.rxsock);
    }

    if(remove(opts.rxsock))
    {
        warn_output("unable to remove socket %s\n", opts.rxsock);
    }

    free(control_buf);

    return err;
}
