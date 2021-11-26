#include <assert.h>
#include "message.h"
#include "packet.h"
#include "convert.h"
#include "misc.h"
#include "error.h"

int use_syslog = 0;

int
main(int argc, char *argv[])
{
    struct Message *message;

    uint8_t source_addr[6] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    uint8_t destination_addr[6] = {0x22, 0x22, 0x22, 0x22, 0x22, 0x22};
    uint8_t data[3] = {'a', 'b', 'c'};

    message = malloc(sizeof(struct Message)+3); // 3 data
    message->retries = 1;
    message->type = 0;
    memcpy(message->source_address, source_addr, 6);
    memcpy(message->destination_address, destination_addr, 6);
    message->source_port = 1;
    message->destination_port = 1;
    message->data_length = 3;
    memcpy(message->data, data, 3);

    assert(message_total_length(message) == 20);

    struct PacketHeader *packet = NULL;

    message_to_packet(message, &packet);

    print_hex((uint8_t*) message, sizeof(struct Message)+3);
    print_hex((uint8_t*)packet, packet->total_length);
    assert(memcmp(message->source_address, packet->source_address, 6)==0);
    printf("0x%x\n", packet->checksum);
    assert(packet->checksum == 0x6e);
    assert(packet->total_length == 20);
    assert(packet->type == 0);

    return EXIT_SUCCESS;
}