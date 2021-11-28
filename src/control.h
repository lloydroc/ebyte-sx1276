#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>
#include <stdlib.h>
#include "list.h"
#include "neighbor.h"

#define CONTROL_REQUEST_GET_NEIGHBORS 1
#define CONTROL_REQUEST_GET_MY_ADDRESS 2

#define CONTROL_RESPONSE_OK 0
#define CONTROL_RESPONSE_ERROR 255

int
control_request_invalid(uint8_t *control, size_t len);

void
control_get_neighbors(struct List *neighbors, uint8_t **neighbor_buffer, size_t *buffer_len);

void
control_get_my_address(uint8_t my_address[], uint8_t **neighbor_buffer, size_t *buffer_len);

#endif