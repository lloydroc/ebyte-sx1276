#include "e32.h"

static int
e32_init_gpio(struct options *opts, struct E32 *dev)
{
  int inputs[64], outputs[64];
  int ninputs, noutputs;
  int hm0=0, hm1=0, haux=0;

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

    if(gpio_set_edge_rising(opts->gpio_aux))
      return 11;
  }

  int edge;
  if(gpio_get_edge(opts->gpio_aux, &edge))
    return 12;

  if(edge != 1)
    if(gpio_set_edge_rising(opts->gpio_aux))
        return 13;

  dev->gpio_m0_fd = gpio_open(opts->gpio_m0);
  if(dev->gpio_m0_fd == -1)
    return 14;

  dev->gpio_m1_fd = gpio_open(opts->gpio_m1);
  if(dev->gpio_m1_fd == -1)
    return 15;

  dev->gpio_aux_fd = gpio_open(opts->gpio_aux);
  if(dev->gpio_aux_fd == -1)
    return 16;

  return 0;
}

static int
e32_init_uart(struct E32 *dev)
{
  dev->uart_fd = uart_open();
  return dev->uart_fd;
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

  ret = e32_init_gpio(opts, dev);

  if(ret)
    return ret;

  ret = e32_init_uart(dev);
  if(ret == -1)
    return ret;

  dev->verbose = opts->verbose;
  dev->prev_mode = -1;

  dev->socket_list = calloc(1, sizeof(struct List));
  list_init(dev->socket_list, socket_match, socket_free);

  return 0;
}

static int
e32_aux_poll(struct E32 *dev)
{
  int timeout;
  struct pollfd pfd;
  int ret;
  int gpio_aux_val;

  /*
  info_output("reading aux before poll\n");
  lseek(dev->gpio_aux_fd, 0, SEEK_SET);
  ret = gpio_read(dev->gpio_aux_fd, buf) != 2;
  if(ret == 0 && buf[0] == 1)
    return 0;
  */
  lseek(dev->gpio_aux_fd, 0, SEEK_SET);

  timeout = 10000;

  pfd.fd = dev->gpio_aux_fd;
  pfd.events = POLLPRI;

  if(dev->verbose)
    debug_output("waiting for rising edge of AUX\n");
  ret = poll(&pfd, 1, timeout);
  if(ret == 0)
  {
    err_output("poll timed out\n");
    return 1;
  }
  else if (ret < 0)
  {
    errno_output("poll");
    return 2;
  }

  if(dev->verbose)
    debug_output("rising edge of AUX found\n");

  gpio_read(dev->gpio_aux_fd, &gpio_aux_val);

  if(dev->verbose)
    debug_output("AUX value is %d\n", gpio_aux_val);

  if(dev->verbose)
    debug_output("sleeping\n");

  // TODO this line has been the biggest source of pain
  usleep(540000);

  if(gpio_aux_val != 1)
    err_output("AUX was low after rising edge?\n");

  return gpio_aux_val != 1;
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

  if(dev->mode == mode)
  {
    if(dev->verbose)
      debug_output("mode %d unchanged\n", mode);
    return 0;
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

  if(dev->verbose)
    debug_output("new mode %d, prev mode is %d\n", mode, dev->prev_mode);

  if(dev->prev_mode == 3 && dev->mode != 3)
  {
    if(e32_aux_poll(dev))
      return 2;
  }

  usleep(10000);

  return ret;
}

int
e32_get_mode(struct E32 *dev)
{
  int ret;

  int m0, m1;

  ret  = gpio_read(dev->gpio_m0_fd, &m0) != 2;
  ret |= gpio_read(dev->gpio_m1_fd, &m1) != 2;

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
  ret |= gpio_close(dev->gpio_m0_fd);
  ret |= gpio_close(dev->gpio_m1_fd);
  ret |= gpio_close(dev->gpio_aux_fd);

  ret |= gpio_unexport(opts->gpio_m0);
  ret |= gpio_unexport(opts->gpio_m1);
  ret |= gpio_unexport(opts->gpio_aux);
*/


  ret |= close(dev->uart_fd);

  list_destroy(dev->socket_list);
  free(dev->socket_list);

  return ret;
}

int
e32_cmd_read_settings(struct E32 *dev)
{
  ssize_t bytes;
  const uint8_t cmd[3] = {0xC1, 0xC1, 0xC1};
  uint8_t *buf_ptr;

  if(dev->verbose)
    debug_output("writing settings command");

  bytes = write(dev->uart_fd, cmd, 3);
  if(bytes == -1)
   return -1;

  if(dev->verbose)
    debug_output("reading settings command");

  bytes = 0;
  buf_ptr = dev->settings;
  do
  {
    bytes += read(dev->uart_fd, buf_ptr, 1);
    buf_ptr += (bytes != 0);
    if(bytes == -1)
      return -1;
  }
  while(bytes < 6);

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

  dev->channel = dev->settings[4] & 0b11100000;
  dev->channel >>= 5;
  dev->channel += 410;

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

  if(e32_aux_poll(dev))
    return 2;

  return 0;
}

void
e32_print_settings(struct E32 *dev)
{
  info_output("Settings Raw Value:       0x");
  for(int i=0; i<6; i++) info_output("%02x", dev->settings[i]);
  info_output("");

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
  info_output("Channel:                  %d MHz\n", dev->channel);

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
  const uint8_t cmd[3] = {0xC3, 0xC3, 0xC3};
  uint8_t *buf;

  if(dev->verbose)
    debug_output("writing version command");

  bytes = write(dev->uart_fd, cmd, 3);
  if(bytes == -1)
   return -1;

  if(dev->verbose)
    debug_output("reading version");

  bytes = 0;
  buf = dev->version;
  do
  {
    bytes += read(dev->uart_fd, buf, 1);
    buf += (bytes != 0);
    if(bytes == -1)
      return -1;
  }
  while(bytes < 4);

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

  if(e32_aux_poll(dev))
    return 2;

  return bytes != 4;
}

void
e32_print_version(struct E32 *dev)
{
  info_output("Version Raw Value:        0x");
  for(int i=0;i<4;i++)
    info_output("%02x", dev->version[i]);
  info_output("");
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

  return e32_aux_poll(dev);
}

int
e32_cmd_write_settings(struct E32 *dev)
{
  return -1;
}

int
e32_transmit(struct E32 *dev, uint8_t *buf, size_t buf_len)
{
  ssize_t bytes;
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

  return e32_aux_poll(dev);
}

int
e32_receive(struct E32 *dev, uint8_t *buf, size_t buf_len)
{
  int bytes;
  bytes = read(dev->uart_fd, buf, buf_len);
  return bytes != buf_len;
}

int
e32_poll(struct E32 *dev, struct options *opts)
{
  ssize_t bytes;
  struct pollfd pfd[4];
  int ret;
  uint8_t buf[E32_TX_BUF_BYTES+1];
  int loop;
  int tty;
  uint8_t clret; // return to socket clients
  struct sockaddr_un client;
  socklen_t addrlen; // unix domain socket client address

  tty = isatty(fileno(stdin));

  // used for stdin or a pipe
  pfd[0].fd = -1;
  pfd[0].events = 0;

  // used for the uart
  pfd[1].fd = dev->uart_fd;
  pfd[1].events = POLLIN;

  // used for an input file
  pfd[2].fd = -1;
  pfd[2].events = 0;

  // used for a unix domain socket
  pfd[3].fd = -1;
  pfd[3].events = 0;

  if(opts->input_standard)
  {
    pfd[0].fd = fileno(stdin);
    pfd[0].events = POLLIN;
  }

  if(opts->input_file)
  {
    pfd[2].fd = fileno(opts->input_file);
    pfd[2].events = POLLIN;
  }

  if(opts->fd_socket_unix)
  {
    pfd[3].fd = opts->fd_socket_unix;
    pfd[3].events = POLLIN;
  }

  debug_output("waiting for input\n");
  loop = 1;
  while(loop)
  {
    ret = poll(pfd, 4, -1);
    if(ret == 0)
    {
      err_output("poll timed out\n");
      return 1;
    }
    else if (ret < 0)
    {
      errno_output("poll");
      return 2;
    }

    /* stdin or pipe */
    if(pfd[0].revents & POLLIN)
    {
      pfd[0].revents ^= POLLIN;

      debug_output("reading from fd %d\n", pfd[0].fd);
      bytes = read(pfd[0].fd, &buf, E32_TX_BUF_BYTES);
      debug_output("got %d bytes\n", bytes);

      debug_output("writing to uart\n");
      ret = e32_transmit(dev, buf, bytes);
      if(ret)
      {
        return 3;
      }

      /* sent input through a pipe */
      if(!tty && bytes < E32_TX_BUF_BYTES)
      {
        if(dev->verbose)
          debug_output("getting out of loop\n");
        loop = 0;
      }
    }

    /* uart for e32 */
    if(pfd[1].revents & POLLIN)
    {
      pfd[1].revents ^= POLLIN;

      bytes = read(pfd[1].fd, buf, 1);

      if(opts->output_file != NULL)
      {
        bytes = fwrite(buf, 1, bytes, opts->output_file);
      }

      if(opts->output_standard)
      {
        buf[bytes] = '\0';
        info_output("%s", buf);
        fflush(stdout);
      }

      // TODO we send a single byte at a time
      for(int i=0; i<list_size(dev->socket_list); i++)
      {
        struct sockaddr_un *cl;
        cl = list_get_index(dev->socket_list, i);
        addrlen = sizeof(struct sockaddr_un);
        bytes = sendto(pfd[3].fd, buf, bytes, 0, (struct sockaddr*) cl, addrlen);
        if(bytes == -1)
        {
          errno_output("unable to send back status to unix socket");
          list_remove(dev->socket_list, cl);
        }
      }
      // TODO write to udp
    }

    /* user specified file */
    if(pfd[2].revents & POLLIN)
    {
      pfd[2].revents ^= POLLIN;

      if(opts->verbose)
        debug_output("reading from fd %d\n", pfd[2].fd);

      bytes = fread(buf, 1, E32_TX_BUF_BYTES, opts->input_file);

      if(opts->verbose)
        debug_output("writing %d bytes from file to uart\n", bytes);

      if(e32_transmit(dev, buf, bytes))
      {
        err_output("error in transmit\n");
        //return 3;
      }

      if(opts->output_standard)
      {
        buf[bytes] = '\0';
        info_output("%s", buf);
      }

      /* all bytes read from file */
      if(bytes < E32_TX_BUF_BYTES)
      {
        if(opts->verbose)
          debug_output("getting out of loop\n");
        loop = 0;
      }
    }

    /* received from unix domain socket */
    if(pfd[3].revents & POLLIN)
    {
      pfd[3].revents ^= POLLIN;
      clret = 0;
      addrlen = sizeof(struct sockaddr_un);

      bytes = recvfrom(pfd[3].fd, buf, E32_TX_BUF_BYTES, 0, (struct sockaddr*) &client, &addrlen);
      if(bytes == -1)
      {
        errno_output("error receiving from unix domain socket");
        continue;
      }
      else if(bytes > E32_TX_BUF_BYTES)
      {
        err_output("overflow: datagram truncated to %d bytes", E32_TX_BUF_BYTES);
        bytes = E32_TX_BUF_BYTES;
        clret++;
      }

      if(opts->verbose)
        debug_output("received %d bytes from unix domain socket: %s\n", bytes, client.sun_path);

      /* if not in the list add them */
      if(bytes == 0 && list_index_of(dev->socket_list, &client))
      {
        if(opts->verbose)
          debug_output("adding client %d at %s\n", list_size(dev->socket_list), client.sun_path);

        struct sockaddr_un *new_client;
        new_client = malloc(sizeof(struct sockaddr_un));
        memcpy(new_client, &client, sizeof(struct sockaddr_un));
        list_add_first(dev->socket_list, new_client);
      }
      else if(bytes == 0)
      {
        if(opts->verbose)
          debug_output("removing client at %s\n", client.sun_path);

        list_remove(dev->socket_list, &client);
      }

      /* sending 0 bytes will register or de-register */
      if(bytes == 0)
      {
        bytes = sendto(pfd[3].fd, &clret, 1, 0, (struct sockaddr*) &client, addrlen);
        if(bytes == -1)
          errno_output("unable to send back status to unix socket");
        continue;
      }

      if(e32_transmit(dev, buf, bytes))
      {
        err_output("error in transmit\n");
        clret++;
      }

      if(opts->output_standard)
      {
        buf[bytes] = '\0';
        info_output("%s", buf);
        fflush(stdout);
      }

      bytes = sendto(pfd[3].fd, &clret, 1, 0, (struct sockaddr*) &client, addrlen);
      if(bytes == -1)
        errno_output("unable to send back status to unix socket");
    }
  }

  return 0;
}
