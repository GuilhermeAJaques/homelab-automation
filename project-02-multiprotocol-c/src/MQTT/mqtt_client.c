#include <stdio.h>
#include <string.h>
#include "mqtt_client.h"
#include <unistd.h>
#include <pthread.h>

// Declare methods used above
static int on_message_arrived(void *context, char *topicName, int topicLen, MQTTClient_message *message);
static void mqtt_reconnect_loop(MQTTClientWrapper *wrapper);
static void on_connection_lost(void *context, char *cause);
static void *reconnect_thread(void *arg);

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

    MQTTClient_setCallbacks(wrapper->client, wrapper, on_connection_lost, on_message_arrived, NULL);

    while(1)
    {
        rc = MQTTClient_connect(wrapper->client, &conn_opts);
        if (rc == MQTTCLIENT_SUCCESS) 
        {
            printf("Connected to MQTT broker\n");
            wrapper->connected = 1;
            return MQTTCLIENT_SUCCESS;
        }

        printf("Failed to connect, return code %d\n", rc);
        wrapper->connected = 0;
        printf("Reconnect failed, retrying in 5 seconds...\n");
        sleep(5);
    }
}

void mqtt_set_message_callback(MQTTClientWrapper *wrapper, MessageCallback callback) 
{
    wrapper->on_message_callback = callback;
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
    // Check if topic already exists in list
    int new = 1;
    for (int i = 0; i < wrapper->topic_count; i++)
    {
        if (strcmp(wrapper->subscribed_topics[i], topic) == 0)
        {
            new = 0;
            break;
        }
    }
    if (new == 1) // If doesn't exists
    {
        strcpy(wrapper->subscribed_topics[wrapper->topic_count], topic);
        wrapper->topic_count++;
    }
    int rc = MQTTClient_subscribe(wrapper->client, topic, 0);
    return rc;
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

static void on_connection_lost(void *context, char *cause)
{
    MQTTClientWrapper *wrapper = (MQTTClientWrapper *)context;

    printf("Connection lost: %s\n", cause);
    wrapper->connected = 0;

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, reconnect_thread, wrapper);
    pthread_detach(thread_id);
}

static void *reconnect_thread(void *arg)
{
    MQTTClientWrapper *wrapper = (MQTTClientWrapper *)arg;

    MQTTClient_destroy(&wrapper->client);
    int rc = mqtt_connect(wrapper);

    printf("Reconnected successfully\n");

    for (int i = 0; i < wrapper->topic_count; i++) {
        mqtt_subscribe(wrapper, wrapper->subscribed_topics[i]);
    }
    return NULL;
}