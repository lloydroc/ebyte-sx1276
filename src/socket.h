#ifndef SOCKET_H
#define SOCKET_H

#include "config.h"
#include <libgen.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include "error.h"

/*

These functions help us send data between the e32 running as a daemon
and clients that send us messages.

We will register a socket to the e32 and when we recvfrom the source
will be the e32's data socket. See the e32 documentation for registration.
Effectively we just send it a zero-byte datagram to it's data socket and
it will store it into it's list of sockets to write data when it is received.

We will also recvfrom clients that send us messages. Their source sockets
will need to be stored in a list so that when we receive data from the e32
we can send data back to them.

Sending LoRa data will be different where the destination socket is that of
the e32 socket as well. It is used bidirectionally.

We also will use an unregistered socket that the e32 can send us back an
acknowledgement from transmission. We will sendto the e32 data socket to
transmit data, then listen back on the socket we send from - the e32 data
client - for one byte to know if it was sent or not.

Summary:

recvfrom:
(fd, source sock) -> (e32 data client, e32 data socket)
(fd, source sock) -> (message fd, an unknown clients sock)
(fd, source sock) -> (e32 ack client, e32 data socket)

sendto:
(fd, dest sock) -> (e32 data client, e32 data socket)
(fd, dest sock) -> (message fd, stored clients sockets)

*/

int
socket_create_unix(char *filename, struct sockaddr_un *sock);

int
socket_unix_bind(char *filename, int *fd, struct sockaddr_un *sock);

int
socket_unix_receive(int fd_sock, uint8_t *buf, size_t buf_len, ssize_t *received_bytes, struct sockaddr_un *sock_source);

int
socket_unix_send(int fd_sock, struct sockaddr_un *sock_dest, uint8_t *data, size_t len);

int
socket_unix_send_with_ack(int fd_sock, struct sockaddr_un sock_dest, uint8_t *data, size_t len, int timeout_ms);

int
socket_unix_poll_source_read_ready(int fd_sock, int timeout_ms);

#endif
