#include <stdio.h>
#include <string.h>
#include "mqtt_client.h"

void mqtt_init(MQTTClientWrapper *wrapper, const char *host, int port, const char *username, const char *password)
{
    strcpy(wrapper->host, host);
    wrapper->port = port;
    strcpy(wrapper->username, username);
    strcpy(wrapper->password, password);
    wrapper->connected = 0;
}

int mqtt_connect(MQTTClientWrapper *wrapper)
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    char address[150];
    sprintf(address, "tcp://%s:%d", wrapper->host, wrapper->port);

    MQTTClient_create(&wrapper->client, address, "c_client", MQTTCLIENT_PERSISTENCE_NONE, NULL);

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = wrapper->username;
    conn_opts.password = wrapper->password;
    rc = MQTTClient_connect(wrapper->client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        wrapper->connected = 0;
        return rc;
    }

    printf("Connected to MQTT broker\n");
    wrapper->connected = 1;
    return MQTTCLIENT_SUCCESS;
}

void mqtt_disconnect(MQTTClientWrapper *wrapper) 
{
    MQTTClient_disconnect(wrapper->client, 10000);
    MQTTClient_destroy(&wrapper->client);
    wrapper->connected = 0;
    printf("Disconnected from MQTT broker\n");
}

int mqtt_publish(MQTTClientWrapper *wrapper, const char *topic, const char *payload) 
{
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    pubmsg.payload = (void *)payload;
    pubmsg.payloadlen = strlen(payload);
    pubmsg.qos = 0;
    pubmsg.retained = 0;

    MQTTClient_publishMessage(wrapper->client, topic, &pubmsg, &token);
    MQTTClient_waitForCompletion(wrapper->client, token, 5000);

    return 0;
}

int mqtt_subscribe(MQTTClientWrapper *wrapper, const char *topic) 
{
    int rc = MQTTClient_subscribe(wrapper->client, topic, 0);
    return rc;
}