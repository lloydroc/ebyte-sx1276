#include "packet.h"


int
packet_invalid(uint8_t *packet, size_t len)
{
    uint8_t checksum, computed_checksum, type;
    struct PacketHeader *header;

    if(len == 0)
        return 1;

    if(packet == NULL)
        return 2;

    type = packet[0];

    /* check for all types */
    switch(type)
    {
        case PACKET_HEADER_BROADCAST:
        case PACKET_DATA:
        case PACKET_ERROR_SERVER:
        case PACKET_ERROR_INVALID:
            break;
        default:
            return 3;
    }

    /* check the length */
    header = (struct PacketHeader *) packet;
    if(header->total_length != len)
        return 4;

    checksum = header->checksum;
    computed_checksum = packet_compute_checksum(header, len);
    if(checksum != computed_checksum)
    {
        err_output("computed checksum 0x%02x != 0x%02x for length %d\n", computed_checksum, checksum, len);
        return 5;
    }

    return 0;
}

/*
    compute the checksum of a packet by summing from the
    beginning to the end-1. The packet is modified. The checksum
    is also returned.
*/

uint8_t
packet_compute_checksum(struct PacketHeader *packet, size_t packet_len)
{
    uint8_t *ptr, *ptr_start, *ptr_end;
    uint32_t checksum;
    checksum = 0;

    /* compute checksum upto the checksum */
    ptr_start = (uint8_t *) packet;
    ptr_end = &packet->checksum;
    for(ptr = ptr_start; ptr < ptr_end; ptr++)
    {
        checksum += *ptr;
    }

    /* check the remaining part from the checksum to tne end of the data */
    ptr_start = &packet->checksum;
    ptr_start++; // go one past the checksum where the data is
    ptr_end = (uint8_t *) packet;
    ptr_end += packet_len;
    for(ptr = ptr_start; ptr < ptr_end; ptr++)
    {
        checksum += *ptr;
    }

    packet->checksum = (uint8_t) checksum;
    return packet->checksum;
}

uint8_t *
packet_get_data_pointer(struct PacketHeader *packet)
{
    // a zero length packet
    if(packet->total_length == sizeof(struct PacketHeader))
        return (uint8_t *) NULL;
    return (uint8_t *) packet+sizeof(struct PacketHeader);
}

uint8_t
packet_get_data_size(struct PacketHeader *packet)
{
    return packet->total_length - sizeof(struct PacketHeader);
}

int
packet_make_uninitialized_packet(struct PacketHeader **header, uint8_t data_length)
{
    uint8_t packet_bytes;
    packet_bytes = sizeof(struct PacketHeader)+data_length;
    *header = malloc(packet_bytes);
    if(*header == NULL)
    {
        errno_output("packet_make_uninitialized_packet: unable to allocate memory\n");
        return 1;
    }

    memset(*header, 0, packet_bytes);
    (*header)->total_length = packet_bytes;

    return 0;
}

void
packet_swap_source_dest(struct PacketHeader *packet)
{
    uint8_t temp_address[PACKET_ADDRESS_SIZE];
    uint8_t temp_port;

    memcpy(temp_address, packet->destination_address, PACKET_ADDRESS_SIZE);
    memcpy(packet->destination_address, packet->source_address, PACKET_ADDRESS_SIZE);
    memcpy(packet->source_address, temp_address, PACKET_ADDRESS_SIZE);

    temp_port = packet->destination_port;
    packet->destination_port = packet->source_port;
    packet->source_port = temp_port;
}

void
packet_make_partial(struct PacketHeader *packet, uint8_t type, uint8_t address_source[], uint8_t address_destination[], uint8_t source_port, uint8_t destination_port)
{
    packet->type = type;
    memcpy(packet->source_address, address_source, PACKET_ADDRESS_SIZE);
    memcpy(packet->destination_address, address_destination, PACKET_ADDRESS_SIZE);
    packet->source_port = source_port;
    packet->destination_port = destination_port;
}

void
packet_print(struct PacketHeader *packet)
{
    char fmt[] = "packet: %02x %02x%02x%02x%02x%02x%02x:%02x -> %02x%02x%02x%02x%02x%02x:%02x %02x %02x\n";

    debug_output(fmt,
        packet->type,
        packet->source_address[0],
        packet->source_address[1],
        packet->source_address[2],
        packet->source_address[3],
        packet->source_address[4],
        packet->source_address[5],
        packet->source_port,
        packet->destination_address[0],
        packet->destination_address[1],
        packet->destination_address[2],
        packet->destination_address[3],
        packet->destination_address[4],
        packet->destination_address[5],
        packet->destination_port,
        packet->total_length,
        packet->checksum
    );
}
