#include "misc.h"

char *
rfc8601_timespec(struct timespec *tv)
{
  char time_str[127];
  double fractional_seconds;
  int milliseconds;
  struct tm tm; // our "broken down time"
  char *rfc8601;

  rfc8601 = malloc(256);

  memset(&tm, 0, sizeof(struct tm));
  sprintf(time_str, "%ld UTC", tv->tv_sec);

  // convert our timespec into broken down time
  strptime(time_str, "%s %U", &tm);

  // do the math to convert nanoseconds to integer milliseconds
  fractional_seconds = (double) tv->tv_nsec;
  fractional_seconds /= 1e6;
  milliseconds = (int) fractional_seconds;

  // print date and time withouth milliseconds
  strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S", &tm);

  // add on the fractional seconds and Z for the UTC Timezone
  sprintf(rfc8601, "%s.%dZ", time_str, milliseconds);

  return rfc8601;
}

/*
 * Convert bytes the hex in a buffer and print them out.
 * One byte is two hex chars so the buffer is double the size
 * plus a byte for the null character.
 */
void
print_hex(uint8_t *address, size_t len)
{
    char *buf;

    buf = malloc(len*2+1);
    buf[len*2] = '\0';
    for(int i=0; i<len; i++)
    {
        sprintf(buf+i*2, "%02x", *(address+i));
    }
    debug_output("%s\n", buf);

    free(buf);
}

int
get_mac_address(uint8_t mac_address[], char *iface)
{
    struct ifreq s;
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    strcpy(s.ifr_name, iface);
    if (0 == ioctl(fd, SIOCGIFHWADDR, &s))
    {
        memcpy(mac_address, s.ifr_addr.sa_data, 6);;
        return 0;
    }
    return 1;
}

int
parse_mac_address(const char *address, uint8_t hex_address[])
{
  int num_parsed;
  char hexbyte[3];
  uint8_t *ptr;

  if(strnlen(address, 13) != 12)
  {
    return 1;
  }

  ptr = hex_address;
  hexbyte[2] = '\0';
  for(int i=0; i<12; i=i+2)
  {
    hexbyte[0] = address[i];
    hexbyte[1] = address[i+1];
    num_parsed = sscanf(hexbyte, "%hhx", ptr++);
    if(num_parsed != 1)
    {
      return 2;
    }
  }

  return 0;
}

int
get_random_timeout(int num_neighbors)
{
  int slot, num_slots;
  int timeout_random_ms;

  if(num_neighbors < 2)
  {
    num_neighbors = 3;
  }

  num_slots = num_neighbors*3;
  slot = rand() % num_slots;
  timeout_random_ms = slot*1000;
  return timeout_random_ms;
}
