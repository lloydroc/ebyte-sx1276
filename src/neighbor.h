#ifndef NEIGHBOR_H
#define NEIGHBOR_H

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <time.h>
#include "error.h"
#include "list.h"
#include "misc.h"
#include "connection.h"

#define NEIGHBOR_ADDRESS_SIZE 6

struct Neighbor
{
    uint8_t address[NEIGHBOR_ADDRESS_SIZE];
    uint8_t type;
    uint8_t num_neighbors;
    struct timespec broadcast_time;
    struct List *connections;
};

struct Neighbor*
neighbor_make_uninitialzed(uint8_t type, uint8_t address[], int (*connection_match)(void *,void*), void (*connection_destroy)(void *data));

void
neighbor_init(struct Neighbor *neighbor, int (*neighbor_match)(void *,void*), void (*neighbor_destroy)(void *data), int (*connection_match)(void *,void*), void (*connection_destroy)(void *data));

int
neighbor_match(void *p1, void *p2);

void
neighbor_destroy(void *data);

void
neighbor_print(struct Neighbor *neighbor);

#endif
