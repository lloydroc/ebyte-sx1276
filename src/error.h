#ifndef ERROR
#define ERROR

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_ENAME 106

void
err_output(const char *format, ...);

void
err_exit(const char *format, ...);

#endif
