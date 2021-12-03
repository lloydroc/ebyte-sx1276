#ifndef CONVERT_H
#define CONVERT_H

#include "message.h"
#include "packet.h"

int
message_to_packet(struct Message *message, struct PacketHeader** packet);

int
packet_to_message(struct PacketHeader *packet, struct Message** message);

#endif
