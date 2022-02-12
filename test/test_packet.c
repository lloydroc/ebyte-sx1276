#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "packet.h"
#include "misc.h"
#include "error.h"

int use_syslog;

int
main(int argc, char *argv[])
{
  use_syslog = 0;
  struct PacketHeader *packet;
  uint8_t source_address[6], destination_address[6];
  uint8_t packet_len, data_len;
  uint8_t checksum;
  uint8_t *packet_data;
  data_len = 30;
  packet_len = sizeof(struct PacketHeader)+data_len;

  packet = malloc(packet_len);
  memset(packet, 0, sizeof(packet_len));

  // check a zero'd out packet has checksum 0
  assert(packet_compute_checksum(packet, 0) == 0);
  free(packet);

  memset(source_address, 1, 6);
  memset(destination_address, 2, 6);

  /* make a packet with no data */
  assert(packet_make_uninitialized_packet(&packet, 0) == 0);
  assert(0 == packet_get_data_size(packet));
  packet_make_partial(packet, PACKET_DATA, source_address, destination_address, 100, 200);

  assert(packet->type == PACKET_DATA);
  assert(packet->total_length == sizeof(struct PacketHeader));
  assert(memcmp(source_address, packet->source_address, 6) == 0);
  assert(memcmp(destination_address, packet->destination_address, 6) == 0);
  assert(100 == packet->source_port);
  assert(200 == packet->destination_port);

  uint32_t sum = PACKET_DATA+6+12+100+200+sizeof(struct PacketHeader);
  sum &= 0xff;
  assert(packet_compute_checksum(packet, 0) == sum);
  assert(sum == packet->checksum);

  free(packet);

  /* make a packet with some data in it */
  assert(packet_make_uninitialized_packet(&packet, data_len) == 0);
  packet_make_partial(packet, PACKET_DATA, source_address, destination_address, 100, 200);

  uint8_t *data_ptr = (uint8_t *) &packet->checksum+1; // TODO make a function for this
  memset(data_ptr, 1, data_len); // set data to all 1's
  assert(data_ptr == packet_get_data_pointer(packet));
  assert(data_len == packet_get_data_size(packet));

  assert(packet->type == PACKET_DATA);
  assert(packet->total_length == packet_len);
  assert(memcmp(source_address, packet->source_address, 6) == 0);
  assert(memcmp(destination_address, packet->destination_address, 6) == 0);
  assert(100 == packet->source_port);
  assert(200 == packet->destination_port);

  sum = PACKET_DATA+6+12+100+200+packet_len+30;
  sum &= 0xff;
  printf("cksu %d %d\n", packet_compute_checksum(packet, data_len), sum);
  assert(packet_compute_checksum(packet, packet_len) == sum);
  assert(sum == packet->checksum);
  assert(0 == packet_invalid((uint8_t *) packet, packet_len));

  /* test packet_invalid */
  packet->type = 0xF1;
  assert(3 == packet_invalid((uint8_t *) packet, packet_len));

  packet->type = PACKET_DATA;
  assert(4 == packet_invalid((uint8_t *) packet, packet_len+1));
  free(packet);

  /* make a broadcast packet 0x01dca632e8ed86ffffffffffff00000000141700 */
  /* 0x01b827eb1b8c0affffffffffff00000000148300
     0x01b827eb1b8c0affffffffffff00000000149300
  */
  data_len = 1;
  packet_len = sizeof(struct PacketHeader)+data_len;
  assert(packet_make_uninitialized_packet(&packet, data_len) == 0);
  assert(packet_len == packet->total_length);

  uint8_t source_address_real[6] = { 0xdc, 0xa6, 0x32, 0xe8, 0xed, 0x86};
  uint8_t broadcast_address[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  packet_make_partial(packet, PACKET_HEADER_BROADCAST, source_address_real, broadcast_address, 0, 0);
  assert(packet->type == PACKET_HEADER_BROADCAST);
  packet_data = packet_get_data_pointer(packet);
  *packet_data = 3;
  assert(packet_compute_checksum(packet, packet->total_length));

  checksum = packet->checksum;
  packet_data = (uint8_t *) packet;
  print_hex(packet_data, packet_len);

  printf("checksum %d 0x%02x\n", checksum, checksum);
  assert(packet->type == PACKET_HEADER_BROADCAST);

  return 0;
}
