#ifndef CONNECTION_H
#define CONNECTION_H

#include "error.h"
#include "message.h"
#include "socket.h"
#include "list.h"
#include "misc.h"

/*enum STATE_CONNECTION
{
    STATE_CONNECTION_CLOSED,
    STATE_CONNECTION_SYN_SENT,
    STATE_CONNECTION_SYN_RCVD,
    STATE_CONNECTION_SYN_ACK_SENT,
    STATE_CONNECTION_SYN_ACK_RCVD,
    // STATE_CONNECTION_ACK_SENT, is established?
    // STATE_CONNECTION_ACK_RCVD, is established
    STATE_CONNECTION_ESTABLISHED,
    STATE_DATA_WAITING_ACK,
    STATE_WAITING_SERVER_RESPONSE,
    STATE_UNKNOWN
};*/

enum STATE_CONNECTION
{
    STATE_WAITING_MESSAGE,
    STATE_WAITING_PACKET
};

/*
A connection holds the source and destination address and port.
This structure is initiated by a client to a server.
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