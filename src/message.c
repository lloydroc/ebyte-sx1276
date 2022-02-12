#include "message.h"

struct Message*
message_make_uninitialized_message(uint8_t *data, uint8_t len)
{
    struct Message *msg;
    msg = malloc(sizeof(struct Message)+len);
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
    struct Message *msg;
    int byte_len;
    if(len < sizeof(struct Message))
        return 1;
    if(message == NULL)
        return 2;

    msg = (struct Message *) message;
    switch(msg->type)
    {
        case MESSAGE_TYPE_DATA:
            break;
        default:
            return 3;
    }

    byte_len = message_total_length(msg)-sizeof(struct Message);
    if (byte_len != msg->data_length)
        return 4;

    return 0;
}

/*
 * Compare members except retries and type. We'll
 * set members we don't want to compare to temp values,
 * set them to zero, compare, then set them back.
 */
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
    char fmt[] = "message: %02x %02x %02x%02x%02x%02x%02x%02x:%02x -> %02x%02x%02x%02x%02x%02x:%02x %02x\n";

    debug_output(fmt,
        message->retries,
        message->type,
        message->source_address[0],
        message->source_address[1],
        message->source_address[2],
        message->source_address[3],
        message->source_address[4],
        message->source_address[5],
        message->source_port,
        message->destination_address[0],
        message->destination_address[1],
        message->destination_address[2],
        message->destination_address[3],
        message->destination_address[4],
        message->destination_address[5],
        message->destination_port,
        message->data_length
    );
}
