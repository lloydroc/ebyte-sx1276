// file uart.c
#include "uart.h"

int
tty_set_read_polling(int fd, struct termios *tty)
{
  tty->c_cc[VMIN] = 0;
  tty->c_cc[VTIME] = 0;

  /* polling read - where read will return immediately with
   * min(bytes avail, bytes requested)
   * and bytes avail can be 0
   */
  if(tcsetattr(fd, TCSANOW, tty) == -1)
  {
    err_output("tty_set_read_polling(): error setting terminal attributes");
    return -1;
  }

  return 0;
}

int
tty_set_read_with_timeout(int fd, struct termios *tty, int deciseconds)
{
  tty->c_cc[VMIN] = 0;
  tty->c_cc[VTIME] = deciseconds;

  /* read with timeout - timer will start when read is called
   * and will return with at least 1 byte available or if
   * returned with 0 will represent a timeout
   */
  if(tcsetattr(fd, TCSANOW, tty) == -1)
  {
    err_output("tty_set_read_with_timeout(): error setting terminal attributes");
    return -1;
  }

  return 0;
}

int
tty_open(char *pty_name, int *tty_fd, struct termios *tty)
{
  *tty_fd = open(pty_name, O_RDWR | O_NOCTTY);
  if(*tty_fd == -1)
  {
    errno_output("error opening terminal %s", pty_name);
    close(*tty_fd);
    return -1;
  }

  if(tcgetattr(*tty_fd, tty) == -1)
  {
    errno_output("tty_open: unable to get tty attributes for %s\n", pty_name);
    close(*tty_fd);
    return -1;
  }

  if(cfsetispeed(tty, B9600) == -1)
  {
    errno_output("tty_open: unable to uart input speed\n");
    return -1;
  }

  if(cfsetospeed(tty, B9600) == -1)
  {
    errno_output("tty_open: unable to set output speed\n");
    return -1;
  }

  cfmakeraw(tty);

  tty->c_cflag |= CREAD | CLOCAL;

  if(tty_set_read_polling(*tty_fd, tty))
  {
    close(*tty_fd);
    return -1;
  }

  if(tcflush(*tty_fd, TCIFLUSH) == -1)
  {
    errno_output("tty_open: unable to flush terminal\n");
    return -1;
  }

  if(tcdrain(*tty_fd) == -1)
  {
    errno_output("tty_open: unable to drain terminal\n");
    return -1;
  }

  return 0;
}
