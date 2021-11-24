#include "e32.h"

uint8_t txbuf[TX_BUF_BYTES];
uint8_t rxbuf[RX_BUF_BYTES];

 /* define these indices so we don't have to reference pfd[3] */
#define PFD_STDIN 0
#define PFD_UART 1
#define PFD_INPUT_FILE 2
#define PFD_SOCKET_UNIX_DATA 3
#define PFD_GPIO_AUX 4
#define PFD_SOCKET_UNIX_CONTROL 5

static int
e32_init_gpio(struct options *opts, struct E32 *dev)
{
  int inputs[64], outputs[64];
  int ninputs, noutputs;
  int hm0=0, hm1=0, haux=0;

  if(gpio_permissions_valid())
    return -1;

  if(gpio_exists())
    return 1;

  if(gpio_valid(opts->gpio_m0))
    return 2;

  if(gpio_valid(opts->gpio_m1))
    return 3;

  if(gpio_valid(opts->gpio_aux))
    return 4;

  /* check if gpio is already set */
  if(gpio_get_exports(inputs, outputs, &ninputs, &noutputs))
    return 5;

  for(int i=0; i<noutputs; i++)
  {
    if(outputs[i] == opts->gpio_m0)
      hm0 = 1;
    if(outputs[i] == opts->gpio_m1)
      hm1 = 1;
  }

  for(int i=0; i<ninputs; i++)
  {
    if(inputs[i] == opts->gpio_aux)
      haux = 1;
  }

  if(!hm0)
  {
    if(gpio_export(opts->gpio_m0))
      return 6;

    if(gpio_set_output(opts->gpio_m0))
      return 7;
  }

  if(!hm1)
  {
    if(gpio_export(opts->gpio_m1))
      return 7;

    if(gpio_set_output(opts->gpio_m1))
      return 8;
  }

  if(!haux)
  {
    if(gpio_export(opts->gpio_aux))
      return 9;

    if(gpio_set_input(opts->gpio_aux))
      return 10;

    if(gpio_set_edge_both(opts->gpio_aux))
      return 11;
  }

  int edge;
  if(gpio_get_edge(opts->gpio_aux, &edge))
    return 12;

  if(edge != 3)
    if(gpio_set_edge_both(opts->gpio_aux))
        return 13;

  dev->fd_gpio_m0 = gpio_open(opts->gpio_m0);
  if(dev->fd_gpio_m0 == -1)
    return 14;

  dev->fd_gpio_m1 = gpio_open(opts->gpio_m1);
  if(dev->fd_gpio_m1 == -1)
    return 15;

  dev->fd_gpio_aux = gpio_open(opts->gpio_aux);
  if(dev->fd_gpio_aux == -1)
    return 16;

  return 0;
}

static int
e32_init_uart(struct E32 *dev, char *tty_name)
{
  int ret;
  ret = tty_open(tty_name, &dev->uart_fd, &dev->tty);

  if(dev->verbose)
  {
    debug_output("opened %s\n", tty_name);
  }
  return ret;
}

static int
socket_match(void *a, void *b)
{
  struct sockaddr_un *socka;
  struct sockaddr_un *sockb;

  socka = (struct sockaddr_un*) a;
  sockb = (struct sockaddr_un*) b;

  return strcmp(socka->sun_path, sockb->sun_path);
}

static void
socket_free(void *socket)
{
  free(socket);
}

int
e32_init(struct E32 *dev, struct options *opts)
{
  int ret;

  dev->verbose = opts->verbose;
  dev->socket_list = NULL;

  ret = e32_init_gpio(opts, dev);

  if(ret)
    return ret;

  ret = e32_init_uart(dev, opts->tty_name);
  if(ret == -1)
    return ret;

  dev->prev_mode = -1;

  dev->socket_list = calloc(1, sizeof(struct List));
  list_init(dev->socket_list, socket_match, socket_free);

  dev->state = IDLE;
  dev->isatty = 0;

  return 0;
}

int
e32_set_mode(struct E32 *dev, int mode)
{
  int ret;

  if(e32_get_mode(dev))
  {
    err_output("unable to get mode\n");
    return 1;
  }

  dev->prev_mode = dev->mode;
  dev->mode = mode;

  if(dev->mode == dev->prev_mode)
  {
    if(dev->verbose)
      debug_output("mode %d unchanged\n", mode);
    return 0;
  }

  int m0 = mode & 0x01;
  int m1 = mode & 0x02;
  m1 >>= 1;

  ret  = gpio_write(dev->fd_gpio_m0, m0) != 1;
  ret |= gpio_write(dev->fd_gpio_m1, m1) != 1;

  if(ret)
  {
    return ret;
  }

  if(dev->verbose)
    debug_output("new mode %d, prev mode is %d\n", dev->mode, dev->prev_mode);

  if(dev->prev_mode != dev->mode)
  {
    usleep(20000);
  }

  return ret;
}

int
e32_get_mode(struct E32 *dev)
{
  int ret;

  int m0, m1;

  ret  = gpio_read(dev->fd_gpio_m0, &m0) != 2;
  ret |= gpio_read(dev->fd_gpio_m1, &m1) != 2;

  if(ret)
    return 1;

  m1 <<= 1;
  dev->mode = m0+m1;

  if(dev->verbose)
    debug_output("read mode %d\n", dev->mode);

  return ret;
}

int
e32_deinit(struct E32 *dev, struct options* opts)
{
  int ret;
  ret = 0;
/*
  ret |= gpio_close(dev->fd_gpio_m0);
  ret |= gpio_close(dev->fd_gpio_m1);
  ret |= gpio_close(dev->fd_gpio_aux);

  ret |= gpio_unexport(opts->gpio_m0);
  ret |= gpio_unexport(opts->gpio_m1);
  ret |= gpio_unexport(opts->gpio_aux);
*/


  ret |= close(dev->uart_fd);

  if(dev->socket_list != NULL)
  {
    list_destroy(dev->socket_list);
    free(dev->socket_list);
  }

  return ret;
}

static int
e32_read_uart(struct E32* dev, uint8_t buf[], int n_bytes)
{
  int bytes, bytes_read;
  uint8_t *ptr;

  bytes = bytes_read = 0;
  ptr = buf;

  do
  {
    bytes = read(dev->uart_fd, ptr, n_bytes);
    if(bytes == 0)
    {
      errno_output("timed out\n");
      return 1;
    }
    else if(bytes == -1)
    {
      errno_output("reading uart\n");
      return 2;
    }

    bytes_read += bytes;
    if(bytes_read > n_bytes)
    {
      err_output("overrun expected %d bytes but read %d\n", n_bytes, bytes_read);
      return 3;
    }

    ptr += bytes;

  }
  while (bytes_read < n_bytes);

  return 0;

}

int
e32_cmd_read_settings(struct E32 *dev)
{
  ssize_t bytes;
  int err;
  const uint8_t cmd[3] = {0xC1, 0xC1, 0xC1};

  if(dev->verbose)
    debug_output("sending command to read settings\n");

  bytes = write(dev->uart_fd, cmd, 3);
  if(bytes == -1)
   return -1;

  if(dev->verbose)
    debug_output("reading settings\n");

  // set a .5 second timout
  tty_set_read_with_timeout(dev->uart_fd, &dev->tty, 5);
  err = e32_read_uart(dev, dev->settings, sizeof(dev->settings));
  if(err)
  {
    return err;
  }

  if(dev->settings[0] != 0xC0 && dev->settings[0] != 0xC2)
    return -2;

  dev->power_down_save = dev->settings[0] == 0xC0;
  dev->addh = dev->settings[1];
  dev->addl = dev->settings[2];
  dev->parity = dev->settings[3] & 0b11000000;
  dev->parity >>= 6;
  dev->uart_baud = dev->settings[3] & 0b00111000;
  dev->uart_baud >>= 3;
  switch(dev->uart_baud)
  {
    case 0:
      dev->uart_baud = 1200;
      break;
    case 1:
      dev->uart_baud = 2400;
      break;
    case 2:
      dev->uart_baud = 4800;
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

  dev->air_data_rate = dev->settings[3] & 0b00000111;
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

  dev->channel = dev->settings[4] & 0b00011111;

  dev->transmission_mode = dev->settings[5] & 0b10000000;
  dev->transmission_mode >>= 7;

  dev->io_drive = dev->settings[5] & 0b01000000;
  dev->io_drive >>= 6;

  dev->wireless_wakeup_time = dev->settings[5] & 0b00111000;
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

  dev->fec = dev->settings[5] & 0b00000100;
  dev->fec >>= 2;

  dev->tx_power_dbm = dev->settings[5] & 0b00000011;
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

  usleep(54000);

  return 0;
}

void
e32_print_settings(struct E32 *dev)
{
  info_output("Settings Raw Value:       0x");
  for(int i=0; i<6; i++) info_output("%02x", dev->settings[i]);
  info_output("\n");

  if(dev->power_down_save)
    info_output("Power Down Save:          Save parameters on power down\n");
  else
    info_output("Power Down Save:          Discard parameters on power down\n");

  info_output("Address:                  0x%02x%02x\n", dev->addh, dev->addl);

  switch(dev->parity)
  {
  case 0:
    info_output("Parity:                   8N1\n");
    break;
  case 1:
    info_output("Parity:                   8O1\n");
    break;
  case 2:
    info_output("Parity:                   8E1\n");
    break;
  case 3:
    info_output("Parity:                   8N1\n");
    break;
  default:
    info_output("Parity:                   Unknown\n");
    break;
  }

  info_output("UART Baud Rate:           %d bps\n", dev->uart_baud);
  info_output("Air Data Rate:            %d bps\n", dev->air_data_rate);
  info_output("Channel:                  %d\n", dev->channel);
  info_output("Frequency                 %d MHz\n", dev->channel+410);

  if(dev->transmission_mode)
    info_output("Transmission Mode:        Transparent\n");
  else
    info_output("Transmission Mode:        Fixed\n");

  if(dev->io_drive)
    info_output("IO Drive:                 TXD and AUX push-pull output, RXD pull-up input\n");
  else
    info_output("IO Drive:                 TXD and AUX open-collector output, RXD open-collector input\n");

  info_output("Wireless Wakeup Time:     %d ms\n", dev->wireless_wakeup_time);

  if(dev->fec)
    info_output("Forward Error Correction: on\n");
  else
    info_output("Forward Error Correction: off\n");

  info_output("TX Power:                 %d dBm\n", dev->tx_power_dbm);
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
  int err;
  const uint8_t cmd[3] = {0xC3, 0xC3, 0xC3};

  if(dev->verbose)
    debug_output("writing version command\n");

  bytes = write(dev->uart_fd, cmd, 3);
  if(bytes == -1)
   return -1;

  if(dev->verbose)
    debug_output("reading version\n");

  // set a .5 second timout
  tty_set_read_with_timeout(dev->uart_fd, &dev->tty, 5);

  err = e32_read_uart(dev, dev->version, sizeof(dev->version));
  if(err)
  {
    return err;
  }

  if(dev->version[0] != 0xC3)
  {
    err_output("mismatch 0x%02x != 0xc3\n", dev->version[0]);
    return -1;
  }

  switch(dev->version[1])
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

  dev->ver = dev->version[2];
  dev->features = dev->version[3];

  usleep(54000);

  return 0;
}

void
e32_print_version(struct E32 *dev)
{
  info_output("Version Raw Value:        0x");
  for(int i=0;i<4;i++)
    info_output("%02x", dev->version[i]);
  info_output("\n");
  info_output("Frequency:                %d MHz\n", dev->frequency_mhz);
  info_output("Version:                  %d\n", dev->ver);
  info_output("Features:                 0x%02x\n", dev->features);
}

int
e32_cmd_reset(struct E32 *dev)
{
  ssize_t bytes;
  const uint8_t cmd[3] = {0xC4, 0xC4, 0xC4};
  bytes = write(dev->uart_fd, cmd, 3);
  if(bytes != 3)
    return 1;

  usleep(54000);
  return 0;
}

int
e32_cmd_write_settings(struct E32 *dev, uint8_t *settings)
{
  int err;
  ssize_t bytes;
  uint8_t orig_settings[6];

  err = 0;
  if(e32_cmd_read_settings(dev))
  {
    err_output("unable to read settings before setting them");
    return 1;
  }

  memcpy(orig_settings, dev->settings, sizeof(dev->settings));

  info_output("original settings 0x");
  for(int i=0; i<6; i++)
    info_output("%x", orig_settings[i]);
  info_output("\n");

  info_output("writing settings 0x");
  for(int i=0; i<6; i++)
    info_output("%x", settings[i]);
  info_output("\n");

  bytes = write(dev->uart_fd, settings, 6);
  if(bytes == -1)
   return -1;

  /* allow time for settings to change */
  usleep(500000);

  if(e32_cmd_read_settings(dev))
  {
    err_output("unable to read settings after setting them\n");
    return 1;
  }

  info_output("read settings 0x");
  for(int i=0; i<6; i++)
    info_output("%x", dev->settings[i]);
  info_output("\n");

  return err;
}

ssize_t
e32_transmit(struct E32 *dev, uint8_t *buf, size_t buf_len)
{
  ssize_t bytes;

  dev->state = TX;

  bytes = write(dev->uart_fd, buf, buf_len);
  if(bytes == -1)
  {
    errno_output("writing to e32 uart\n");
    return -1;
  }
  else if(bytes != buf_len)
  {
    warn_output("wrote only %d of %d\n", bytes, buf_len);
    return bytes;
  }

  if(dev->verbose)
      debug_output("e32_transmit: transmitted %d bytes\n", bytes);

  return 0;
}

int
e32_receive(struct E32 *dev, uint8_t *buf, size_t buf_len)
{
  int bytes;
  bytes = read(dev->uart_fd, buf, buf_len);
  return bytes != buf_len;
}

static int
e32_write_output(struct E32 *dev, struct options *opts, uint8_t* buf, const size_t bytes)
{
  socklen_t addrlen; // unix domain socket client address
  size_t outbytes;
  int ret = 0;

  if(bytes == 0)
    return 0;

  if(opts->output_file != NULL)
  {
    outbytes = fwrite(buf, 1, bytes, opts->output_file);
    if(outbytes != bytes)
    {
      err_output("e32_write_output: only wrote %d of %d bytes to output file", outbytes, bytes);
      ret++;
    }
  }

  for(int i=0; i<list_size(dev->socket_list); i++)
  {
    struct sockaddr_un *cl;
    cl = list_get_index(dev->socket_list, i);
    addrlen = sizeof(struct sockaddr_un);

    if(dev->verbose)
      debug_output("e32_write_output: sending %d bytes to socket %s", bytes, cl->sun_path);

    outbytes = sendto(opts->fd_socket_unix_data, buf, bytes, 0, (struct sockaddr*) cl, addrlen);
    if(outbytes == -1)
    {
      errno_output("e32_write_output: unable to send back status to unix socket. removing from list.");
      list_remove(dev->socket_list, cl);
      ret++;
    }
  }

  if(opts->output_standard)
  {
    buf[bytes] = '\0';
    info_output("%s", buf);
    fflush(stdout);
  }

  return ret;
}

static void
e32_poll_input_enable(struct options *opts, struct pollfd pfd[])
{
  if(opts->verbose)
    debug_output("e32_poll_input_enable\n");

  if(opts->input_standard)
  {
    pfd[PFD_STDIN].fd = fileno(stdin);
    pfd[PFD_STDIN].events = POLLIN;
  }

  if(opts->input_file)
  {
    pfd[PFD_INPUT_FILE].fd = fileno(opts->input_file);
    pfd[PFD_INPUT_FILE].events = POLLIN;
  }

  if(opts->fd_socket_unix_data)
  {
    pfd[PFD_SOCKET_UNIX_DATA].fd = opts->fd_socket_unix_data;
    pfd[PFD_SOCKET_UNIX_DATA].events = POLLIN;
  }

  if(opts->fd_socket_unix_control)
  {
    pfd[PFD_SOCKET_UNIX_CONTROL].fd = opts->fd_socket_unix_control;
    pfd[PFD_SOCKET_UNIX_CONTROL].events = POLLIN;
  }
}

static void
e32_poll_input_disable(struct options *opts, struct pollfd pfd[])
{
  if(opts->verbose)
    debug_output("e32_poll_input_disable\n");

  if(opts->input_standard)
  {
    pfd[PFD_STDIN].fd = -1;
    pfd[PFD_STDIN].events = 0;
  }

  if(opts->input_file)
  {
    pfd[PFD_INPUT_FILE].fd = -1;
    pfd[PFD_INPUT_FILE].events = 0;
  }

  if(opts->fd_socket_unix_data)
  {
    pfd[PFD_SOCKET_UNIX_DATA].fd = -1;
    pfd[PFD_SOCKET_UNIX_DATA].events = 0;
  }

  if(opts->fd_socket_unix_control)
  {
    pfd[PFD_SOCKET_UNIX_CONTROL].fd = -1;
    pfd[PFD_SOCKET_UNIX_CONTROL].events = 0;
  }
}

static void
e32_poll_init(struct E32 *dev, struct options *opts, struct pollfd pfd[])
{
  tty_set_read_polling(dev->uart_fd, &dev->tty);

  dev->isatty = isatty(fileno(stdin));
  if(dev->isatty)
  {
    dev->isatty = 1;
    info_output("waiting for input from the terminal\n");
  }
    // used for stdin or a pipe
  pfd[PFD_STDIN].fd = -1;
  pfd[PFD_STDIN].events = 0;

  // used for the uart
  pfd[PFD_UART].fd = dev->uart_fd;
  pfd[PFD_UART].events = POLLIN;

  // used for an input file
  pfd[PFD_INPUT_FILE].fd = -1;
  pfd[PFD_INPUT_FILE].events = 0;

  // used for a unix domain socket data
  pfd[PFD_SOCKET_UNIX_DATA].fd = -1;
  pfd[PFD_SOCKET_UNIX_DATA].events = 0;

  // poll the AUX pin for rising and falling edges
  pfd[PFD_GPIO_AUX].fd = dev->fd_gpio_aux;
  pfd[PFD_GPIO_AUX].events = POLLPRI;

  // used for a unix domain socket control
  pfd[PFD_SOCKET_UNIX_CONTROL].fd = -1;
  pfd[PFD_SOCKET_UNIX_CONTROL].events = 0;

  e32_poll_input_enable(opts, pfd);

}

static int
e32_poll_stdin(struct E32 *dev, int fd_stdin, int *loop_continue)
{
  ssize_t bytes;

  bytes = read(fd_stdin, &txbuf, E32_MAX_PACKET_LENGTH);
  if(bytes == -1)
  {
    errno_output("error reading from stdin\n");
    return 1;
  }

  if(dev->verbose)
    debug_output("e32_poll_stdin: got %d bytes as input writing to uart\n", bytes);

  if(e32_transmit(dev, txbuf, bytes))
    return 3;

  /* sent input through a pipe */
  if(!dev->isatty && bytes < E32_MAX_PACKET_LENGTH)
  {
    if(dev->verbose)
      debug_output("getting out of loop\n");
    *loop_continue = 0;
  }

  return 0;
}

static int
e32_poll_uart(struct E32 *dev, struct options *opts, int fd_uart, ssize_t *rx_buf_size)
{
  ssize_t bytes;

  bytes = read(fd_uart, rxbuf+(*rx_buf_size), RX_BUF_BYTES);
  if(bytes == -1)
  {
    errno_output("e32_poll_uart error reading from uart, fd=%d, buf=%p, total=%p, mode=%d\n", fd_uart, rxbuf, rx_buf_size, dev->mode);
    return 1;
  }

  /* some Raspberry Pi Models will buffer > 1 byte at a time which is ideal
   * the implication here is after the AUX pin transitions when reading bytes
   * we will leave bytes in the buffer. If we detect this we'll set an additional
   * delay where after the AUX pin transitions we will delay before reading the
   * additional bytes.
   */
  if (bytes > 1)
  {
    opts->aux_transition_additional_delay = 1;
  }

  *rx_buf_size += bytes;

  if(dev->verbose)
    debug_output("e32_poll_uart: received %d bytes for a total of %ld bytes from uart\n", bytes, *rx_buf_size);

  return 0;
}

static int
e32_poll_file(struct E32 *dev, struct options *opts, int fd_file, int *loop_continue)
{
  ssize_t bytes;

  if(opts->verbose)
    debug_output("reading from fd %d\n", fd_file);

  bytes = fread(txbuf, 1, E32_MAX_PACKET_LENGTH, opts->input_file);

  if(opts->verbose)
    debug_output("e32_poll_file: writing %d bytes from file to uart\n", bytes);

  if(e32_transmit(dev, txbuf, bytes))
  {
    err_output("error in transmit\n");
    return 1;
  }

  if(e32_write_output(dev, opts, txbuf, bytes))
    err_output("error writing outputs\n");

  /* all bytes read from file */
  if(bytes < E32_MAX_PACKET_LENGTH)
  {
    if(opts->verbose)
      debug_output("getting out of loop\n");
    *loop_continue = 0;
  }

  return 0;
}

static int
e32_poll_socket_unix_data(struct E32 *dev, struct options *opts, int fd_sockd, int *loop_continue)
{
  ssize_t bytes;
  uint8_t client_err; // return to socket clients
  struct sockaddr_un client;
  socklen_t addrlen; // unix domain socket client address

  client_err = 0;
  addrlen = sizeof(struct sockaddr_un);

  bytes = recvfrom(fd_sockd, txbuf, E32_MAX_PACKET_LENGTH+1, 0, (struct sockaddr*) &client, &addrlen);
  if(bytes == -1)
  {
    errno_output("error receiving from unix domain socket");
    return 1;
  }
  else if(bytes > E32_MAX_PACKET_LENGTH)
  {
    err_output("overflow: %d > %d", bytes, E32_MAX_PACKET_LENGTH);
    client_err++;
  }

  if(opts->verbose)
  {
    debug_output("e32_poll_socket_unix_data: received %d bytes from unix domain socket: %s\n", bytes, client.sun_path);
  }

  // sending 0 bytes will register and we'll add to the client list
  if(bytes == 0 && list_index_of(dev->socket_list, &client) == -1)
  {
    struct sockaddr_un *new_client;
    new_client = malloc(sizeof(struct sockaddr_un));
    memcpy(new_client, &client, sizeof(struct sockaddr_un));
    list_add_first(dev->socket_list, new_client);

    if(opts->verbose)
      debug_output("e32_poll_socket_unix_data: registered client %d at %s\n", list_size(dev->socket_list), client.sun_path);
  }

  // send back an acknowledge of 1 byte to the client
  if(bytes == 0)
  {
    bytes = sendto(fd_sockd, &client_err, 1, 0, (struct sockaddr*) &client, addrlen);
    if(bytes == -1)
    {
      errno_output("e32_poll_socket_unix_data: unable to send back status to unix socket");
      return 1;
    }
    return 0;
  }

  if(!client_err && e32_transmit(dev, txbuf, bytes))
  {
    err_output("e32_poll_socket_unix_data: error in transmit\n");
    client_err++;
  }

  if(opts->output_standard)
  {
    info_output("e32_poll_socket_unix_data: transmitted:\n");
    txbuf[bytes] = '\0';
    info_output("%s", txbuf);
    fflush(stdout);
    info_output("\n");
  }

  bytes = sendto(fd_sockd, &client_err, 1, 0, (struct sockaddr*) &client, addrlen);
  if(bytes == -1)
  {
    errno_output("e32_poll_socket_unix_data: unable to send back status to unix socket %s\n", client.sun_path);
  }

  return client_err;
}

static int
e32_poll_socket_unix_control(struct E32 *dev, struct options *opts, int fd_sockc)
{
  ssize_t bytes, ret_bytes;
  uint8_t client_err; // return to socket clients
  struct sockaddr_un client;
  socklen_t addrlen; // unix domain socket client address
  uint8_t *control;

  client_err = 0;
  /*
     allocate memory on the heap since we have to send it
     to other functions for a buffer
  */
  control = malloc(32);
  addrlen = sizeof(struct sockaddr_un);

  bytes = recvfrom(fd_sockc, control, 32, 0, (struct sockaddr*) &client, &addrlen);
  if(bytes == -1)
  {
    errno_output("e32_poll_socket_unix_control: error receiving from unix domain socket");
    client_err = 1;
  }

  debug_output("e32_poll_socket_unix_control: received %d bytes from unix domain socket: %s\n", bytes, client.sun_path);

  if(e32_set_mode(dev, SLEEP))
  {
    err_output("e32_poll_socket_unix_control: unable to go to sleep mode\n");
    client_err = 2;
  }

  if(bytes == 1 && control[0] == 's')
  {
    if(e32_cmd_read_settings(dev))
      client_err = 3;
    memcpy(control, dev->settings, sizeof(dev->settings));
    ret_bytes = sizeof(dev->settings);
  }
  else if(bytes == 1 && control[0] == 'v')
  {
    if(e32_cmd_read_version(dev))
      client_err = 4;
    memcpy(control, dev->version, sizeof(dev->version));
    ret_bytes = sizeof(dev->version);
  }
  else if(bytes == 6 && (control[0] == 0xC0 || control[0] == 0xC3))
  {
    if(e32_cmd_write_settings(dev, control))
      client_err = 5;
    ret_bytes = bytes;
  }
  else
  {
    err_output("received %d bytes\n", bytes);
    for(int i=0;i<bytes;i++)
      err_output("%02x ", control[i]);
    err_output("\n");
    client_err = 7;
  }

  if(!client_err)
  {
    //dev->state = CONTROL;
    if(opts->verbose)
    {
      for(int i=0;i<bytes;i++)
        debug_output("%02x", control[i]);
      debug_output("\n");
    }

    // TODO should we write standard output and verbose?
    if(opts->output_standard)
    {
      txbuf[bytes] = '\0';
      info_output("%s", txbuf);
      fflush(stdout);
    }
  }

  if(e32_set_mode(dev, NORMAL))
  {
    err_output("e32_poll_socket_unix_control: unable to go to normal mode\n");
    client_err = 8;
  }

  if(client_err)
  {
    err_output("e32_poll_socket_unix_control: client error %d\n", client_err);
    ret_bytes = 1;
    control[0] = client_err;
  }

  bytes = sendto(fd_sockc, control, ret_bytes, 0, (struct sockaddr*) &client, addrlen);
  if(bytes == -1)
    errno_output("e32_poll_socket_unix_control: unable to send back status to unix socket");
  else if(opts->verbose && opts->output_standard)
  {
    debug_output("writing back %d bytes to unix domain socket: %s\n", ret_bytes, client.sun_path);
    for(int i=0;i<ret_bytes;i++)
      debug_output("%02x", control[i]);
    debug_output("\n");
  }

  /*
    functions to read the settings and version will set the tty
    to have a timeout. After this function returns we need to
    set it back to a polling read. If the tcflush call isn't there
    the poll loop will detect the uart can be read and when we
    read it then it will have an invalid address. Once this happens
    we get into an infinite loop of the UART being ready but
    have an error when reading it.
  */
  tty_set_read_polling(dev->uart_fd, &dev->tty);
  tcflush(dev->uart_fd, TCIFLUSH);

  free(control);
  return client_err;
}

static int
e32_poll_gpio_aux(struct E32 *dev, struct options *opts, struct pollfd pfd[], ssize_t *rx_buf_size)
{
  /* AUX pin transitioned from high->low or low->high */
  ssize_t bytes;
  int aux;

  lseek(dev->fd_gpio_aux, 0, SEEK_SET);
  gpio_read(dev->fd_gpio_aux, &aux);

  if(aux == 0 && dev->state == IDLE)
  {
    if(dev->verbose)
      debug_output("e32_poll_gpio_aux: transition from IDLE to RX state\n");

    dev->state = RX;
    *rx_buf_size = 0;
    e32_poll_input_disable(opts, pfd);
  }
  else if(aux == 1 && dev->state == RX)
  {
    if(dev->verbose)
      debug_output("e32_poll_gpio_aux: transition from RX to IDLE state\n");

    if(opts->aux_transition_additional_delay)
    {
      if(dev->verbose)
        debug_output("e32_poll_gpio_aux: additional sleep for uart buffered data\n");
      usleep(54000);
    }

    bytes = read(pfd[PFD_UART].fd, rxbuf+(*rx_buf_size), RX_BUF_BYTES);
    if(bytes == -1)
    {
      errno_output("e32_poll_gpio_aux: error reading from uart\n");
      return -1;
    }

    *rx_buf_size += bytes;

    if(dev->verbose)
      debug_output("e32_poll_gpio_aux: received %d bytes for a total of %d bytes from uart\n", bytes, *rx_buf_size);

    if(e32_write_output(dev, opts, rxbuf, *rx_buf_size))
      err_output("e32_poll_gpio_aux: error writing outputs after RX to IDLE transition\n");

    dev->state = IDLE;
    e32_poll_input_enable(opts, pfd);
  }
  else if(aux == 0 && dev->state == TX)
  {
    if(dev->verbose)
      debug_output("e32_poll_gpio_aux: transition from IDLE to TX state\n");
    e32_poll_input_disable(opts, pfd);
  }
  else if(aux == 1 && dev->state == TX)
  {
    if(dev->verbose)
      debug_output("e32_poll_gpio_aux: transition from TX to IDLE state\n");
    dev->state = IDLE;
    e32_poll_input_enable(opts, pfd);
  }

  return 0;
}

/*
Input Sources
 - stdin with or without pipe
 - unix domain socket data
 - unix domain socket control
 - file


Output Sources
 - stdout
 - file
 - unix domain socket data
 - unix domain socket control

State Machine
 - IDLE -> When AUX=0
 - RX   -> When AUX=1
 - TX   -> When AUX=1

 We transition states when AUX transitions from high to low or low to high. When AUX transitions
 if the UART is ready to read then we go in to the RX state, if one of the input sources are ready
 we go into the TX state. In both the TX and RX state we don't go back into IDLE unless AUX
 transitions back to high.


*/
size_t
e32_poll(struct E32 *dev, struct options *opts)
{
  ssize_t rx_buf_size;
  int ret, loop;
  size_t errors;

  /* used in our poll loop */
  struct pollfd pfd[6];

  e32_poll_init(dev, opts, pfd);

  errors = 0;
  loop = 1;
  rx_buf_size = 0;
  enum E32_state prev_state;

  while(loop)
  {
    ret = poll(pfd, 6, -1);
    if(ret == 0)
    {
      err_output("poll timed out\n");
      return -1;
    }
    else if (ret < 0)
    {
      errno_output("poll");
      return ret;
    }

    prev_state = dev->state;

    if(pfd[PFD_GPIO_AUX].revents & POLLPRI)
    {
      errors += e32_poll_gpio_aux(dev, opts, pfd, &rx_buf_size);
    }

    if(pfd[PFD_UART].revents & POLLIN)
    {
      errors+= e32_poll_uart(dev, opts, pfd[PFD_UART].fd, &rx_buf_size);
    }

    if(pfd[PFD_STDIN].revents & POLLIN)
    {
      errors += e32_poll_stdin(dev, pfd[PFD_STDIN].fd, &loop);
    }

    if(pfd[PFD_INPUT_FILE].revents & POLLIN)
    {
      errors += e32_poll_file(dev, opts, pfd[PFD_INPUT_FILE].fd, &loop);
    }

    if(pfd[PFD_SOCKET_UNIX_DATA].revents & POLLIN)
    {
      errors += e32_poll_socket_unix_data(dev, opts, pfd[PFD_SOCKET_UNIX_DATA].fd, &loop);
    }

    if( pfd[PFD_SOCKET_UNIX_CONTROL].revents & POLLIN)
    {
      errors += e32_poll_socket_unix_control(dev, opts, pfd[PFD_SOCKET_UNIX_CONTROL].fd);
    }

    /*
      Take a situation where we are transferring a file.
      This file will be ready for reading much faster than can
      transmit it's bytes. So we'll first read bytes into the buffer
      and transmit them. Before the AUX pin can go low we can do this
      many times. Hence we need to disable inputs until the AUX pin
      goes back high and then they will be enabled. This logic to
      disable reading more input before the AUX pin can react.
    */
    if(prev_state == IDLE && dev->state == TX)
    {
      e32_poll_input_disable(opts, pfd);
    }
  }

  return errors;
}
