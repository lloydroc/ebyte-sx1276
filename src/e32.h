#ifndef E32_DEF
#define E32_DEF

#include <poll.h>
#include <sys/time.h>
#include "options.h"
#include "gpio.h"
#include "uart.h"

struct E32
{
  int gpio_m0_fd;
  int gpio_m1_fd;
  int gpio_aux_fd;
  int uart_fd;
  int prev_mode;
  int mode;
  int frequency_mhz;
  int version;
  int features;
  int power_down_save;
  int addh;
  int addl;
  int parity;
  int uart_baud;
  int air_data_rate;
  int option;
  int channel;
  int transmission_mode;
  int io_drive;
  int wireless_wakeup_time;
  int fec;
  int tx_power_dbm;
};

enum E32_mode
{
  normal,
  wake_up,
  power_save,
  sleep_mode
};

int
e32_init(struct options *opts, struct E32 *dev);

int
e32_deinit(struct options *opts, struct E32 *dev);

int
e32_set_mode(struct E32 *dev, int mode);

int
e32_get_mode(struct E32 *dev);

int
e32_cmd_read_settings(struct E32 *dev);

void
e32_print_settings(struct E32 *dev);

int
e32_cmd_read_operating(struct E32 *dev);

int
e32_cmd_read_version(struct E32 *dev);

void
e32_print_version(struct E32 *dev);

int
e32_cmd_reset(struct E32 *dev);

int
e32_cmd_write_settings(struct E32 *dev);

int
e32_transmit(struct E32 *dev, uint8_t *buf, uint8_t buf_len);

int
e32_receive(struct E32 *dev, uint8_t *buf, uint8_t buf_len);

#endif
