#include "config.h"
#include <stdio.h>

#include <signal.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "become_daemon.h"
#include "error.h"
#include "e32.h"
#include "gpio.h"
#include "options.h"

struct options opts;
struct E32 dev;

static
void signal_handler(int sig)
{
  int exit_status;

  if(opts.daemon)
    info_output("daemon stopping pid=%d", getpid());

  options_deinit(&opts);
  exit_status = e32_deinit(&dev, &opts);
  exit(exit_status);
}

int
main(int argc, char *argv[])
{
  int err = 0;

  if (signal(SIGINT, signal_handler) == SIG_ERR)
    err_output("installing SIGNT signal handler");
  if (signal(SIGTERM, signal_handler) == SIG_ERR)
    err_output("installing SIGTERM signal handler");

  options_init(&opts);
  err = options_parse(&opts, argc, argv);
  if(err || opts.help)
  {
    usage(argv[0]);
    return err;
  }

  if(opts.verbose)
  {
    options_print(&opts);
  }

  err = e32_init(&dev, &opts);
  if(err)
    goto cleanup;

  if(opts.mode != -1)
  {
    err |= e32_set_mode(&dev, opts.mode);
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
    err |= e32_set_mode(&dev, SLEEP);
    err |= e32_cmd_reset(&dev);
    err |= e32_set_mode(&dev, NORMAL);
    goto cleanup;
  }

  /* must be in sleep mode to read or write settings */
  if(opts.status || opts.settings_write_input[0])
  {
    if(e32_set_mode(&dev, SLEEP))
    {
      err_output("unable to go to sleep mode\n");
      goto cleanup;
    }
  }

  if(opts.status)
  {
    if(e32_cmd_read_version(&dev))
    {
      err_output("unable to read version\n");
      goto cleanup;
    }
    if(e32_cmd_read_settings(&dev))
    {
      err_output("unable to read settings\n");
      goto cleanup;
    }
    e32_print_version(&dev);
    e32_print_settings(&dev);
    goto cleanup;
  }

  if(opts.settings_write_input[0])
  {
    err |= e32_cmd_write_settings(&dev, &opts);
  }

  /* switch back to normal mode for tx/rx */
  if(e32_set_mode(&dev, NORMAL))
  {
    err_output("unable to go to normal mode\n");
    goto cleanup;
  }

  if(opts.daemon)
  {
    become_daemon();
    info_output("daemon started pid=%d", getpid());
  }

  err |= e32_poll(&dev, &opts);
  if(err)
    err_output("error polling %d", err);
cleanup:
  err |= e32_deinit(&dev, &opts);

  return err;
}
