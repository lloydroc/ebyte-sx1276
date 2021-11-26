#include "convert.h"

int
message_to_packet(struct Message *message, struct PacketHeader** packet)
{
    packet_make_unitialized_packet(packet, message->data_length);
    packet_make_partial(*packet, message->type, message->source_address, message->destination_address, message->source_port, message->destination_port);
    memcpy(packet_get_data_pointer(*packet), message->data, message->data_length);
    packet_compute_checksum(*packet, (*packet)->total_length);

    return 0;
}

int
packet_to_message(struct PacketHeader *packet, struct Message** message)
{
    uint8_t* data_ptr;
    uint8_t data_size;

    data_ptr = packet_get_data_pointer(packet);
    data_size = packet_get_data_size(packet);

    *message = message_make_unitialized_packet(data_ptr, data_size);
    message_make_partial(*message, packet->type, packet->source_address, packet->destination_address, packet->source_port, packet->destination_port);

    return 0;
}
