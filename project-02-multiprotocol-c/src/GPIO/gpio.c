#include "gpio.h"
#include <string.h>
#include <stdio.h>

void gpio_client_init(GPIOClientWrapper *wrapper, const char *chipPath)
{
    strcpy(wrapper->chipPath, chipPath);
    wrapper->connected = 0;
}

int gpio_client_connect(GPIOClientWrapper *wrapper)
{
    wrapper->chip = gpiod_chip_open(wrapper->chipPath);

    if (wrapper->chip == NULL)
    {
        printf("Error opening GPIO chip: %s\n", wrapper->chipPath);
        return 0;
    }

    printf("Connected to GPIO chip: %s\n", wrapper->chipPath);
    wrapper->connected = 1;
    return 1;
}

void gpio_client_disconnect(GPIOClientWrapper *wrapper)
{
    gpiod_chip_close(wrapper->chip);
    wrapper->connected = 0;
    printf("Disconnected from GPIO chip: %s\n", wrapper->chipPath);
}

int gpio_client_read(GPIOClientWrapper *wrapper, int offset, char *value, int max_len)
{
    // Configuring request parameters
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offsets[1] = { (unsigned int)offset };
    gpiod_line_config_add_line_settings(line_cfg, offsets, 1, settings);

    struct gpiod_line_request *request = gpiod_chip_request_lines(wrapper->chip, NULL, line_cfg);

    if (request == NULL)
    {
        printf("Error requesting GPIO line %d\n", offset);
        gpiod_line_config_free(line_cfg);
        gpiod_line_settings_free(settings);
        return 0;
    }

    // Read from GPIO
    enum gpiod_line_value val = gpiod_line_request_get_value(request, (unsigned int)offset);

    if (val == GPIOD_LINE_VALUE_ERROR)
    {
        printf("Error reading GPIO line %d\n", offset);
        gpiod_line_request_release(request);
        gpiod_line_config_free(line_cfg);
        gpiod_line_settings_free(settings);
        return 0;
    }

    snprintf(value, max_len, val == GPIOD_LINE_VALUE_ACTIVE ? "true" : "false");

    gpiod_line_request_release(request);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);

    return 1;
}

int gpio_client_write(GPIOClientWrapper *wrapper, int offset, const char *value)
{
    // Configuring request parameters
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offsets[1] = { (unsigned int)offset };
    gpiod_line_config_add_line_settings(line_cfg, offsets, 1, settings);

    struct gpiod_line_request *request = gpiod_chip_request_lines(wrapper->chip, NULL, line_cfg);

    if (request == NULL)
    {
        printf("Error requesting GPIO line %d\n", offset);
        gpiod_line_config_free(line_cfg);
        gpiod_line_settings_free(settings);
        return 0;
    }

    // Writing value to the output
    int bool_value = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) ? 1 : 0;
    enum gpiod_line_value val = bool_value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;

    int rc = gpiod_line_request_set_value(request, (unsigned int)offset, val);

    gpiod_line_request_release(request);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);

    if (rc != 0)
    {
        printf("Error writing GPIO line %d\n", offset);
        return 0;
    }

    return 1;
}