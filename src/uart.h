// file: uart.h
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include "error.h"

int
tty_set_read_polling(int fd, struct termios *tty);

int
tty_set_read_with_timeout(int fd, struct termios *tty, int deciseconds);

int
tty_open(char *pty_name, int *tty_fd, struct termios *tty);
