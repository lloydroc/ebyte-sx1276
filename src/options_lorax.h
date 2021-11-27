#ifndef OPTIONS_LORAX_H
#define OPTIONS_LORAX_H

#include <getopt.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <netdb.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "socket.h"
#include "error.h"
#include "misc.h"

/*

  1     2
  |     ^
  v     |
  ________
     us
  ________
  |   ^  ^
  v   |  |
  3   4  5
  ________
    e32
  ________
    ^
    |
    v
    6

1 we listen for data
2 we send back to client
3 send data to e32 for tx
4 and acknowledgement data was sent from 3
5 a registered socket with e32 for rx data
6 the uart file to send to the e32 module

1 sock_message_rx bind recvfrom
2 not in options but saved when sensor data sent
3 socket_unix_e32_data open sendto
4 sock_e32_tx_ack bind recvfrom
5 sock_e32_data_rx bind recvfrom

recvfrom(sockfd, src_sock)
sendto(sockfd, dest_sock)

source is the e32_ack_client or e32_data_client
dest   is the e32_data

send a packet (source_sock not used)
  sendto:
   sockfd e32_ack_client
   dest   e32_data
  recvfrom:
   sockfd e32_ack_client
   source e32_data

receive a packet:
  recvfrom:
    sockfd e32_data_client (registered)
    source e32_data

what about the file descriptors?
 e32_data_client and e32_ack_client are used for recvfrom and sendto
 sensor_data is polled

seems source_sock isn't used and dest_fd isn't used

 pairs (source, dest)
   (e32_data_client, e32_data) -> will receive data from e32
   (e32_ack_client, e32_data)  -> will send data to the e32
   (sensor_data, x)            -> will receive data from client

*/

struct OptionsLorax
{
    bool verbose;
    bool help;
    bool daemon;
    bool systemd;

    char rundir[64];
    char pidfile[64];

    char *iface_default;
    uint8_t mac_address[6];

    /* incoming messages from clients */
    int fd_socket_message_data;

    /* incoming data received from e32 over lora (registered) */
    int fd_socket_e32_data_client;

    /* used to acknowledge data sent to e32 for transmission */
    int fd_socket_e32_ack_client;

    /* outgoing message to server */
    struct sockaddr_un sock_server_data;

    /* the e32 socket that we will send data for transmission and
       also the source socket when data is received and sent to
       our registered socket
    */
    struct sockaddr_un sock_e32_data;

    uint8_t num_retries;

    /* TODO unused */
    struct sockaddr_un sock_e32_tx_ack;
    struct sockaddr_un sock_e32_data_rx;
    struct sockaddr_un sock_message_rx;

    int timeout_to_e32_socket_ms, timeout_from_e32_rx_ms, timeout_broadcast_ms;
};

void
options_lorax_usage(char *progname);

void
options_lorax_init(struct OptionsLorax *opts);

int
options_lorax_deinit(struct OptionsLorax *opts);

int
options_lorax_get_mac_address(struct OptionsLorax *opts, char *iface);

int
options_lorax_parse(struct OptionsLorax *opts, int argc, char *argv[]);

void
options_lorax_print(struct OptionsLorax *opts);

#endif
