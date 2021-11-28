#include "control.h"

int
control_request_invalid(uint8_t *control, size_t len)
{
    if(len != 1)
        return 1;

    switch(control[0])
    {
        case CONTROL_REQUEST_GET_NEIGHBORS:
        case CONTROL_REQUEST_GET_MY_ADDRESS:
            break;
        default:
            return 2;
    }

    return 0;
}

void
control_get_neighbors(struct List *neighbors, uint8_t **neighbor_buffer, size_t *buffer_len)
{
    uint8_t num_neighbors;
    uint8_t *buf_ptr;
    struct Neighbor *neighbor;
    num_neighbors = list_size(neighbors);
    *buffer_len = num_neighbors*NEIGHBOR_ADDRESS_SIZE+3;
    buf_ptr = malloc(*buffer_len);
    *neighbor_buffer = buf_ptr;
    buf_ptr[0] = CONTROL_RESPONSE_OK;
    buf_ptr[1] = CONTROL_REQUEST_GET_NEIGHBORS;
    buf_ptr[2] = num_neighbors;

    for(int i=0; i<num_neighbors; i++)
    {
        neighbor = (struct Neighbor *) list_get_index(neighbors, i);
        memcpy(buf_ptr+i*NEIGHBOR_ADDRESS_SIZE+3, neighbor->address, NEIGHBOR_ADDRESS_SIZE);
    }
}

void
control_get_my_address(uint8_t my_address[], uint8_t **neighbor_buffer, size_t *buffer_len)
{
    uint8_t *buf_ptr;
    *buffer_len = NEIGHBOR_ADDRESS_SIZE+2;
    buf_ptr = malloc(*buffer_len);
    *neighbor_buffer = buf_ptr;
    buf_ptr[0] = CONTROL_RESPONSE_OK;
    buf_ptr[1] = CONTROL_REQUEST_GET_MY_ADDRESS;
    memcpy(buf_ptr+2, my_address, NEIGHBOR_ADDRESS_SIZE);
}