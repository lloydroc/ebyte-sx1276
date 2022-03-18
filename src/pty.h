#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <libgen.h>
#include <sys/select.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "error.h"

#define MAX_SNAME 100
#define BUF_SIZE 1024

/*
 * Sets our tty to raw settings
 */
int
ttySetRaw(int fd, struct termios *prevTermios);

/*
 * Sets up the connection between the two processes using a pseudo-terminal pair.
 * Create a child process connected to the parent by a pseudo-terminal pair.
 *
 * returns the process id of the child or -1 on error. In successfully created
 * child it will return 0.
 */
pid_t
ptyFork(
    int *masterFd,
    char *slaveName,
    size_t snLen,
    const struct termios *slaveTermios,
    const struct winsize *slaveWS);

int
pty_create(int *fd_pty_master);
