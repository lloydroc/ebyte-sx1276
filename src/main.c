#include "../config.h"
#include <stdio.h>

#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include "error.h"
#include "become_daemon.h"
#include "options.h"
#include "gpio.h"
#include "e32.h"

struct options opts;
struct E32 dev;

static
void signal_handler(int sig)
{
  int exit_status;
  exit_status = e32_deinit(&opts, &dev);
  exit(exit_status);
}

int
main(int argc, char *argv[])
{
  int ret = 0;

  if (signal(SIGINT, signal_handler) == SIG_ERR)
    err_output("installing SIGNT signal handler");
  if (signal(SIGTERM, signal_handler) == SIG_ERR)
    err_output("installing SIGTERM signal handler");

  options_init(&opts);
  ret = options_parse(&opts, argc, argv);
  if(ret || opts.help)
  {
    usage(argv[0]);
    return ret;
  }
  else if(opts.help)
  {
    usage(argv[0]);
    return EXIT_SUCCESS;
  }

  ret = e32_init(&opts, &dev);
  if(ret)
    goto cleanup;

  if(opts.mode != -1)
  {
    ret |= e32_set_mode(&dev, opts.mode);
    goto cleanup;
  }

  if(opts.test)
  {
    uint8_t buf[512];
    for(int i=0; i<512; i++) buf[i] = i;
    e32_transmit(&dev, buf, 512);
    goto cleanup;
  }
  if(opts.reset)
  {
    ret |= e32_set_mode(&dev, 3);
    ret |= e32_cmd_reset(&dev);
    goto cleanup;
  }
  if(opts.status)
  {
    if(e32_set_mode(&dev, 3))
    {
      fprintf(stderr, "unable to go to sleep mode\n");
      goto cleanup;
    }
    if(e32_cmd_read_version(&dev))
    {
      fprintf(stderr, "unable to read version\n");
      goto cleanup;
    }
    if(e32_cmd_read_settings(&dev))
    {
      fprintf(stderr, "unable to read settings\n");
      goto cleanup;
    }
    e32_print_version(&dev);
    e32_print_settings(&dev);
    goto cleanup;
  }

  /*if(lsm9ds1_configure_ag_interrupt(opts.gpio_interrupt_ag, &fd))
  {
    fprintf(stderr, "unable to configure interrupt %d\n", opts.gpio_interrupt_ag);
    return EXIT_FAILURE;
  }*/

/*
  lsm9ds1_init(&dev);

  if(opts.reset)
  {
    lsm9ds1_reset(&dev);
    ret = EXIT_SUCCESS;
    goto cleanup;
  }
  else if(opts.configure)
  {
    lsm9ds1_configure(&dev);
    ret = lsm9ds1_test(&dev);
    goto cleanup;
  }
  else if(opts.daemon)
  {
    if(opts.data_file == stdout)
      opts.data_file = NULL;
    become_daemon();
  }

  ret = lsm9ds1_test(&dev);
  if(ret)
  {
    fprintf(stderr, "Failed %d test cases\n", ret);
    ret = EXIT_FAILURE;
    goto cleanup;
  }
  else
  {
    // skip test since we did it above
    if(opts.test)
    {
      ret = EXIT_SUCCESS;
      goto cleanup;
    }
  }

  lsm9ds1_ag_poll(&dev, &opts);

*/
  e32_poll(&dev, &opts);
cleanup:
  ret |= e32_deinit(&opts, &dev);

  return ret;
}
