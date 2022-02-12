#ifndef CONNECTION_H
#define CONNECTION_H

#include <sys/socket.h>
#include "error.h"
#include "message.h"
#include "socket.h"
#include "list.h"
#include "misc.h"

/* A connection can be in two states:
   1) waiting for a message
   2) waiting for a packet
   For example we create a new connection by receiving a
   message to a neighbor. We will convert the message to
   a packet and then send it off. We'll put this connection
   in the waiting packet state since we're waiting on a packet
   back from the destination.
   On the other end if we receive a packet that with the destination
   that matches our lorax we will send this packet to the server socket
   and wait to hear back. Thus, we'd be waiting for a message back from
   the server. Once we hear back from the server we'll put the connection
   back in the state of waiting for a packet.
   */
enum STATE_CONNECTION
{
    STATE_WAITING_MESSAGE,
    STATE_WAITING_PACKET
};

/*
A connection holds the source and destination address and port.
We create a connection when a message is sent to a neighbor, or,
a packet with the destination address of our lorax.
When created we will set the source to ourselves and the destination
to the other end. However, when looking up a connection from a
received packet we need to swap the source and destination as we
hold the connection to other end.
*/
struct Connection
{
    enum STATE_CONNECTION connection_state;
    struct sockaddr_un *sock_client;
    struct timespec state_time;
    uint8_t source_port;
    uint8_t destination_port;
    bool client;
    struct List *messages;
};

struct Connection*
connection_make_uninitialzed(uint8_t state, uint8_t source_port, uint8_t destination_port, int (*message_match)(void *,void*), void (*message_destroy)(void *data));

int
connection_invalid(uint8_t* con, size_t len);

int
connection_match(void *p1, void *p2);

void
connection_destroy(void *data);

void
connection_print(struct Connection *connection);

struct Connection*
connection_lookup(struct List* connections, uint8_t source_port, uint8_t destination_port);

#endif
