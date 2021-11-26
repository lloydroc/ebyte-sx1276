#include "message.h"

struct Message*
message_make_unitialized_packet(uint8_t *data, uint8_t len)
{
    struct Message *msg;
    msg = malloc(sizeof(struct Message)+len-1);
    memcpy(msg->data, data, len);
    msg->data_length = len;
    msg->retries = 1;
    return msg;
}

void
message_make_partial(struct Message *message, uint8_t type, uint8_t address_source[], uint8_t address_destination[], uint8_t source_port, uint8_t destination_port)
{
    message->type = type;
    memcpy(message->source_address, address_source, MESSAGE_ADDRESS_SIZE);
    memcpy(message->destination_address, address_destination, MESSAGE_ADDRESS_SIZE);
    message->source_port = source_port;
    message->destination_port = destination_port;
}

void
message_swap_source_dest(struct Message *message)
{
    uint8_t address_temp[MESSAGE_ADDRESS_SIZE];
    uint8_t port_temp;

    memcpy(address_temp, message->source_address, MESSAGE_ADDRESS_SIZE);
    memcpy(message->source_address, message->destination_address, MESSAGE_ADDRESS_SIZE);
    memcpy(message->destination_address, address_temp, MESSAGE_ADDRESS_SIZE);

    port_temp = message->source_port;
    message->source_port = message->destination_port;
    message->destination_port = port_temp;
}

int
message_invalid(uint8_t *message, size_t len)
{
    // TODO more checks
    if(len < sizeof(struct Message))
        return 1;
    if(message == NULL)
        return 2;
    return 0;
}

/* compare everything except retries and type */
int
message_match(void *m1, void *m2)
{
    struct Message *message1, *message2;
    int match;
    int temp1_retries, temp2_retries;
    int temp1_type, temp2_type;
    message1 = (struct Message *) m1;
    message2 = (struct Message *) m2;

    temp1_retries = message1->retries;
    temp2_retries = message2->retries;
    temp1_type = message1->type;
    temp2_type = message2->type;

    message1->retries = 0;
    message2->retries = 0;
    message1->type = 0;
    message2->type = 0;

    match = memcmp(message1, message2, message1->data_length+sizeof(struct Message)-1);

    message1->retries= temp1_retries;
    message2->retries = temp2_retries;
    message1->type = temp1_type;
    message2->type = temp2_type;

    return match;
}

size_t
message_total_length(struct Message *message)
{
    size_t len;
    len  = sizeof(struct Message)+message->data_length;
    return len;
}

void
message_destroy(void *data)
{
    struct Message *msg;
    msg = (struct Message*) data;
    //free(msg->data);
    free(msg);
}

void
message_print(struct Message *message)
{
    debug_output("%02x %02x ", message->retries, message->type);

    for(int i=0; i<MESSAGE_ADDRESS_SIZE; i++)
        debug_output("%02x", message->source_address[i]);

    debug_output(":%02x -> ", message->source_port);

    for(int i=0; i<MESSAGE_ADDRESS_SIZE; i++)
        debug_output("%02x", message->destination_address[i]);

    debug_output(":%02x ", message->destination_port);

    debug_output("%02x ", message->data_length);

    for(int i=0; i<message->data_length; i++)
        debug_output("%02x", message->data[i]);

    debug_output("\n");
}
