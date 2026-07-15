#include <stdio.h>
#include <string.h>
#include <mqtt_client.h>

void mqtt_init(MQTTClientWrapper *wrapper, const char *host, int port, const char *username, const char *password)
{
    strcpy(wrapper->host, host);
    wrapper-> = port;
    strcpy(wrapper->username, username);
    strcpy(wrapper->password, password);
    wrapper->connected = 0;
}