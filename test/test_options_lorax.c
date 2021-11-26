#include <assert.h>
#include <string.h>
#include "options_lorax.h"
#include "error.h"


struct OptionsTx opts;

int
main(int argc, char *argv[])
{
    int err;
    err = options_lorax_get_mac_address(&opts, "eth0");
    assert(strcmp(opts.iface_default, "eth0") == 0);

    for(int i=0; i<6; i++)
        assert(opts.mac_address[i]);

    assert(options_lorax_get_mac_address(&opts, "blah1234"));
    assert(strcmp(opts.iface_default, "blah1234"));

    return err;
}