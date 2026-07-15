#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <MQTTClient.h>

typedef struct {
    char host[100];
    int port;
    char username[100];
    char password[100];
    MQTTClient client;
    int connected;
} MQTTTClientWrapper;

void mqtt_init(MQTTClientWrapper *wrapper, const char *host, int port, const char *username, const char *password);
int mqtt_connect(MQTTClientWrapper *wrapper);
void mqtt_disconnect(MQTTClientWrapper *wrapper);

#endif