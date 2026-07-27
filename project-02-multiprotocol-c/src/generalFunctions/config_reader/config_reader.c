#include <stdio.h>
#include <string.h>
#include "config_reader.h"
#include "Logger/logger.h"

void get_config_value(const char *filename, const char *key, char *value_out, int max_len)
{
    // Read file
    FILE *file = fopen(filename, "r"); // r = read mode

    if (file == NULL)
    {
        logger_log(CLASS_GENERAL, LOG_ERROR , "Error opening file: %s", filename);
        return;
    }


    char line[200];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        // Search for "=" char
        char *equals_pos = strchr(line, '=');
        if (equals_pos == NULL) 
        {
            continue;
        }

        // If found "="
        int key_len = equals_pos - line; 
        char found_key[100];

        // Get parameter name
        strncpy(found_key, line, key_len);
        found_key[key_len] = '\0';

        // Check if is the desired parameter
        if (strcmp(found_key, key) == 0) 
        {
            // Get parameter value
            char *value_start = equals_pos + 1;
            value_start[strcspn(value_start, "\n")] = '\0';

            // Return value
            strncpy(value_out, value_start, max_len);
            value_out[max_len - 1] = '\0';
            fclose(file);
            return;
        }
    }

    // Excue if couldn't find any parameter
    fclose(file);
    logger_log(CLASS_GENERAL, LOG_WARNING , "Key not found: %s", key);
}