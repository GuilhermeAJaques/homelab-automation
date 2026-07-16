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
} MQTTClientWrapper;

void mqtt_init(MQTTClientWrapper *wrapper, const char *host, int port, const char *username, const char *password);
int mqtt_connect(MQTTClientWrapper *wrapper);
void mqtt_disconnect(MQTTClientWrapper *wrapper);
int mqtt_publish(MQTTClientWrapper *wrapper, const char *topic, const char *payload);
int mqtt_subscribe(MQTTClientWrapper *wrapper, const char *topic);

#endif