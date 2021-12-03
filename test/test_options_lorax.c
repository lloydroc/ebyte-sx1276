#include <assert.h>
#include <string.h>
#include "options_lorax.h"
#include "error.h"


struct OptionsLorax opts;

int
main(int argc, char *argv[])
{
    int err;
    int sum;

    err = options_lorax_get_mac_address(&opts, "lo");
    assert(strcmp(opts.iface_default, "lo") == 0);

    for(int i=0; i<6; i++)
        assert(opts.mac_address[i] == 0);

    err = options_lorax_get_mac_address(&opts, "eth0");
    assert(strcmp(opts.iface_default, "eth0") == 0);

    // poor attempt to check the mac address matches
    // what it really is on the nic
    sum = 0;
    for(int i=0; i<6; i++)
        sum += opts.mac_address[i];
    assert(sum > 0);

    assert(options_lorax_get_mac_address(&opts, "blah1234"));
    assert(strcmp(opts.iface_default, "blah1234"));

    return err;
}
