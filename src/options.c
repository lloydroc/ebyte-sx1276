#include "options.h"

// tells error.c when we print to use stdout, stderr, or syslog
int use_syslog = 0;

void
usage(char *progname)
{
  printf("Usage: %s [OPTIONS]\n", progname);
  printf("Version %s\n\n", VERSION);
  printf("A command line tool to transmit and receive data from the EByte e32 LORA Module. If this tool is run without options the e32 will transmit what is sent from the keyboard - stdin and will output what is received to stdout. Hit return to send the message. To test a connection between two e32 boards run a %s -s on both to ensure status information is correct and matching. Once the status is deemed compatible on both e32 modules then run %s without options on both. On the first type something and hit enter, which will transmit from one e32 to the other and you should see this message show up on second e32.\n\n", progname, progname);
  printf("OPTIONS:\n\
-h --help                  Print help\n\
-r --reset                 SW Reset\n\
-t --test                  Perform a test\n\
-v --verbose               Verbose Output\n\
-s --status                Get status model, frequency, address, channel, data rate, baud, parity and transmit power.\n\
-y --tty                   The UART to use. Defaults to /dev/ttyAMA0\n\
-m --mode MODE             Set mode to normal, wake-up, power-save or sleep.\n\
   --m0                    GPIO M0 Pin for output\n\
   --m1                    GPIO M1 Pin for output\n\
   --aux                   GPIO Aux Pin for input interrupt\n\
   --in-file  FILENAME     Read intput from a File\n\
   --out-file FILENAME     Write output to a File\n\
-x --socket-unix FILENAME  Send and Receive data from a Unix Domain Socket\n\
-d --daemon                Run as a Daemon\n\
");
}

void
options_init(struct options *opts)
{
  opts->reset = 0;
  opts->help = 0;
  opts->test = 0;
  opts->verbose = 0;
  opts->status = 0;
  opts->mode = -1;
  opts->uart_dev = 0;
  opts->gpio_m0 = 23;
  opts->gpio_m1 = 24;
  opts->gpio_aux = 18;
  opts->daemon = 0;
  opts->input_standard = 1;
  opts->output_standard = 1;
  opts->input_file = NULL;
  opts->output_file = NULL;
  opts->fd_socket_unix = -1;
  snprintf(opts->tty_name, 64, "/dev/ttyAMA0");
}

int
options_deinit(struct options *opts)
{
  int ret;
  ret = 0;
  if(opts->daemon)
    closelog();

  if(opts->output_file)
    ret |= fclose(opts->output_file);

  if(opts->fd_socket_unix != -1)
    close(opts->fd_socket_unix);

  return ret;
}

static int
options_get_mode(char *optarg)
{
  if(strncasecmp("normal", optarg, 6) == 0)
    return 0;
  else if(strncasecmp("wake-up", optarg, 7) == 0)
    return 1;
  else if(strncasecmp("power-save", optarg, 9) == 0)
    return 2;
  else if(strncasecmp("sleep", optarg, 9) == 0)
    return 3;

  return -1;
}

static FILE*
options_open_file(char *optarg, char *mode)
{
  FILE* file = fopen(optarg, mode);
  if(file == NULL)
  {
    errno_output("unable to open file %s", optarg);
  }
  return file;
}

static int
options_open_socket_unix(struct options *opts, char *optarg)
{
  opts->fd_socket_unix = socket(AF_UNIX, SOCK_DGRAM, 0);
  if(opts->fd_socket_unix == -1)
  {
    errno_output("error opening socket\n");
    return 1;
  }

  if(strlen(optarg) > sizeof(opts->socket_unix_server.sun_path)-1)
  {
    err_output("socket path too long must be %d chars\n", sizeof(opts->socket_unix_server.sun_path)-1);
    return 2;
  }

  if(remove(optarg) == -1 && errno != ENOENT)
  {
    errno_output("error removing socket\n");
    return 2;
  }

  memset(&opts->socket_unix_server, 0, sizeof(struct sockaddr_un));
  opts->socket_unix_server.sun_family = AF_UNIX;
  strncpy(opts->socket_unix_server.sun_path, optarg, sizeof(opts->socket_unix_server.sun_path)-1);

  if(bind(opts->fd_socket_unix, (struct sockaddr*) &opts->socket_unix_server, sizeof(struct sockaddr_un)) == -1)
  {
    errno_output("error binding to socket\n");
    return 3;
  }
  return 0;
}

int
options_parse(struct options *opts, int argc, char *argv[])
{
  int c;
  int option_index;
  int ret = 0;
#define BUF 128
  char infile[BUF];
  char outfile[BUF];
  char sockunix[BUF];

  infile[0] = '\0';
  outfile[0] = '\0';
  sockunix[0] = '\0';

  static struct option long_options[] =
  {
    {"help",                     no_argument, 0, 'h'},
    {"reset",                    no_argument, 0, 'r'},
    {"test",                     no_argument, 0, 't'},
    {"verbose",                  no_argument, 0, 'v'},
    {"status",                   no_argument, 0, 's'},
    {"tty",                required_argument, 0, 'y'},
    {"mode",               required_argument, 0, 'm'},
    {"m0",                 required_argument, 0,   0},
    {"m1",                 required_argument, 0,   0},
    {"aux",                required_argument, 0,   0},
    {"in-file",            required_argument, 0,   0},
    {"out-file",           required_argument, 0,   0},
    {"socket-unix",        required_argument, 0, 'x'},
    {"binary",                   no_argument, 0, 'b'},
    {"daemon",                   no_argument, 0, 'd'},
    {0,                                    0, 0,   0}
  };

  while(1)
  {
    option_index = 0;
    c = getopt_long(argc, argv, "hrtvsy:m:bdx:", long_options, &option_index);

    if(c == -1)
      break;

    switch(c)
    {
    case 0:
      if(strcmp("m1", long_options[option_index].name) == 0)
        opts->gpio_m1 = atoi(optarg);
      else if(strcmp("aux", long_options[option_index].name) == 0)
        opts->gpio_aux = atoi(optarg);
      else if(strcmp("out-file", long_options[option_index].name) == 0)
        strncpy(outfile, optarg, BUF);
      else if(strcmp("in-file", long_options[option_index].name) == 0)
        strncpy(infile, optarg, BUF);
      else if(strcmp("tty", long_options[option_index].name) == 0)
        strncpy(opts->tty_name, optarg, 64);
      break;
    case 'h':
      opts->help = 1;
      break;
    case 'r':
      opts->reset = 1;
      break;
    case 't':
      opts->test = 1;
      break;
    case 'v':
      opts->verbose = 1;
      break;
    case 's':
      opts->status = 1;
      break;
    case 'y':
      strncpy(opts->tty_name, optarg, 64);
      break;
    case 'm':
      opts->mode = options_get_mode(optarg);
      if(opts->mode == -1)
      {
        ret |= 1;
      }
      break;
    case 'x':
      strncpy(sockunix, optarg, BUF);
      break;
    case 'd':
      opts->daemon = 1;
      break;
    }
  }

  // this option sets whether we log to syslog or not
  // and should checked first
  if(opts->daemon)
  {
    use_syslog = 1;
    opts->input_standard = 0;
    opts->output_standard = 0;
    openlog("e32", 0, LOG_DAEMON);
  }

  if(strnlen(sockunix, BUF))
    ret |= options_open_socket_unix(opts, sockunix);

  if(strnlen(infile, BUF))
  {
    opts->input_file = options_open_file(infile, "r");
    ret |= opts->input_file == NULL;
  }

  if(strnlen(outfile, BUF))
  {
    opts->output_file = options_open_file(outfile, "w");
    ret |= opts->output_file == NULL;
  }

  if(opts->input_file != NULL)
    opts->input_standard = 0;

  if(opts->output_file != NULL)
    opts->output_standard = 0;

  // TODO fix this section if user passes in arguments without options
  if (optind < argc)
  {
    err_output("non-option ARGV-elements: ");
    while (optind < argc)
      err_output("%s ", argv[optind++]);
    err_output("");
  }

  return ret;
}
