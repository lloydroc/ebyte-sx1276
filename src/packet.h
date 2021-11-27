#ifndef PACKET_H
#define PACKET_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "error.h"

#define PACKET_HEADER_UNKNOWN 0
#define PACKET_DATA 1
#define PACKET_ERROR_INVALID 2
#define PACKET_ERROR_SERVER 3
#define PACKET_HEADER_BROADCAST 0xFF

#define PACKET_ADDRESS_SIZE 6

struct PacketHeader
{
    uint8_t type;
    uint8_t source_address[PACKET_ADDRESS_SIZE];
    uint8_t destination_address[PACKET_ADDRESS_SIZE];
    uint8_t source_port;
    uint8_t destination_port;
    uint8_t total_length;
    uint8_t checksum; // computed for entire packet
    // data follows and is part of the checksum
};

int
packet_invalid(uint8_t *packet, size_t len);

int
packet_make_uninitialized_packet(struct PacketHeader **header, uint8_t data_length);

uint8_t *
packet_get_data_pointer(struct PacketHeader *packet);

uint8_t
packet_get_data_size(struct PacketHeader *packet);

void
packet_make_partial(struct PacketHeader *packet, uint8_t type, uint8_t address_source[], uint8_t address_destination[], uint8_t source_port, uint8_t destination_port);

void
packet_swap_source_dest(struct PacketHeader *packet);

uint8_t
packet_compute_checksum(struct PacketHeader *packet, size_t packet_len);

void
packet_print(struct PacketHeader *packet);

#endif
