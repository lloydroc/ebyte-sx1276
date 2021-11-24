#ifndef ERROR
#define ERROR

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

// use the extern use_syslog variable to print to stdout/stderr or syslog
extern int use_syslog;

#define MAX_ENAME 106

void
info_output(const char *format, ...);

void
debug_output(const char *format, ...);

void
warn_output(const char *format, ...);

void
err_output(const char *format, ...);

void
errno_output(const char *format, ...);

#endif
