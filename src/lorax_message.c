#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>

#include "error.h"
#include "message.h"
#include "misc.h"
#include "socket.h"

// TODO 1 need to allow to change the type of the message
// TODO 2 we have a permission problem for /run/lorax/message where it doesn't have other write privs

int use_syslog = 0;

struct Options
{
  int help;
  int verbose;
  char *message;
  char *txsock;
  char *rxsock;
  int receive_fd;
  int timeout_ms;
  int retries;
  int message_type;
  uint8_t source_address[6];
  uint8_t destination_address[6];
  uint8_t source_port;
  uint8_t destination_port;
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
    printf("Usage: %s [OPTIONS] dest_addr dest_port message\n", progname);
    printf("Version: %s\n\n", VERSION);
    printf("A client to send reliable messages over LORA\n\n");

    printf("type: the type of message 0=DATA\n");
    printf("dest_addr: a 6 byte hex field. E.g. aabbccddeeff\n");
    printf("dest_port: a 1 byte port from 0-255 in decimal\n");
    printf("message: the message to send\n");

    printf("OPTIONS:\n\
-h, --help\n\
-v, --verbose\n\
    --type [TYPE] type of message to send. Default is data\n\
    --txsock [/run/lorax/messages] destination socket we'll send the message to\n\
    --rxsock [%s/client.messages] source socket we will listen for a response\n\
    --retries [1] number of times to retry if the message fails\n\
    --timeout [30000] how long wait for a response on milliseconds\n\
    --source-address TODO\n\n", homedir);
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
    opts->timeout_ms = 55000;
    opts->retries = 3;
    opts->message_type = MESSAGE_TYPE_DATA;

    static struct option long_options[] =
    {
        {"help",                     no_argument, 0, 'h'},
        {"verbose",                  no_argument, 0, 'v'},
        {"txsock",             required_argument, 0,   0},
        {"rxsock",             required_argument, 0,   0},
        {"retries",            required_argument, 0,   0},
        {"timeout",            required_argument, 0,   0},
        {"type",               required_argument, 0,   0},
        {"source-address",     required_argument, 0,   0}, // TODO
        {0,                                    0, 0,   0}
    };

    while(1)
    {
        option_index = 0;
        c = getopt_long(argc, argv, "hv", long_options, &option_index);

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
            else if(strcmp("retries", long_options[option_index].name) == 0)
                opts->retries = atoi(optarg);
            else if(strcmp("type", long_options[option_index].name) == 0)
                opts->message_type = atoi(optarg);
            else if(strcmp("source-address", long_options[option_index].name) == 0)
            {
                err = parse_mac_address(optarg, opts->source_address);
                if(err)
                {
                    err_output("unable to parse mac address %s\n", optarg);
                }
            }
            break;
        case 'h':
            opts->help = 1;
            break;
        case 'v':
            opts->verbose = 1;
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
        sprintf(opts->txsock, "/run/lorax/messages");
    }

    if(!opts->rxsock)
    {
        opts->rxsock = malloc(strlen(homedir)+64);
        sprintf(opts->rxsock, "%s/client.messages", homedir);
    }

    // TODO make configurable
    if(get_mac_address(opts->source_address, "eth0"))
    {
        if(get_mac_address(opts->source_address, "wlan0"))
        {

        }
    }

    opts->source_port = 1;

    if((argc - optind) != 3)
    {
        err = 1;
        err_output("not enough input arguments\n");
    }
    else
    {
        if(parse_mac_address(argv[argc-3], opts->destination_address))
        {
            err_output("bad mac address: expect 12 chars = 6 hex bytes\n");
            err = 10;
        }

        opts->destination_port = atoi(argv[argc-2]);
        if(opts->destination_port > 255)
        {
            err_output("destination port > 255\n");
            err = 11;
        }

        opts->message = argv[argc-1];
    }

    return err || opts->help;
}

int poll_loop(struct Options *opts)
{
    struct pollfd pfd[1];
    struct sockaddr_un lorax_sock;
    struct Message *message;
    uint8_t buf[1024];
    char *message_ptr;
    ssize_t bytes_received;
    int ret, err;

    err = 0;
    pfd[0].fd = opts->receive_fd;
    pfd[0].events = POLLIN;

    info_output("waiting %d ms for response\n", opts->timeout_ms);
    ret = poll(pfd, 1, opts->timeout_ms);
    if(ret == 0)
    {
        err_output("timed out waiting for response\n");
        err = 10;
    }
    else if(ret < 0)
    {
        errno_output("error polling\n");
        err = 11;
    }
    else if(pfd[0].revents & POLLIN)
    {
        if(socket_unix_receive(opts->receive_fd, buf, 1024, &bytes_received, &lorax_sock))
        {
            err_output("unable to receive from lorax socket\n");
            err = 12;
        }

        info_output("received %d bytes from %s\n", bytes_received, lorax_sock.sun_path);
        if(message_invalid(buf, bytes_received))
        {
            err_output("invalid message\n");
        }
        message = (struct Message *) buf;
        message_print(message);

        if(message_invalid(buf, bytes_received))
        {
            err_output("message received is invalid\n");
            err = 13;
        }
        else
        {
            message_ptr = (char *) message->data;
            message_ptr[message->data_length] = '\0';

            info_output("Received message data [%d bytes]: %s\n", message->data_length, message_ptr);
        }
    }
    else
    {
        err_output("unknown return code from poll\n");
        err = 14;
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
    struct Message *message;

    if(parse_options(&opts, argc, argv))
    {
        print_usage(argv[0]);
        return 1;
    }

    info_output("peer socket:  %s\n", opts.txsock);
    info_output("bound socket: %s\n", opts.rxsock);
    info_output("message:      %s\n", opts.message);
    info_output("message type: %d\n", opts.message_type);

    if(socket_create_unix(opts.txsock, &transmit_sock))
    {
        err_output("unable to open socket %s\n", opts.txsock);
    }

    if(socket_unix_bind(opts.rxsock, &opts.receive_fd, &receive_sock))
    {
        err_output("unable to bind to socket %s\n", opts.rxsock);
        err = 2;
    }

    printf("text is %d %s\n", strlen(opts.message), opts.message);

    message = message_make_uninitialized_message((uint8_t *) opts.message, strlen(opts.message));
    message_make_partial(message, opts.message_type, opts.source_address, opts.destination_address,
        opts.source_port, opts.destination_port);
    message->retries = opts.retries;

    if(message_invalid((uint8_t *)message, message_total_length(message)))
    {
        err_output("message is invalid\n");
        err = 3;
    }

    if(opts.verbose)
    {
        info_output("transmitting message of %d bytes: ", message_total_length(message));
        message_print(message);

    }

    if(socket_unix_send(opts.receive_fd, &transmit_sock, (uint8_t *)message, message_total_length(message)))
    {
        err_output("unable to transmit message\n");
        err = 4;
        goto cleanup;
    }

    err = poll_loop(&opts);

    if(close(opts.receive_fd))
    {
        warn_output("unable to close socket %s\n", opts.rxsock);
    }

    if(remove(opts.rxsock))
    {
        warn_output("unable to remove socket %s\n", opts.rxsock);
    }

cleanup:
    free(message);

    return err;
}
