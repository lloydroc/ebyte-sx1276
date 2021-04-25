#include "../config.h"
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

  ret = e32_init(&dev, &opts);
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
    ret |= e32_set_mode(&dev, 0);
    goto cleanup;
  }
  if(opts.status)
  {
    if(e32_set_mode(&dev, 3))
    {
      err_output("unable to go to sleep mode\n");
      goto cleanup;
    }
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
    if(e32_set_mode(&dev, 0))
    {
      err_output("unable to go to normal mode\n");
      goto cleanup;
    }
    e32_print_version(&dev);
    e32_print_settings(&dev);
    goto cleanup;
  }
  else if(opts.daemon)
  {
    become_daemon();
    info_output("daemon started pid=%d", getpid());
  }

  ret |= e32_poll(&dev, &opts);
  if(ret)
    err_output("error polling %d", ret);
cleanup:
  ret |= e32_deinit(&dev, &opts);

  return ret;
}
