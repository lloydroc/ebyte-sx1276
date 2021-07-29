#ifndef BECOME_DAEMON_H
#define BECOME_DAEMON_H

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int
become_daemon();

int
write_pidfile(char *file);

#endif
