#include "options.h"
#include "error.h"

struct options opts;

void
print_options()
{
    printf("options are: ");
    for(int i=0;i<6;i++)
        printf("%x", opts.settings_write_input[i]);
    puts("");
}

int
main(int argc, char *argv[])
{
    int err;
    char settings[32];

    // Test a good value
    sprintf(settings, "%s", "C000001A17B4");

    err = options_parse_settings(&opts, settings);

    print_options();

    if(opts.settings_write_input[0] != 0xC0)
        return 1;
    if(opts.settings_write_input[1] != 0x00)
        return 2;
    if(opts.settings_write_input[2] != 0x00)
        return 3;
    if(opts.settings_write_input[3] != 0x1A)
        return 4;
    if(opts.settings_write_input[4] != 0x17)
        return 5;
    if(opts.settings_write_input[5] != 0xB4)
        return 6;

    memset(opts.settings_write_input, 0, 6);
    argc = 3;
    argv[0] = strdup("test_options.c");
    argv[1] = strdup("-w");
    argv[2] = strdup("0000001A17B4");

    // Test without leading C
    sprintf(settings, "%s", "0000001A17B4");
    err = options_parse_settings(&opts, settings);

    if(err == 0)
        return 7;
    else
        err = 0;

    // Test invalid hex
    sprintf(settings, "%s", "C00000ZA17B4");
    err = options_parse_settings(&opts, settings);

    if(err == 0)
        return 8;
    else
        err = 0;

    // Test too short
    sprintf(settings, "%s", "C00000A17B4");
    err = options_parse_settings(&opts, settings);

    if(err == 0)
        return 9;
    else
        err = 0;

    // Test too long
    sprintf(settings, "%s", "C00000A17B4AA");
    err = options_parse_settings(&opts, settings);

    if(err == 0)
        return 10;
    else
        err = 0;


    return err;
}