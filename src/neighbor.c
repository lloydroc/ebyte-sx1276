#include "neighbor.h"

struct Neighbor*
neighbor_make_uninitialzed(uint8_t type, uint8_t address[], int (*connection_match)(void *,void*), void (*connection_destroy)(void *data))
{
    struct Neighbor *neighbor;
    neighbor = malloc(sizeof(struct Neighbor));

    memset(neighbor, 0, sizeof(struct Neighbor));
    neighbor->connections = malloc(sizeof(struct List));
    clock_gettime(CLOCK_REALTIME, &neighbor->broadcast_time);
    neighbor->type = type;
    memcpy(neighbor->address, address, NEIGHBOR_ADDRESS_SIZE);
    list_init(neighbor->connections, connection_match, connection_destroy);

    return neighbor;
}

void
neighbor_init(struct Neighbor *neighbor, int (*neighbor_match)(void *,void*), void (*neighbor_destroy)(void *data), int (*connection_match)(void *,void*), void (*connection_destroy)(void *data))
{
    memset(neighbor, 0, sizeof(struct Neighbor));
    neighbor->connections = malloc(sizeof(struct List));
    clock_gettime(CLOCK_REALTIME, &neighbor->broadcast_time);
    list_init(neighbor->connections, connection_match, connection_destroy);
}

int
neighbor_match(void *p1, void *p2)
{
    struct Neighbor *n1, *n2;
    n1 = (struct Neighbor *) p1;
    n2 = (struct Neighbor *) p2;
    return memcmp(n1->address, n2->address, NEIGHBOR_ADDRESS_SIZE);
}

void
neighbor_destroy(void *data)
{
    struct Neighbor *neighbor;
    neighbor = (struct Neighbor *) data;
    list_destroy(neighbor->connections);
    free(data);
}

void
neighbor_print(struct Neighbor *neighbor)
{
    char fmt[] = "%02x %02x%02x%02x%02x%02x%02x %02x %02x %s\n";
    char *rfc8601;

    rfc8601 = rfc8601_timespec(&neighbor->broadcast_time);
    debug_output(fmt,
        neighbor->type,
        neighbor->address[0],
        neighbor->address[1],
        neighbor->address[2],
        neighbor->address[3],
        neighbor->address[4],
        neighbor->address[5],
        neighbor->num_neighbors,
        list_size(neighbor->connections),
        rfc8601
    );

    free(rfc8601);
}
