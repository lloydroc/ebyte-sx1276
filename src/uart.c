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
    err_output("error setting terminal attributes");
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
    err_output("error setting terminal attributes");
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
    err_output("unable to get tty attributes for %s\n", pty_name);
    close(*tty_fd);
    return -1;
  }

  cfsetispeed(tty, B9600);
  cfsetospeed(tty, B9600);
  cfmakeraw(tty);

  tty->c_cflag |= CREAD | CLOCAL;

  if(tty_set_read_polling(*tty_fd, tty))
  {
    close(*tty_fd);
    return -1;
  }

  tcflush(*tty_fd, TCIFLUSH);
  tcdrain(*tty_fd);

  return 0;
}
