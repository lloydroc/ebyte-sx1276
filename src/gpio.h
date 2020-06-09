#ifndef GPIO_INCLUDE
#define GPIO_INCLUDE

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define GPIO_PATH "/sys/class/gpio"
#define GPIO_EXPORT_PATH "/sys/class/gpio/export"
#define GPIO_DIRECTION_PATH "/sys/class/gpio/gpio%d/direction"
#define GPIO_EDGE_PATH "/sys/class/gpio/gpio%d/edge"
#define GPIO_VALUE_PATH "/sys/class/gpio/gpio%d/value"
#define GPIO_UNEXPORT_PATH "/sys/class/gpio/unexport"

enum edge
{
  none,
  rising,
  falling,
  both
};

int
gpio_exists();

int
gpio_permissions_valid();

// TODO map pin to cpu ports
int
gpio_valid(int gpio);

int
gpio_get_exports(int inputs[], int ouputs[], int *num_inputs, int *num_outputs);

int
gpio_export(int gpio);

#define gpio_set_output(gpio) gpio_set_direction(gpio,0)
#define gpio_set_input(gpio) gpio_set_direction(gpio,1)

int
gpio_set_direction(int gpio, int input);

int
gpio_get_direction(int gpio, int *input);

#define gpio_set_edge_rising(gpio) gpio_set_edge(gpio, rising)
#define gpio_set_edge_falling(gpio) gpio_set_edge(gpio, falling)
#define gpio_set_edge_both(gpio) gpio_set_edge(gpio, both)

int
gpio_set_edge(int gpio, int edge);

int
gpio_get_edge(int gpio, int *edge);

#define gpio_open_output(gpio) gpio_open(gpio,0)
#define gpio_open_input(gpio) gpio_open(gpio,1)

int
gpio_open(int gpio, int input);

int
gpio_close(int fd);

int
gpio_write(int fd, int val);

int
gpio_read(int fd, char *val);

int
gpio_unexport(int gpio);

#endif
