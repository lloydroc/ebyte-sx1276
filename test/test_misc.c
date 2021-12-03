#include <assert.h>
#include <stdio.h>
#include "misc.h"
#include "error.h"

int use_syslog;

int
main(int argc, char *argv[])
{
    struct timespec now_time;
    char *strtime;
    const char *addr1 = "dca632e8ed86";
    const char *addr2 = "dca632e8ed87";

    const uint8_t hex_addr1[6] = {0xdc, 0xa6, 0x32, 0xe8, 0xed, 0x86};
    const uint8_t hex_addr2[6] = {0xdc, 0xa6, 0x32, 0xe8, 0xed, 0x87};

    uint8_t test_addr1[6];
    uint8_t test_addr2[6];

    parse_mac_address(addr1, test_addr1);
    parse_mac_address(addr2, test_addr2);

    assert(memcmp(hex_addr1, test_addr1, 6) == 0);
    assert(memcmp(hex_addr2, test_addr2, 6) == 0);

    assert(clock_gettime(CLOCK_REALTIME, &now_time) == 0);
    strtime = rfc8601_timespec(&now_time);
    printf("time is %s\n", strtime);
    assert(strlen(strtime));

    int max_timeout_delay_ms = 3*2*100;
    for(int i=0; i<1000; i++)
    {
        int rando = get_random_timeout(2);
        assert(rando <= max_timeout_delay_ms);
    }

    return 0;

}