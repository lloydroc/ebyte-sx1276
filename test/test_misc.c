#include <assert.h>
#include "misc.h"
#include "error.h"

int use_syslog;

int
main(int argc, char *argv[])
{
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

    return 0;

}