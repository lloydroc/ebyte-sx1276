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
  printf("reading aux before poll\n");
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
    printf("waiting for rising edge of AUX\n");
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

  if(dev->verbose)
    printf("rising edge of AUX found\n");

  gpio_read(dev->gpio_aux_fd, &gpio_aux_val);

  if(dev->verbose)
    printf("AUX value is %d\n", gpio_aux_val);

  if(dev->verbose)
    printf("sleeping\n");

  // TODO this line has been the biggest source of pain
  usleep(540000);

  if(gpio_aux_val != 1)
    fprintf(stderr, "AUX was low after rising edge?\n");

  return gpio_aux_val != 1;
}

int
e32_set_mode(struct E32 *dev, int mode)
{
  int ret;

  if(e32_get_mode(dev))
  {
    fprintf(stderr, "unable to get mode\n");
    return 1;
  }

  if(dev->mode == mode)
  {
    if(dev->verbose)
      printf("mode %d unchanged\n", mode);
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
    printf("new mode %d, prev mode is %d\n", mode, dev->prev_mode);

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
    printf("read mode %d\n", dev->mode);

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
  if(opts->output_file)
    ret |= fclose(opts->output_file);

  ret |= close(dev->uart_fd);

  if(opts->fd_socket_unix != -1)
    close(opts->fd_socket_unix);

  return ret;
}

int
e32_cmd_read_settings(struct E32 *dev)
{
  ssize_t bytes;
  const uint8_t cmd[3] = {0xC1, 0xC1, 0xC1};
  uint8_t *buf_ptr;

  if(dev->verbose)
    puts("writing settings command");

  bytes = write(dev->uart_fd, cmd, 3);
  if(bytes == -1)
   return -1;

  if(dev->verbose)
    puts("reading settings command");

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
  printf("Settings Raw Value:       0x");
  for(int i=0; i<6; i++) printf("%02x", dev->settings[i]);
  puts("");

  if(dev->power_down_save)
    printf("Power Down Save:          Save parameters on power down\n");
  else
    printf("Power Down Save:          Discard parameters on power down\n");

  printf("Address:                  0x%02x%02x\n", dev->addh, dev->addl);

  switch(dev->parity)
  {
  case 0:
    printf("Parity:                   8N1\n");
    break;
  case 1:
    printf("Parity:                   8O1\n");
    break;
  case 2:
    printf("Parity:                   8E1\n");
    break;
  case 3:
    printf("Parity:                   8N1\n");
    break;
  default:
    printf("Parity:                   Unknown\n");
    break;
  }

  printf("UART Baud Rate:           %d bps\n", dev->uart_baud);
  printf("Air Data Rate:            %d bps\n", dev->air_data_rate);
  printf("Channel:                  %d MHz\n", dev->channel);

  if(dev->transmission_mode)
    printf("Transmission Mode:        Transparent\n");
  else
    printf("Transmission Mode:        Fixed\n");

  if(dev->io_drive)
    printf("IO Drive:                 TXD and AUX push-pull output, RXD pull-up input\n");
  else
    printf("IO Drive:                 TXD and AUX open-collector output, RXD open-collector input\n");

  printf("Wireless Wakeup Time:     %d ms\n", dev->wireless_wakeup_time);

  if(dev->fec)
    printf("Forward Error Correction: on\n");
  else
    printf("Forward Error Correction: off\n");

  printf("TX Power:                 %d dBm\n", dev->tx_power_dbm);
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
    puts("writing version command");

  bytes = write(dev->uart_fd, cmd, 3);
  if(bytes == -1)
   return -1;

  if(dev->verbose)
    puts("reading version");

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
    fprintf(stderr, "mismatch 0x%02x != 0xc3\n", dev->version[0]);
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
  printf("Version Raw Value:        0x");
  for(int i=0;i<4;i++)
    printf("%02x", dev->version[i]);
  puts("");
  printf("Frequency:                %d MHz\n", dev->frequency_mhz);
  printf("Version:                  %d\n", dev->ver);
  printf("Features:                 0x%02x\n", dev->features);
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
    printf("here\n");
    err_output("writing to e32 uart\n");
    return -1;
  }
  else if(bytes != buf_len)
  {
    printf("wrote only %d of %d\n", bytes, buf_len);
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

  printf("polling file descriptors\n");
  loop = 1;
  while(loop)
  {
    ret = poll(pfd, 4, -1);
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

    /* stdin or pipe */
    if(pfd[0].revents & POLLIN)
    {
      pfd[0].revents ^= POLLIN;

      printf("reading from fd %d\n", pfd[0].fd);
      bytes = read(pfd[0].fd, &buf, E32_TX_BUF_BYTES);
      printf("got %d bytes\n", bytes);

      printf("writing to uart\n");
      ret = e32_transmit(dev, buf, bytes);
      if(ret)
      {
        return 3;
      }

      /* sent input through a pipe */
      if(!tty && bytes < E32_TX_BUF_BYTES)
      {
        printf("getting out of loop\n");
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
        printf("%s", buf);
      }
    }

    /* user specified file */
    if(pfd[2].revents & POLLIN)
    {
      pfd[2].revents ^= POLLIN;

      if(opts->verbose)
        printf("reading from fd %d\n", pfd[2].fd);

      bytes = fread(buf, 1, E32_TX_BUF_BYTES, opts->input_file);

      if(opts->verbose)
        printf("writing %d bytes from file to uart\n", bytes);

      if(e32_transmit(dev, buf, bytes))
      {
        fprintf(stderr, "error in transmit\n");
        //return 3;
      }

      if(opts->output_standard)
      {
        buf[bytes] = '\0';
        printf("%s", buf);
      }

      /* all bytes read from file */
      if(bytes < E32_TX_BUF_BYTES)
      {
        if(opts->verbose)
          printf("getting out of loop\n");
        loop = 0;
      }
    }

    /* received from unix domain socket */
    if(pfd[3].revents & POLLIN)
    {
      pfd[3].revents ^= POLLIN;

      size_t len = sizeof(struct sockaddr_un);
      bytes = recvfrom(pfd[3].fd, buf, E32_TX_BUF_BYTES, 0, (struct sockaddr*) &opts->socket_unix_client, &len);

      if(bytes == -1)
      {
        fprintf(stderr, "error receiving from unix domain socket");
        return 4;
      }

      if(opts->verbose)
        printf("received %d bytes from unix domain socket: %s\n", bytes, opts->socket_unix_client.sun_path);

      if(e32_transmit(dev, buf, bytes))
      {
        fprintf(stderr, "error in transmit\n");
        //return 3;
      }

      if(opts->output_standard)
      {
        buf[bytes] = '\0';
        printf("%s", buf);
      }
    }
  }

  return 0;
}
