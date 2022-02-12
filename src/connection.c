#include "connection.h"

struct Connection*
connection_make_uninitialzed(uint8_t state, uint8_t source_port, uint8_t destination_port, int (*message_match)(void *,void*), void (*message_destroy)(void *data))
{
    struct Connection* connection;
    connection = malloc(sizeof(struct Connection));
    memset(connection, 0, sizeof(struct Connection));
    connection->connection_state = state;
    clock_gettime(CLOCK_REALTIME, &connection->state_time);
    connection->source_port = source_port;
    connection->destination_port = destination_port;
    connection->client = false;
    connection->messages = malloc(sizeof(struct List));
    list_init(connection->messages, message_match, message_destroy);

    return connection;
}

int
connection_invalid(uint8_t* con, size_t len)
{
    if(len < sizeof(struct Connection))
        return 1;

    return 0;
}

int
connection_match(void *p1, void *p2)
{
    int t1, t2;
    struct Connection *c1, *c2;
    c1 = (struct Connection *) p1;
    c2 = (struct Connection *) p2;

    t1 = c1->source_port != c2->source_port;
    if(t1)
        return t1;

    t2 = c1->destination_port != c2->destination_port;
    if(t2)
        return t2;

    return 0;
}

void
connection_destroy(void *data)
{
    struct Connection *connection;
    connection = (struct Connection*) data;

    if(connection->sock_client)
        free(connection->sock_client);

    list_destroy(connection->messages);
    free(data);
}

void
connection_print(struct Connection *connection)
{
    char fmt[] = "%s %d %d %d -> %d %s\n";
    char *rfc8601, *client_sock;

    client_sock = malloc(sizeof(struct sockaddr_un));

    if(connection->client)
        strncpy(client_sock, connection->sock_client->sun_path, sizeof(struct sockaddr_un));
    else
        sprintf(client_sock, "client_socket_undefined");

    rfc8601 = rfc8601_timespec(&connection->state_time);
    debug_output(fmt,
        client_sock,
        list_size(connection->messages),
        connection->connection_state,
        connection->source_port,
        connection->destination_port,
        rfc8601
    );

    free(client_sock);
    free(rfc8601);
}

struct Connection*
connection_lookup(struct List* connections, uint8_t source_port, uint8_t destination_port)
{
    struct Connection connection_needle;
    if(connections == NULL)
    {
        return NULL;
    }
    connection_needle.source_port = source_port;
    connection_needle.destination_port = destination_port;

    return list_get(connections, &connection_needle);
}
