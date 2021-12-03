#ifndef MISC_H
#define MISC_H

#define _GNU_SOURCE
#include <linux/if.h>
#include <socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>

#include "error.h"

char *
rfc8601_timespec(struct timespec *tv);

void
print_hex(uint8_t *address, size_t len);

int
get_mac_address(uint8_t mac_address[], char *iface);

int
parse_mac_address(const char *address, uint8_t hex_address[]);

int
get_random_timeout(int num_neighbors);

#endif
