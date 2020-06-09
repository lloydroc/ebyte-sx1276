#include "e32.h"

static int
e32_init_gpio(struct options *opts, struct E32 *dev)
{
  if(gpio_exists())
    return 1;

  if(gpio_valid(opts->gpio_m0))
    return 2;

  if(gpio_valid(opts->gpio_m1))
    return 3;

  if(gpio_valid(opts->gpio_aux))
    return 4;

  if(gpio_export(opts->gpio_m0))
    return 5;

  if(gpio_export(opts->gpio_m1))
    return 6;

  if(gpio_export(opts->gpio_aux))
    return 6;

  if(gpio_set_output(opts->gpio_m0))
    return 7;

  if(gpio_set_output(opts->gpio_m1))
    return 8;

  if(gpio_set_input(opts->gpio_aux))
    return 9;

  if(gpio_set_edge_rising(opts->gpio_aux))
    return 10;

  dev->gpio_m0_fd = gpio_open_output(opts->gpio_m0);
  if(dev->gpio_m0_fd == -1)
    return 11;

  dev->gpio_m1_fd = gpio_open_output(opts->gpio_m1);
  if(dev->gpio_m1_fd == -1)
    return 12;

  dev->gpio_aux_fd = gpio_open_input(opts->gpio_aux);
  if(dev->gpio_aux_fd == -1)
    return 13;

  return 0;
}

static int
e32_init_uart(struct E32 *dev)
{
  dev->uart_fd = uart_open();
  return dev->uart_fd;
}

int
e32_init(struct options *opts, struct E32 *dev)
{
  int ret;

  ret = e32_init_gpio(opts, dev);

  if(ret)
    return ret;

  ret = e32_init_uart(dev);
  if(ret == -1)
    return ret;

  return 0;
}

static int
e32_aux_poll(struct E32 *dev)
{
  int timeout;
  struct pollfd pfd;
  int fd;
  int ret;
  char buf[2];

  // we will wait 100ms+10ms buffer
  timeout = 110;

  fd = dev->gpio_aux_fd;

  pfd.fd = fd;
  pfd.events = POLLPRI;

  ret = poll(&pfd, 1, timeout);
  if(ret == 0)
  {
    fprintf(stderr, "poll timed out\n");
    return 1;
  }
  else if (ret < 0)
  {
    err_output("poll");
    return 2;
  }

  lseek(fd, 0, SEEK_SET);
  read(fd, buf, sizeof(buf));
  // TODO WTH??
  printf("here\n");
  close(fd);

  return 0;
}

int
e32_set_mode(struct E32 *dev, int mode)
{
  int ret;

  if(e32_get_mode(dev))
  {
    return 1;
  }

  int m0 = mode & 0x01;
  int m1 = mode & 0x02;

  ret  = gpio_write(dev->gpio_m0_fd, m0) != 1;
  ret |= gpio_write(dev->gpio_m1_fd, m1) != 1;

  if(!ret)
  {
    dev->prev_mode = dev->mode;
    dev->mode = mode;
  }
  else
    return ret;

  if(dev->prev_mode == 3 && dev->mode != 3)
  {
    if(e32_aux_poll(dev))
      return 2;
  }

  return ret;
}

int
e32_get_mode(struct E32 *dev)
{
  int ret;

  char m0, m1;

  ret  = gpio_read(dev->gpio_m0_fd, &m0) != 2;
  ret |= gpio_read(dev->gpio_m1_fd, &m1) != 2;

  if(ret)
    return 1;

  m1 <<= 1;
  dev->mode = m0+m1;

  return ret;
}

int
e32_deinit(struct options* opts, struct E32 *dev)
{
  int ret;
  ret = 0;
/*
  ret |= gpio_close(dev->gpio_m0_fd);
  ret |= gpio_close(dev->gpio_m1_fd);
  ret |= gpio_close(dev->gpio_aux_fd);

  ret |= gpio_unexport(opts->gpio_m0);
  ret |= gpio_unexport(opts->gpio_m1);
  ret |= gpio_unexport(opts->gpio_aux);
*/

  ret |= close(dev->uart_fd);
  return ret;
}


int
e32_cmd_read_settings(struct E32 *dev)
{
  ssize_t bytes;
  const uint8_t cmd[3] = {0xC1, 0xC1, 0xC1};
  uint8_t buf[6];
  bytes = write(dev->uart_fd, cmd, 3);
  if(bytes == -1)
   return -1;

  bytes = read(dev->uart_fd, buf, 6);
  if(bytes == -1)
    return -1;

  if(buf[0] != 0xC0 && buf[0] != 0xC2)
    return -2;

  dev->power_down_save = buf[0] == 0xC0;
  dev->addh = buf[1];
  dev->addl = buf[2];
  dev->parity = buf[3] & 0b11000000;
  dev->parity >>= 6;
  dev->uart_baud = buf[3] & 0b00111000;
  dev->uart_baud >>= 3;
  switch(dev->uart_baud)
  {
    case 0:
      dev->uart_baud = 1200;
      break;
    case 1:
      dev->uart_baud = 2400;
      break;
    case 3:
      dev->uart_baud = 9600;
      break;
    case 4:
      dev->uart_baud = 19200;
      break;
    case 5:
      dev->uart_baud = 38400;
      break;
    case 6:
      dev->uart_baud = 56700;
      break;
    case 7:
      dev->uart_baud = 115200;
      break;
    default:
      dev->uart_baud = 0;
  }

  dev->air_data_rate = buf[3] & 0b00000111;
  switch(dev->air_data_rate)
  {
    case 0:
      dev->air_data_rate = 300;
      break;
    case 1:
      dev->air_data_rate = 1200;
      break;
    case 2:
      dev->air_data_rate = 2400;
      break;
    case 3:
      dev->air_data_rate = 4800;
      break;
    case 4:
      dev->air_data_rate = 9600;
      break;
    case 5:
      dev->air_data_rate = 19200;
    case 6:
      dev->air_data_rate = 19200;
    case 7:
      dev->air_data_rate = 19200;
      break;
    default:
      dev->air_data_rate = 0;
  }

  dev->channel = buf[4] & 0b11100000;
  dev->channel >>= 5;
  dev->channel += 410;

  dev->transmission_mode = buf[5] & 0b10000000;
  dev->transmission_mode >>= 7;

  dev->io_drive = buf[5] & 0b01000000;
  dev->io_drive >>= 6;

  dev->wireless_wakeup_time = buf[5] & 0b00111000;
  dev->wireless_wakeup_time >>= 5;

  switch(dev->wireless_wakeup_time)
  {
    case 0:
      dev->wireless_wakeup_time = 250;
      break;
    case 1:
      dev->wireless_wakeup_time = 500;
      break;
    case 2:
      dev->wireless_wakeup_time = 750;
      break;
    case 3:
      dev->wireless_wakeup_time = 1000;
      break;
    case 4:
      dev->wireless_wakeup_time = 1250;
      break;
    case 5:
      dev->wireless_wakeup_time = 1500;
      break;
    case 6:
      dev->wireless_wakeup_time = 1750;
      break;
    case 7:
      dev->wireless_wakeup_time = 2000;
      break;
    default:
      dev->wireless_wakeup_time = 0;
  }

  dev->fec = buf[5] & 0b00000100;
  dev->fec >>= 1;

  dev->tx_power_dbm = buf[5] & 0b00000011;
  switch(dev->tx_power_dbm)
  {
    case 0:
      dev->tx_power_dbm = 30;
      break;
    case 1:
      dev->tx_power_dbm = 27;
      break;
    case 2:
      dev->tx_power_dbm = 24;
      break;
    case 3:
      dev->tx_power_dbm = 21;
      break;
    default:
      dev->tx_power_dbm = 0;
  }

  if(e32_aux_poll(dev))
    return 2;

  return 0;
}

int
e32_cmd_read_operating(struct E32 *dev)
{
  return -1;
}

int
e32_cmd_read_version(struct E32 *dev)
{
  ssize_t bytes;
  const uint8_t cmd[3] = {0xC3, 0xC3, 0xC3};
  uint8_t ver_buf[4];
  bytes = write(dev->uart_fd, cmd, 3);
  if(bytes == -1)
   return -1;

  bytes = read(dev->uart_fd, ver_buf, 4);
  if(ver_buf[0] != 0xC3)
    return -1;

  switch(ver_buf[1])
  {
    case 0x32:
      dev->frequency_mhz = 433;
      break;
    case 0x38:
      dev->frequency_mhz = 470;
      break;
    case 0x45:
      dev->frequency_mhz = 868;
      break;
    case 0x44:
      dev->frequency_mhz = 915;
      break;
    case 0x46:
      dev->frequency_mhz = 170;
      break;
    default:
      dev->frequency_mhz = 0;
  }

  dev->version = ver_buf[2];
  dev->features = ver_buf[3];

  if(e32_aux_poll(dev))
    return 2;

  return bytes != 4;
}

void
e32_print_version(struct E32 *dev)
{
  printf("frequency [MHz]: %d, version: %d, features %d\n", dev->frequency_mhz, dev->version, dev->features);
}

int
e32_cmd_reset(struct E32 *dev)
{
  ssize_t bytes;
  const uint8_t cmd[3] = {0xC4, 0xC4, 0xC4};
  bytes = write(dev->uart_fd, cmd, 3);
  if(bytes != 3)
    return 1;

  return e32_aux_poll(dev);
}

int
e32_cmd_write_settings(struct E32 *dev)
{
  return -1;
}

int
e32_transmit(struct E32 *dev, uint8_t *buf, uint8_t buf_len)
{
  ssize_t bytes;
  bytes = write(dev->uart_fd, buf, buf_len);
  if(bytes != buf_len)
    return 1;

  return e32_aux_poll(dev);
}
