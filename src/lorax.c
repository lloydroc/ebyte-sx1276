#include "lorax.h"

struct Neighbor *myself;
struct List *neighbors;

uint8_t e32_rx_buf[64];
uint8_t message_rx_buf[64];

#define NEIGHBOR_STALE_SECONDS 60
#define CONNECTION_RETRY_SECONDS 3

int
lorax_e32_init(struct OptionsLorax *opts)
{
    neighbors = malloc(sizeof(struct List));
    list_init(neighbors, neighbor_match, neighbor_destroy);

    myself = malloc(sizeof(struct Neighbor));
    memset(myself, 0, sizeof(struct Neighbor));
    memcpy(myself->address, opts->mac_address, 6);

    myself->connections = malloc(sizeof(struct List));
    list_init(myself->connections, connection_match, connection_destroy);

    info_output("my source address: %02x%02x%02x%02x%02x%02x\n",
        myself->address[0],
        myself->address[1],
        myself->address[2],
        myself->address[3],
        myself->address[4],
        myself->address[5]
    );

    return 0;
}

int
lorax_e32_destroy()
{
    // FIXME destroy connections
    list_destroy(neighbors);
    free(neighbors);
    free(myself);
    return 0;
}

int
lorax_e32_register(struct OptionsLorax *opts)
{
    if(opts->verbose)
        debug_output("lorax_e32_register: registering client\n");

    if(socket_unix_send_with_ack(opts->fd_socket_e32_data_client, opts->sock_e32_data, NULL, 0, opts->timeout_to_e32_socket_ms))
    {
        err_output("unable to register e32\n");
        return 1;
    }

    return 0;
}

static int
lorax_send_message(struct OptionsLorax *opts, struct sockaddr_un *sock_dest, struct Message *message)
{
    size_t message_len;

    message_len = message_total_length(message);

    if(opts->verbose)
    {
        debug_output("lorax_send_message: %s ", sock_dest->sun_path);
        message_print(message);
    }

    return socket_unix_send(opts->fd_socket_message_data, sock_dest, (uint8_t *) message, message_len);
}

static int
lorax_send_packet(struct OptionsLorax *opts, uint8_t *packet, ssize_t packet_size)
{
    if(opts->verbose)
    {
        debug_output("lorax_send_packet: %s ", opts->sock_e32_data.sun_path);
        packet_print((struct PacketHeader*) packet);
    }

    return socket_unix_send_with_ack(opts->fd_socket_e32_ack_client, opts->sock_e32_data, packet, packet_size, opts->timeout_to_e32_socket_ms);
}

static int
lorax_send_broadcast_packet(struct OptionsLorax *opts)
{
    int err;
    struct PacketHeader *packet;
    uint8_t *packet_data;
    uint8_t destination_broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    packet_make_uninitialized_packet(&packet, 1);
    packet_make_partial(packet, PACKET_HEADER_BROADCAST, myself->address, destination_broadcast, 0, 0);

    packet_data = packet_get_data_pointer(packet);

    // set the data section
    *packet_data = (uint8_t) list_size(neighbors);

    // compute the checksum
    packet_compute_checksum(packet, packet->total_length);

    clock_gettime(CLOCK_REALTIME, &myself->broadcast_time);

    err = lorax_send_packet(opts, (uint8_t *) packet, packet->total_length);

    free(packet);
    return err;
}

static struct Neighbor*
lorax_find_neighbor_by_address(uint8_t address[])
{
    struct Neighbor needle;

    memcpy(needle.address, address, 6);

    if(neighbor_match(myself, &needle) == 0)
    {
        return myself;
    }

    return list_get(neighbors, &needle);
}

static int
lorax_neighbor_loop(struct OptionsLorax *opts)
{
    struct Neighbor *neighbor;
    struct Connection *connection;
    struct Message *message;
    struct PacketHeader *packet;
    struct timespec now_time;

    struct ListIterator neighbors_iter;
    struct ListIterator connections_iter;

    time_t delta_seconds;

    list_iter_init(neighbors, &neighbors_iter);

    clock_gettime(CLOCK_REALTIME, &now_time);

    while(list_iter_has_next(&neighbors_iter))
    {
        neighbor = list_iter_get(&neighbors_iter);

        /* neighbors we have not heard from in a long time
           we will remove
        */
       delta_seconds = now_time.tv_sec - neighbor->broadcast_time.tv_sec;
       if(delta_seconds > NEIGHBOR_STALE_SECONDS)
       {
            info_output("not heard from neighbor for %d seconds, removing: %02x%02x%02x%02x%02x%02x\n",
                NEIGHBOR_STALE_SECONDS,
                neighbor->address[0],
                neighbor->address[1],
                neighbor->address[2],
                neighbor->address[3],
                neighbor->address[4],
                neighbor->address[5]
            );
           list_iter_remove(&neighbors_iter);
           continue;
       }

        list_iter_init(neighbor->connections, &connections_iter);

        while(list_iter_has_next(&connections_iter))
        {
            connection = list_iter_get(&connections_iter);

            delta_seconds = now_time.tv_sec - connection->state_time.tv_sec;
            if(connection->connection_state == STATE_WAITING_PACKET && delta_seconds > CONNECTION_RETRY_SECONDS)
            {
                message = list_get_first(connection->messages);

                if(message && message->retries)
                {
                    info_output("retry packet %dx\n", message->retries);
                    message->retries--;
                    message_to_packet(message, &packet);
                    lorax_send_packet(opts, (uint8_t *)packet, packet->total_length);
                    free(packet);
                    return 1;
                }
                // have a message but retries are zero
                else if(message)
                {
                    // exhausted retries send message to client
                    message->type = MESSAGE_TYPE_UNREACHABLE;
                    if(socket_unix_send(opts->fd_socket_message_data, connection->sock_client, (uint8_t *) message, message_total_length(message)))
                    {
                        list_remove_first(connection->messages);
                        err_output("unreachable: unable to send message to client\n");
                    }
                    return 2;
                }
                else
                {
                    // FIXME should we remove the connection
                    warn_output("no messages for connection\n");
                }
                // loop through messages and retry them
                // retry
            }

            /* connections need to be retried or purged.
               when we retry we send back to the e32. when
               we purge them we send back to the client socket
               that we could not deliver the message
            */
            list_iter_next(&connections_iter);
        }

        list_iter_next(&neighbors_iter);
    }

    return 0;
}

/*
    Add or update the neighbor to our neighbor list.
*/
static int
lorax_e32_process_broadcast_packet(struct PacketHeader *packet, struct OptionsLorax *opts)
{
    struct Neighbor *neighbor;
    uint8_t *num_neighbors_ptr;

    /* update the neighbor if in the list */
    neighbor = lorax_find_neighbor_by_address(packet->source_address);
    if(neighbor == NULL)
    {
        neighbor = neighbor_make_uninitialzed(packet->type, packet->source_address, connection_match, connection_destroy);
        debug_output("lorax_e32_process_broadcast_packet: new neighbor: ");
        neighbor_print(neighbor);
        list_add_last(neighbors, neighbor);
    }
    else
    {
        neighbor->type = packet->type;
        clock_gettime(CLOCK_REALTIME, &neighbor->broadcast_time);
    }

    num_neighbors_ptr = (uint8_t *)packet+sizeof(struct PacketHeader);
    neighbor->num_neighbors = *num_neighbors_ptr;

    return 0;
}

/*
    Process Packets from reception
    It is assumed the packet is valid.

    Packets are the lower level that get passed up to messages.
    These messages are sent either to the client who sent the
    message in the first place as a reply. Or to a server to
    which the request will come to.

    We can also send packets back instead of messages for example
    when we are unable to send a packet upstream we'll send back
    an error packet.
*/
int
lorax_e32_process_packet(struct OptionsLorax *opts, uint8_t *packet, size_t len)
{
    //uint8_t packet_to_send_header;
    struct PacketHeader *packet_received;
    //enum STATE_CONNECTION new_state, existing_state;
    struct Neighbor *neighbor;
    struct Connection *connection;
    struct Message *message;
    struct sockaddr_un *sock_dest;
    int err;

    err = 0;
    packet_received = malloc(len);
    memcpy(packet_received, packet, len);
    //packet_received = (struct PacketHeader*) packet;

    if(packet_received->type == PACKET_HEADER_BROADCAST)
    {
        return lorax_e32_process_broadcast_packet(packet_received, opts);
    }

    if(memcmp(packet_received->destination_address, myself->address, PACKET_ADDRESS_SIZE))
    {
        debug_output("not processing packet as destination address doesn't match");
        return 0;
    }

    neighbor = lorax_find_neighbor_by_address(packet_received->source_address);

    if(neighbor == NULL)
    {
        neighbor = neighbor_make_uninitialzed(0, packet_received->source_address, connection_match, connection_destroy);
        list_add_first(neighbors, neighbor);
        debug_output("lorax_e32_process_packet: new neighbor ");
        neighbor_print(neighbor);
    }

    connection = connection_lookup(neighbor->connections, packet_received->destination_port, packet_received->source_port);
    if(connection == NULL)
    {
        connection = connection_make_uninitialzed(STATE_WAITING_PACKET, packet_received->destination_port, packet_received->source_port, message_match, message_destroy);
        list_add_first(neighbor->connections, connection);
        debug_output("lorax_e32_process_packet: new connection");
        connection_print(connection);
    }

    if(connection->client)
    {
        sock_dest = connection->sock_client;
    }
    else
    {
        sock_dest = &opts->sock_server_data;
    }

    if(sock_dest == NULL || strlen(sock_dest->sun_path) == 0)
        warn_output("no server socket specified");

    packet_to_message(packet_received, &message);
    if(lorax_send_message(opts, sock_dest, message))
    {
        err_output("unable to send message from received packet sending back error packet\n");
        // send back the same packet with of type ERROR
        packet_swap_source_dest(packet_received);
        packet_received->type = PACKET_ERROR_SERVER;
        packet_compute_checksum(packet_received, packet_received->total_length);
        if(lorax_send_packet(opts, (uint8_t *)packet_received, packet_received->total_length))
        {
            list_remove_first(neighbor->connections);
            err = 2;
            err_output("unable to send PACKET_ERROR_SERVER packet\n");
        }
    }

    connection->connection_state = STATE_WAITING_MESSAGE;
    clock_gettime(CLOCK_REALTIME, &connection->state_time);

    free(message);
    free(packet_received);
    return err;
}

/*
    Process Messages from clients.

    We will normally conver the message to a packet and send it. However,
    the message can also be a request for some information such as the
    known neighors in which we can send a message back.
*/
int
lorax_e32_process_message(struct OptionsLorax *opts, uint8_t *packet_bytes, size_t len, struct sockaddr_un *sock_source)
{

    int err;
    struct Message *message;
    struct PacketHeader *packet_to_send;
    struct Neighbor *neighbor, neighbor_lookup;
    struct Connection *connection;

    err = 0;
    packet_to_send = NULL;

    // we malloc since the message is added to the connection's list of messages
    message = (struct Message *) malloc(len);
    memcpy(message, packet_bytes, len);

    if(strncmp(sock_source->sun_path, (const char*) &opts->sock_server_data.sun_path, strnlen(sock_source->sun_path, sizeof(struct sockaddr_un))) == 0)
    {
        debug_output("message was received by server: swapping source and destination addresses\n");
        message_swap_source_dest(message);
    }

    if(memcmp(message->source_address, myself->address, MESSAGE_ADDRESS_SIZE))
    {
        warn_output("received message with source address not equal to our address\n");
    }

    memcpy(neighbor_lookup.address, message->destination_address, 6);

    /* lookup neighbor */
    neighbor = lorax_find_neighbor_by_address(neighbor_lookup.address);

    // create neighbor if it doesn't exist
    if(neighbor == NULL)
    {
        neighbor = neighbor_make_uninitialzed(0, message->destination_address, connection_match, connection_destroy);
        list_add_first(neighbors, neighbor);
        debug_output("lorax_e32_process_message: new neighbor ");
        neighbor_print(neighbor);
    }

    /* make a new connection and send the message */
    connection = connection_lookup(neighbor->connections, message->source_port, message->destination_port);
    if(connection == NULL)
    {
        connection = connection_make_uninitialzed(STATE_WAITING_PACKET, message->source_port, message->destination_port, message_match, message_destroy);
        connection->client = true;
        connection->sock_client = malloc(sizeof(struct sockaddr_un));
        memcpy(connection->sock_client, sock_source, sizeof(struct sockaddr_un));
        list_add_first(neighbor->connections, connection);
        debug_output("lorax_e32_process_message: new connection ");
        connection_print(connection);
    }
    else if(connection->connection_state == STATE_WAITING_PACKET)
    {
        // FIXME we should send back message to client not send a packet
        // TODO
        warn_output("message is STATE_WAITING_PACKET\n");
    }

    /* only add the message for retries if client and not the server */
    // TODO compare sockets can be better
    if(strncmp(sock_source->sun_path, opts->sock_server_data.sun_path, sizeof(struct sockaddr_un)))
    {
        list_add_last(connection->messages, message);
    }

    // convert the message to a packet
    // TODO what about source of packet? client shouldn't have to fill in?
    message_to_packet(message, &packet_to_send);

    if(lorax_send_packet(opts, (uint8_t *)packet_to_send, packet_to_send->total_length))
    {
        list_remove_first(neighbor->connections);
        err = 2;
    }

    free(packet_to_send);

    return err;
}

int
lorax_e32_poll(struct OptionsLorax *opts)
{
    struct pollfd pfd[2];

    /* info from other sockets */
    struct sockaddr_un sock_source;
    ssize_t received_bytes;

    /* for time keeping */
    struct timespec now_time;
    int seconds_ago_broadcast;
    int ret;
    int timeout_broadcast_s = opts->timeout_broadcast_ms/1000;

    pfd[0].fd = opts->fd_socket_e32_data_client;
    pfd[0].events = POLLIN;
    pfd[1].fd = opts->fd_socket_message_data;
    pfd[1].events = POLLIN;

    for(;;)
    {
        ret = poll(pfd, 2, opts->timeout_broadcast_ms);
        if(ret == 0)
        {
            if(lorax_neighbor_loop(opts))
            {
                continue;
            }
            else if(lorax_send_broadcast_packet(opts))
                err_output("lorax_e32_poll: unable to send broadcast in poll timeout\n");
            continue;
        }
        if(ret < 0)
        {
            errno_output("lorax_e32_poll: error polling\n");
            sleep(3);
        }

        /* e32 data client sending data to us */
        if(pfd[0].revents & POLLIN)
        {
            if(socket_unix_receive(pfd[0].fd, e32_rx_buf, sizeof(e32_rx_buf), &received_bytes, &sock_source))
            {
                errno_output("lorax_e32_poll: error receiving from e32 socket\n");
                continue;
            }

            ret = packet_invalid(e32_rx_buf, received_bytes);
            if(ret)
            {
                err_output("lorax_e32_poll: invalid packet of %d bytes got exit code %d\n", received_bytes, ret);
                continue;
            }

            if(opts->verbose)
            {
                debug_output("lorax_e32_poll: received packet ");
                packet_print((struct PacketHeader *) e32_rx_buf);
            }

            if(lorax_e32_process_packet(opts, e32_rx_buf, received_bytes))
            {
                err_output("lorax_e32_poll: unable to process packet\n");
                continue;
            }
        }

        /* client sending data for us to send to the e32 client to transmit */
        if(pfd[1].revents & POLLIN)
        {
            if(socket_unix_receive(pfd[1].fd, message_rx_buf, sizeof(message_rx_buf), &received_bytes, &sock_source))
            {
                errno_output("lorax_e32_poll: error receiving from message socket\n");
                return 1;
            }

            ret = message_invalid(message_rx_buf, received_bytes);
            if(ret)
            {
                err_output("lorax_e32_poll: invalid message from %s of %d bytes got exit code %d ", sock_source.sun_path, received_bytes, ret);
                continue;
            }

            if(opts->verbose)
            {
                debug_output("lorax_e32_poll: received message from %s ", sock_source.sun_path);
                message_print((struct Message *) message_rx_buf);
            }

            if(lorax_e32_process_message(opts, message_rx_buf, received_bytes, &sock_source))
            {
                err_output("lorax_e32_poll: unable to process message\n");
                continue;
            }
        }

        /* squeeze in a broadcast not triggered by a timeout */

        if(clock_gettime(CLOCK_REALTIME, &now_time))
        {
            errno_output("lorax_e32_poll: unable to get time\n");
            return 1;
        }

        seconds_ago_broadcast = now_time.tv_sec - myself->broadcast_time.tv_sec;
        if(seconds_ago_broadcast > timeout_broadcast_s)
        {
            if(lorax_send_broadcast_packet(opts))
                err_output("lorax_e32_poll: unable to send broadcast in poll\n");
            continue;
        }
    }
}
