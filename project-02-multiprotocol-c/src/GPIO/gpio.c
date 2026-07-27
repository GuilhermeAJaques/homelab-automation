#include "gpio.h"
#include <string.h>
#include <stdio.h>
#include "Logger/logger.h"

void gpio_client_init(GPIOClientWrapper *wrapper, const char *chipPath)
{
    strcpy(wrapper->chipPath, chipPath);
    wrapper->connected = 0;
    wrapper->output_request = NULL;
}

int gpio_client_connect(GPIOClientWrapper *wrapper, int *output_pins, int output_count)
{
    wrapper->chip = gpiod_chip_open(wrapper->chipPath);

    if (wrapper->chip == NULL)
    {
        logger_log(CLASS_GPIO, LOG_ERROR , "Error opening GPIO chip: %s", wrapper->chipPath);
        return 0;
    }

    logger_log(CLASS_GPIO, LOG_INFO , "Connected to GPIO chip: %s", wrapper->chipPath);
    wrapper->connected = 1;
    
    if (output_count > 0)
    {
        struct gpiod_line_settings *settings = gpiod_line_settings_new();
        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);

        struct gpiod_line_config *line_cfg = gpiod_line_config_new();
        unsigned int offsets[output_count];

        for (int i = 0; i < output_count; i++)
        {
            offsets[i] = (unsigned int)output_pins[i];
        }

        gpiod_line_config_add_line_settings(line_cfg, offsets, output_count, settings);

        wrapper->output_request = gpiod_chip_request_lines(wrapper->chip, NULL, line_cfg);

        gpiod_line_config_free(line_cfg);
        gpiod_line_settings_free(settings);

        if (wrapper->output_request == NULL)
        {
            logger_log(CLASS_GPIO, LOG_ERROR , "Error requesting GPIO output lines");
            return 0;
        }
    }

    return 1;
}

void gpio_client_disconnect(GPIOClientWrapper *wrapper)
{
    gpiod_chip_close(wrapper->chip);
    wrapper->connected = 0;
    logger_log(CLASS_GPIO, LOG_INFO , "Disconnected from GPIO chip: %s", wrapper->chipPath);
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
        logger_log(CLASS_GPIO, LOG_ERROR , "Error requesting GPIO line %d", offset);
        gpiod_line_config_free(line_cfg);
        gpiod_line_settings_free(settings);
        return 0;
    }

    // Read from GPIO
    enum gpiod_line_value val = gpiod_line_request_get_value(request, (unsigned int)offset);

    if (val == GPIOD_LINE_VALUE_ERROR)
    {
        logger_log(CLASS_GPIO, LOG_ERROR , "Error reading GPIO line %d", offset);
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
    if (wrapper->output_request == NULL)
    {
        logger_log(CLASS_GPIO, LOG_ERROR , "Error: output lines not requested");
        return 0;
    }

    int bool_value = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) ? 1 : 0;
    enum gpiod_line_value val = bool_value ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;

    int rc = gpiod_line_request_set_value(wrapper->output_request, (unsigned int)offset, val);

    if (rc != 0)
    {
        logger_log(CLASS_GPIO, LOG_ERROR , "Error writing GPIO line %d", offset);
        return 0;
    }

    return 1;
}