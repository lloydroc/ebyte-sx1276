#include "convert.h"
#include "assert.h"

int use_syslog = 0;

#define DATA_LEN 20

int
main(int argc, char *argv[])
{

    struct Message *message, *message2;
    struct PacketHeader *packet, *packet2;

    uint8_t source_address[6], destination_address[6];
    uint8_t data[DATA_LEN];

    memset(source_address, 0xaa, PACKET_ADDRESS_SIZE);
    memset(destination_address, 0xbb, PACKET_ADDRESS_SIZE);
    memset(data, 0x00, DATA_LEN);

    message = message_make_uninitialized_message(data, DATA_LEN);
    message_make_partial(message, MESSAGE_TYPE_DATA, source_address, destination_address, 0, 0);

    assert(memcmp(source_address, message->source_address, 6) == 0);
    assert(memcmp(destination_address, message->destination_address, 6) == 0);
    assert(message->data_length == DATA_LEN);
    assert(message->data_length == (message_total_length(message)-sizeof(struct Message)));

    // convert a message to a packet
    message_to_packet(message, &packet);

    assert(memcmp(packet->source_address, message->source_address, 6) == 0);
    assert(memcmp(packet->destination_address, message->destination_address, 6) == 0);
    assert(packet_get_data_size(packet) == DATA_LEN);

    // convert a packet to another message
    packet_to_message(packet, &message2);

    assert(memcmp(packet->source_address, message2->source_address, 6) == 0);
    assert(memcmp(packet->destination_address, message2->destination_address, 6) == 0);
    assert(message2->data_length == DATA_LEN);

    // convert the message to another packet
    message_to_packet(message2, &packet2);
    assert(memcmp(packet2->source_address, message2->source_address, 6) == 0);
    assert(memcmp(packet2->destination_address, message2->destination_address, 6) == 0);
    assert(packet_get_data_size(packet2) == DATA_LEN);

    assert(((void *)message) != ((void *)packet));
    assert(((void *)message) != ((void *)message2));
    assert(((void *)packet) != ((void *)packet2));
    assert(((void *)packet2) != ((void *)message2));

    return 0;
}