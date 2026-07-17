#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <MQTTClient.h>

typedef void (*MessageCallback)(char *topic, char *payload); // Used to create memory area

#define MAX_TOPICS 20

typedef struct {
    char host[100];
    int port;
    char username[100];
    char password[100];
    MQTTClient client;
    int connected;
    MessageCallback on_message_callback;

    // MQTT subscribed topics, used in reconection case
    char subscribed_topics[MAX_TOPICS][100];
    int topic_count;
} MQTTClientWrapper;

void mqtt_init(MQTTClientWrapper *wrapper, const char *host, int port, const char *username, const char *password);
int mqtt_connect(MQTTClientWrapper *wrapper);
void mqtt_disconnect(MQTTClientWrapper *wrapper);
int mqtt_publish(MQTTClientWrapper *wrapper, const char *topic, const char *payload);
int mqtt_subscribe(MQTTClientWrapper *wrapper, const char *topic);
void mqtt_set_message_callback(MQTTClientWrapper *wrapper, MessageCallback callback);

#endif