#ifndef LORAX_H
#define LORAX_H

#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "options_lorax.h"
#include "packet.h"
#include "list.h"
#include "neighbor.h"
#include "connection.h"
#include "message.h"
#include "convert.h"

int
lorax_e32_init(struct OptionsTx *opts);

int
lorax_e32_destroy();

int
lorax_e32_register(struct OptionsTx *opts);

int
lorax_e32_poll(struct OptionsTx *opts);

int
lorax_e32_process_packet(struct OptionsTx *opts, uint8_t *packet, size_t len);

int
lorax_join(struct OptionsTx *opts);

#endif