#include "options.h"

// tells error.c when we print to use stdout, stderr, or syslog
int use_syslog = 0;

void
usage(char *progname)
{
  struct options opts;
  options_init(&opts);
  printf("Usage: %s [OPTIONS]\n", progname);
  printf("Version %s\n\n", VERSION);
  printf("A command line tool to transmit and receive data from the EByte e32 LORA Module. If this tool is run without options the e32 will transmit what is sent from the keyboard - stdin and will output what is received to stdout. Hit return to send the message. To test a connection between two e32 boards run a %s -s on both to ensure status information is correct and matching. Once the status is deemed compatible on both e32 modules then run %s without options on both. On the first type something and hit enter, which will transmit from one e32 to the other and you should see this message show up on second e32.\n\n", progname, progname);
  printf("OPTIONS:\n\
-h --help                Print help\n\
-r --reset               SW Reset\n\
-t --test                Perform a test\n\
-v --verbose             Verbose Output\n\
-s --status              Get status model, frequency, address, channel, data rate, baud, parity and transmit power.\n\
-w --write-settings HEX  Write settings from HEX. see datasheet for these 6 bytes. Example: -w C000001A1744.\n\
                         For the form XXYYYY1AZZ44. If XX=C0 parameters are saved to e32's EEPROM, if XX=C2 settings\n\
                         will be lost on power cycle. The address is represented by YYYY and the channel is represented\n\
                         by ZZ.\n\
-y --tty                 The UART to use. Defaults to /dev/serial0 the soft link\n\
-m --mode MODE           Set mode to normal, wake-up, power-save or sleep.\n\
   --m0                  GPIO M0 Pin for output [%d]\n\
   --m1                  GPIO M1 Pin for output [%d]\n\
   --aux                 GPIO Aux Pin for input interrupt [%d]\n\
   --in-file  FILENAME   Transmit a file\n\
   --out-file FILENAME   Write received output to a file\n\
-x --sock-unix-data FILE Send and receive data from a Unix Domain Socket\n\
-c --sock-unix-ctrl FILE Change and Read settings from a Unix Domain Socket\n\
-d --daemon              Run as a Daemon\n\
", opts.gpio_m0, opts.gpio_m1, opts.gpio_aux);
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

#ifdef GPIO_M0_PIN
  opts->gpio_m0 = GPIO_M0_PIN;
#else
  opts->gpio_m0 = _GPIO_M0_PIN;
#endif

#ifdef GPIO_M1_PIN
  opts->gpio_m1 = GPIO_M1_PIN;
#else
  opts->gpio_m1 = _GPIO_M1_PIN;
#endif

#ifdef GPIO_AUX_PIN
  opts->gpio_aux = GPIO_AUX_PIN;
#else
  opts->gpio_aux = _GPIO_AUX_PIN;
#endif

  opts->daemon = 0;
  opts->input_standard = 1;
  opts->output_standard = 1;
  opts->input_file = NULL;
  opts->output_file = NULL;
  opts->fd_socket_unix_data = -1;
  opts->fd_socket_unix_control = -1;
  memset(opts->settings_write_input, 0, sizeof(opts->settings_write_input));
  snprintf(opts->tty_name, 64, "/dev/serial0");
}

void
options_print(struct options* opts)
{
  printf("option reset is %d\n", opts->reset);
  printf("option help is %d\n", opts->help);
  printf("option verbose is %d\n", opts->verbose);
  printf("option status is %d\n", opts->status);
  printf("option mode is %d\n", opts->mode);
  printf("option GPIO M0 Pin is %d\n", opts->gpio_m0);
  printf("option GPIO M1 Pin is %d\n", opts->gpio_m1);
  printf("option GPIO AUX Pin is %d\n", opts->gpio_aux);
  printf("option daemon %d\n", opts->daemon);
  printf("option TTY Name is %s\n", opts->tty_name);
  printf("option socket unix data file desciptor %d\n", opts->fd_socket_unix_data);
  printf("option socket unix control file desciptor %d\n", opts->fd_socket_unix_control);

  if(opts->settings_write_input[0])
  {
    printf("option write settings is: ");
    for(int i=0;i<6;i++)
      printf("%x", opts->settings_write_input[i]);
    puts("");
  }
}

int
options_deinit(struct options *opts)
{
  int err;
  err = 0;
  if(opts->daemon)
    closelog();

  if(opts->output_file)
    err |= fclose(opts->output_file);

  if(opts->fd_socket_unix_data != -1)
    close(opts->fd_socket_unix_data);

  if(opts->fd_socket_unix_control != -1)
    close(opts->fd_socket_unix_control);

  return err;
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

int
options_parse_settings(struct options *opts, char *settings)
{
  int num_parsed, err;
  char hexbyte[3];
  uint8_t *ptr;

  printf("parsing %s\n", settings);

  if(strnlen(settings, 13) != 12)
  {
    fprintf(stderr, "options not of length 12");
    err = 1;
    goto bad_settings;
  }

  if(settings[0] != 'c' && settings[0] != 'C')
  {
    fprintf(stderr, "options don't start with 0xc\n");
    err = 2;
    goto bad_settings;
  }

  ptr = opts->settings_write_input;
  hexbyte[2] = '\0';
  for(int i=0; i<12; i=i+2)
  {
    hexbyte[0] = settings[i];
    hexbyte[1] = settings[i+1];
    num_parsed = sscanf(hexbyte, "%hhx", ptr++);
    if(num_parsed != 1)
    {
      err_output("error parsing %s\n", hexbyte);
      err = 3;
      goto bad_settings;
    }
  }
  return 0;

bad_settings:
  fprintf(stderr, "error parsing settings %s, expect form CXXXXXXXXXXX\n", settings);
  return err;
}

int
options_parse(struct options *opts, int argc, char *argv[])
{
  int c;
  int option_index;
  int err = 0;
#define BUF 128
  char infile[BUF];
  char outfile[BUF];

  infile[0] = '\0';
  outfile[0] = '\0';

  static struct option long_options[] =
  {
    {"help",                     no_argument, 0, 'h'},
    {"reset",                    no_argument, 0, 'r'},
    {"test",                     no_argument, 0, 't'},
    {"verbose",                  no_argument, 0, 'v'},
    {"status",                   no_argument, 0, 's'},
    {"tty",                required_argument, 0, 'y'},
    {"mode",               required_argument, 0, 'm'},
    {"write-settings",     required_argument, 0, 'w'},
    {"m0",                 required_argument, 0,   0},
    {"m1",                 required_argument, 0,   0},
    {"aux",                required_argument, 0,   0},
    {"in-file",            required_argument, 0,   0},
    {"out-file",           required_argument, 0,   0},
    {"sock-unix-data",     required_argument, 0, 'x'},
    {"sock-unix-ctrl",     required_argument, 0, 'c'},
    {"binary",                   no_argument, 0, 'b'},
    {"daemon",                   no_argument, 0, 'd'},
    {0,                                    0, 0,   0}
  };

  while(1)
  {
    option_index = 0;
    c = getopt_long(argc, argv, "hrtvsy:m:bdx:w:c:", long_options, &option_index);

    if(c == -1)
      break;

    switch(c)
    {
    case 0:
      if(strcmp("m0", long_options[option_index].name) == 0)
        opts->gpio_m0 = atoi(optarg);
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
      else if(strcmp("write-input", long_options[option_index].name) == 0)
        err |= options_parse_settings(opts, optarg);
      else if(strcmp("sock-unix-data", long_options[option_index].name) == 0)
        err |= socket_unix_bind(optarg, &opts->fd_socket_unix_data, &opts->socket_unix_data);
      else if(strcmp("sock-unix-ctrl", long_options[option_index].name) == 0)
        err |= socket_unix_bind(optarg, &opts->fd_socket_unix_control, &opts->socket_unix_control);
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
        err |= 1;
      }
      break;
    case 'x':
      err |= socket_unix_bind(optarg, &opts->fd_socket_unix_data, &opts->socket_unix_data);
      break;
    case 'c':
      err |= socket_unix_bind(optarg, &opts->fd_socket_unix_control, &opts->socket_unix_control);
      break;
    case 'd':
      opts->daemon = 1;
      break;
    case 'w':
      err |= options_parse_settings(opts, optarg);
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

  if(strnlen(infile, BUF))
  {
    opts->input_file = options_open_file(infile, "r");
    err |= opts->input_file == NULL;
  }

  if(strnlen(outfile, BUF))
  {
    opts->output_file = options_open_file(outfile, "w");
    err |= opts->output_file == NULL;
  }

  if(opts->input_file != NULL)
    opts->input_standard = 0;

  if(opts->output_file != NULL)
    opts->output_standard = 0;

  if (optind < argc)
  {
    err_output("non-option ARGV-elements: ");
    while (optind < argc)
      err_output("%s ", argv[optind++]);
    err_output("");
    err |= 1;
  }

  return err;
}
