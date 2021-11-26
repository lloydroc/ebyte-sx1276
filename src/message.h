#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "error.h"

#define MESSAGE_ADDRESS_SIZE 6
#define MESSAGE_TYPE_UNREACHABLE 0
#define MESSAGE_TYPE_DATA 1

struct Message
{
    uint8_t retries;
    uint8_t type;
    uint8_t source_address[MESSAGE_ADDRESS_SIZE];
    uint8_t destination_address[MESSAGE_ADDRESS_SIZE];
    uint8_t source_port;
    uint8_t destination_port;
    uint8_t data_length; // TODO rename to data length
    uint8_t data[];
};

struct Message*
message_make_unitialized_packet(uint8_t *data, uint8_t len);

void
message_make_partial(struct Message *message, uint8_t type, uint8_t address_source[], uint8_t address_destination[], uint8_t source_port, uint8_t destination_port);

void
message_swap_source_dest(struct Message *message);

int
message_invalid(uint8_t *message, size_t len);

int
message_match(void *m1, void *m2);

void
message_destroy(void *data);

size_t
message_total_length(struct Message *message);

void
message_print(struct Message *message);

#endif