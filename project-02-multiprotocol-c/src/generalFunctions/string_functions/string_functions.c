#include <string.h>

void to_lowercase(char *str)
{
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = tolower(str[i]);
    }
}