#include <stdio.h>
#include <string.h>
#include "mqtt_client.h"
#include <unistd.h>

// Initialize MQTT parameters
void mqtt_init(MQTTClientWrapper *wrapper, const char *host, int port, const char *username, const char *password)
{
    strcpy(wrapper->host, host);
    wrapper->port = port;
    strcpy(wrapper->username, username);
    strcpy(wrapper->password, password);
    wrapper->connected = 0;
    wrapper->on_message_callback = NULL;
}

// Method that calls the method in main.c
static int on_message_arrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) 
{
    // Convert context to MQTTClientWrapper
    MQTTClientWrapper *wrapper = (MQTTClientWrapper *)context;

    // Organize string
    char payload[500];
    int len = message->payloadlen;
    if (len > 499) len = 499;
    memcpy(payload, message->payload, len); // Used to ensure that the final value will be a string
    payload[len] = '\0';

    if (wrapper->on_message_callback != NULL) 
    {
        wrapper->on_message_callback(topicName, payload);
    }

    // Release memory arrea
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

static void mqtt_reconnect_loop(MQTTClientWrapper *wrapper)
{
    int rc;

    while (1) {
        printf("Attempting to reconnect...\n");

        MQTTClient_destroy(&wrapper->client); // Release memory area
        rc = mqtt_connect(wrapper);

        if (rc == MQTTCLIENT_SUCCESS) {
            printf("Reconnected successfully\n");
            return;
        }

        printf("Reconnect failed, retrying in 5 seconds...\n");
        sleep(5);
        // This will lock the code while trying to reconnect
        // This is not a big problem for this project because 
        // this device doesn't have a internal buffer from PLCs
    }
}

static void on_connection_lost(void *context, char *cause)
{
    MQTTClientWrapper *wrapper = (MQTTClientWrapper *)context;

    printf("Connection lost: %s\n", cause);
    wrapper->connected = 0;

    mqtt_reconnect_loop(wrapper);
}

int mqtt_connect(MQTTClientWrapper *wrapper)
{
    // Get the standart structure for MQTT parameters
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    char address[150];
    sprintf(address, "tcp://%s:%d", wrapper->host, wrapper->port);

    // Create MQTT client
    MQTTClient_create(&wrapper->client, address, "c_client", MQTTCLIENT_PERSISTENCE_NONE, NULL);

    // MQTT parameters
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = wrapper->username;
    conn_opts.password = wrapper->password;

    // Set method that will be triggered when receive MQTT message
    MQTTClient_setCallbacks(wrapper->client, wrapper, on_connection_lost, on_message_arrived, NULL);

    // Connect MQTT broker
    rc = MQTTClient_connect(wrapper->client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) 
    {
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
    MQTTClient_destroy(&wrapper->client); // Release memory area
    wrapper->connected = 0;
    printf("Disconnected from MQTT broker\n");
}

int mqtt_publish(MQTTClientWrapper *wrapper, const char *topic, const char *payload) 
{
    MQTTClient_message pubmsg = MQTTClient_message_initializer; // Start parameters for MQTT publish
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

void mqtt_set_message_callback(MQTTClientWrapper *wrapper, MessageCallback callback) 
{
    wrapper->on_message_callback = callback;
}