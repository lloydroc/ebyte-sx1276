#include "options_lorax.h"

// tells error.c when we print to use stdout, stderr, or syslog
int use_syslog = 0;

void options_lorax_usage(char *progname)
{
    struct OptionsLorax opts;
    options_lorax_init(&opts);
    printf("Usage: %s [OPTIONS]\n", progname);
    printf("Version %s\n\n", VERSION);
    printf("OPTIONS:\n\
    -h --help                  Print help\n\
    -v --verbose               Verbose Output\n\
    -i --eth-iface             STRING Ethernet Interface to get mac address from. E.G. eth0\n\
    -d, --daemon               become a daemon\n\
        --systemd  create a pid file in the /run/lorax directory\n\
    -m --sock-unix-messages    FILE Listening Socket for Client Messages\n\
    -s --sock-unix-server      FILE Listening Socket for the server\n\
    -a --sock-unix-e32-ack     FILE Listening Socket for e32 Acknowledements\n\
    -c --sock-unix-e32-client  FILE Client Socket the e32 will send data to\n\
    -e --sock-unix-e32-data    FILE Output Sensor Data to E32\n");
}

int
options_lorax_get_mac_address(struct OptionsLorax *opts, char *iface)
{
    if(get_mac_address(opts->mac_address, iface) == 0)
    {
        opts->iface_default = strdup(iface);
        return 0;
    }
    return 1;
}

void
options_lorax_init(struct OptionsLorax *opts)
{
    opts->verbose = false;
    opts->help = false;
    opts->iface_default = NULL;
    opts->timeout_to_e32_socket_ms = 3000;
    opts->timeout_from_e32_rx_ms = 10000;
    opts->timeout_broadcast_ms = 10000;
    opts->num_retries = 1;
    opts->daemon = false;
    opts->systemd = false;
    sprintf(opts->rundir, "/run/lorax");
    sprintf(opts->pidfile, "%s/lorax.pid", opts->rundir);
}

int
options_lorax_deinit(struct OptionsLorax *opts)
{
    if(opts->iface_default)
        free(opts->iface_default);
    return 0;
}

int
options_lorax_parse(struct OptionsLorax *opts, int argc, char *argv[])
{
    int option_index, c;
    int err = 0;
    static struct option long_options[] =
    {
        {"help",                      no_argument, 0, 'h'},
        {"verbose",                   no_argument, 0, 'v'},
        {"daemon",                    no_argument, 0, 'd'},
        {"systemd",                   no_argument, 0, 0},
        {"eth-iface",                 required_argument, 0, 'i'},
        {"sock-unix-messages",        required_argument, 0, 'm'},
        {"sock-unix-e32-ack",         required_argument, 0, 'a'},
        {"sock-unix-e32-client",      required_argument, 0, 'c'},
        {"sock-unix-e32-data",        required_argument, 0, 'e'},
        {"sock-unix-server",          required_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    while (1)
    {
        option_index = 0;
        c = getopt_long(argc, argv, "hvdi:m:s:a:c:e:", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
        case 0:
            if(strcmp("help", long_options[option_index].name) == 0)
                opts->help = true;
            if(strcmp("verbose", long_options[option_index].name) == 0)
                opts->verbose = true;
            if(strcmp("daemon", long_options[option_index].name) == 0)
                opts->daemon = true;
            if(strcmp("systemd", long_options[option_index].name) == 0)
                opts->systemd = true;
            if(strcmp("eth-iface", long_options[option_index].name) == 0)
            {
                opts->iface_default = strdup(optarg);
                err |= options_lorax_get_mac_address(opts, optarg);
            }
            if(strcmp("sock-unix-messages", long_options[option_index].name) == 0)
                err |= socket_unix_bind(optarg, &opts->fd_socket_message_data, &opts->sock_message_rx);
            if(strcmp("sock-unix-server", long_options[option_index].name) == 0)
                err |= socket_create_unix(optarg, &opts->sock_server_data);
            if(strcmp("sock-unix-e32-ack", long_options[option_index].name) == 0)
                err |= socket_unix_bind(optarg, &opts->fd_socket_e32_ack_client, &opts->sock_e32_tx_ack);
            if(strcmp("sock-unix-e32-client", long_options[option_index].name) == 0)
                err |= socket_unix_bind(optarg, &opts->fd_socket_e32_data_client, &opts->sock_e32_data_rx);
            if(strcmp("sock-unix-e32-data", long_options[option_index].name) == 0)
                err |= socket_create_unix(optarg, &opts->sock_e32_data);
            break;
        case 'h':
            opts->help = true;
            break;
        case 'v':
            opts->verbose = true;
            break;
        case 'd':
            opts->daemon = true;
            break;
        case 'i':
            err |= options_lorax_get_mac_address(opts, optarg);
            break;
        case 'm':
            err |= socket_unix_bind(optarg, &opts->fd_socket_message_data, &opts->sock_message_rx);
            break;
        case 's':
            err |= socket_create_unix(optarg, &opts->sock_server_data);
            break;
        case 'a':
            err |= socket_unix_bind(optarg, &opts->fd_socket_e32_ack_client, &opts->sock_e32_tx_ack);
            break;
        case 'c':
            err |= socket_unix_bind(optarg, &opts->fd_socket_e32_data_client, &opts->sock_e32_data_rx);
            break;
        case 'e':
            err |= socket_create_unix(optarg, &opts->sock_e32_data);
            break;
        default:
            err |= 1;
        }
    }

    if(err == 0 && opts->iface_default == NULL)
    {
        if(options_lorax_get_mac_address(opts, "eth0"))
        {
            if(options_lorax_get_mac_address(opts, "wlan0"))
            {
                err |= 1;
            }
        }
    }

    if(opts->daemon)
    {
        use_syslog = 1;
        openlog("lorax", 0, LOG_DAEMON);
    }

    return err;
}

void options_lorax_print(struct OptionsLorax *opts)
{
    if(!opts->verbose)
    {
        return;
    }

    debug_output("option verbose is %d\n", opts->verbose);
    debug_output("option help is %d\n", opts->help);
    debug_output("option iface_default is %s\n", opts->iface_default);
    debug_output("option timeout_to_e32_socket_ms is %d\n", opts->timeout_to_e32_socket_ms);
    debug_output("option timeout_from_e32_rx_ms is %d\n", opts->timeout_from_e32_rx_ms);
    debug_output("option timeout_broadcast_ms is %d\n", opts->timeout_broadcast_ms);
    debug_output("option num_retries is %d\n", opts->num_retries);
    debug_output("option daemon is %d\n", opts->daemon);

    debug_output("option sock_message_rx is %s with fd %d\n", opts->sock_message_rx.sun_path, opts->fd_socket_message_data);
    debug_output("option sock_e32_tx_ack is %s with fd %d\n", opts->sock_e32_tx_ack.sun_path, opts->fd_socket_e32_ack_client);
    debug_output("option sock_e32_data_rx is %s with fd %d\n", opts->sock_e32_data_rx.sun_path, opts->fd_socket_e32_ack_client);
    debug_output("option sock_server_data is %s\n", opts->sock_server_data.sun_path);
}
