#ifndef OPTIONS_H
#define OPTIONS_H
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "error.h"

struct options
{
  int help;
  int reset;
  int test;
  int verbose;
  int status;
  int configure;
  int uart_dev;
  int gpio_m0;
  int gpio_m1;
  int gpio_aux;
  int mode;
  int daemon;
  int input_standard;
  int output_standard;
  FILE* input_file;
  FILE* output_file;
  int fd_socket_udp;
  struct sockaddr_in socket_udp_dest;
  int fd_socket_unix;
  struct sockaddr_un socket_unix_server;
  struct sockaddr_un socket_unix_client;
  int binary;
};

void
usage(char *progname);

void
options_init(struct options *opts);

int
options_parse(struct options *opts, int argc, char *argv[]);

#endif
