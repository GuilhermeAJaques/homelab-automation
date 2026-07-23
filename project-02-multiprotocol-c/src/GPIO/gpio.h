#ifndef GPIO_H
#define GPIO_H

#include <gpiod.h>

typedef struct {
    char chipPath[50];
    struct gpiod_chip *chip;
    int connected;
} GPIOClientWrapper;

void gpio_client_init(GPIOClientWrapper *wrapper, const char *chipPath);
int gpio_client_connect(GPIOClientWrapper *wrapper);
void gpio_client_disconnect(GPIOClientWrapper *wrapper);
int gpio_client_read(GPIOClientWrapper *wrapper, int offset, char *value, int max_len);
int gpio_client_write(GPIOClientWrapper *wrapper, int offset, const char *value);

#endif