#include <string.h>
#include <assert.h>

#include "neighbor.h"
#include "connection.h"
#include "control.h"

int use_syslog;

int
test_control_neighborlist()
{
    struct List neighborlist;
    struct Neighbor *neighbor1, *neighbor2, *neighbor3;
    uint8_t address1[6], address2[6], address3[6];
    uint8_t *neighbor_buffer;
    size_t buf_len;

    list_init(&neighborlist, neighbor_match, neighbor_destroy);

    memset(address1, 0xaa, NEIGHBOR_ADDRESS_SIZE);
    memset(address2, 0xbb, NEIGHBOR_ADDRESS_SIZE);
    memset(address3, 0xcc, NEIGHBOR_ADDRESS_SIZE);

    neighbor1 = neighbor_make_uninitialzed(0, address1, connection_match, connection_destroy);
    neighbor2 = neighbor_make_uninitialzed(0, address2, connection_match, connection_destroy);
    neighbor3 = neighbor_make_uninitialzed(0, address3, connection_match, connection_destroy);

    /* test an empty neighbor list */
    control_get_neighbors(&neighborlist, &neighbor_buffer, &buf_len);
    assert(buf_len == 3);
    assert(neighbor_buffer[0] == CONTROL_RESPONSE_OK);
    assert(neighbor_buffer[1] == CONTROL_REQUEST_GET_NEIGHBORS);
    assert(neighbor_buffer[2] == 0);

    list_add_first(&neighborlist, neighbor3);
    list_add_first(&neighborlist, neighbor2);
    list_add_first(&neighborlist, neighbor1);

    /* test an empty neighbor list */

    control_get_neighbors(&neighborlist, &neighbor_buffer, &buf_len);
    assert(buf_len == 3*NEIGHBOR_ADDRESS_SIZE+3);
    assert(neighbor_buffer[0] == CONTROL_RESPONSE_OK);
    assert(neighbor_buffer[1] == CONTROL_REQUEST_GET_NEIGHBORS);
    assert(neighbor_buffer[2] == 3);
    assert(memcmp(&neighbor_buffer[3], address1, NEIGHBOR_ADDRESS_SIZE) == 0);
    assert(memcmp(&neighbor_buffer[9], address2, NEIGHBOR_ADDRESS_SIZE) == 0);
    assert(memcmp(&neighbor_buffer[15], address3, NEIGHBOR_ADDRESS_SIZE) == 0);

    free(neighbor_buffer);
    return 0;
}

int
test_control_get_my_address()
{
    uint8_t address1[6];
    uint8_t *address_buffer;
    size_t buf_len;

    memset(address1, 0xaa, NEIGHBOR_ADDRESS_SIZE);

    /* test an empty neighbor list */
    control_get_my_address(address1, &address_buffer, &buf_len);
    assert(buf_len == NEIGHBOR_ADDRESS_SIZE+2);
    assert(address_buffer[0] == CONTROL_RESPONSE_OK);
    assert(address_buffer[1] == CONTROL_REQUEST_GET_MY_ADDRESS);
    assert(memcmp(&address_buffer[2], address1, NEIGHBOR_ADDRESS_SIZE) == 0);

    free(address_buffer);
    return 0;
}

int
main(int argc, char *argv[])
{
    int err;
    err = test_control_neighborlist();
    assert(err == 0);

    err = test_control_get_my_address();
    assert(err == 0);

    return err;
}
