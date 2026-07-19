#ifndef CONFIG_READER_H
#define CONFIG_READER_H

void get_config_value(const char *filename, const char *key, char *value_out, int max_len);

#endif