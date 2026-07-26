#include "MQTT/mqtt_client.h"
#include "Connection_Manager/connection_manager.h"
#include "generalFunctions/config_reader/config_reader.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <ctype.h>

ConnectionManager manager;
pthread_mutex_t connection_mutex = PTHREAD_MUTEX_INITIALIZER;

static void mqtt_calback(char *topic, char *payload);
static void get_iso_timestamp(char *buffer, int max_len);
static int is_numeric_or_bool(const char *value);

int main()
{
    // ===============================================
    // ============ Connection manager ===============
    // ===============================================
    connection_manager_init(&manager);
    connection_manager_connect(&manager);

    // ===============================================
    // ===================== MQTT ====================
    // ===============================================

    // Read mqtt parameter
    char mqttConfFile[20] = "MQTT/mqttConf.txt";
    char mqttCredFile[20] = "MQTT/.env";

    char host[20];
    char portStr[10];
    char username[20];
    char password[20];
    
    // Read files
    get_config_value(mqttConfFile, "host", host, sizeof(host));
    get_config_value(mqttConfFile, "port", portStr, sizeof(portStr));
    get_config_value(mqttCredFile, "MQTT_USERNAME", username, sizeof(username));
    get_config_value(mqttCredFile, "MQTT_PASSWORD", password, sizeof(password));
    int port = atoi(portStr); // Convert to integer

    // Start MQTT
    MQTTClientWrapper MQTT_wrapper;
    mqtt_init(&MQTT_wrapper, host, port, username, password);
    mqtt_set_message_callback(&MQTT_wrapper, mqtt_calback);
    mqtt_connect(&MQTT_wrapper);

    // Subscribe in all write values
    for (int i = 0; i < manager.subscribeTopicCount; i++)
    {
        mqtt_subscribe(&MQTT_wrapper, manager.subscribeTopics[i]);
    }

    // ===============================================
    // ==================== Cycle ====================
    // ===============================================
    char cycleStr[10];
    get_config_value("generalConf.txt", "cycle", cycleStr, sizeof(cycleStr));
    int cycleMs = atoi(cycleStr);
    while (1)
    {
        pthread_mutex_lock(&connection_mutex);
        connection_manager_read_all(&manager);
        pthread_mutex_unlock(&connection_mutex);
        for (int i = 0; i < manager.totalVariables; i++)
        {
            char timestamp[30];
            get_iso_timestamp(timestamp, sizeof(timestamp));

            char payload[300];
            get_iso_timestamp(timestamp, sizeof(timestamp));

            if (is_numeric_or_bool(manager.returnValues[i].value))
            {
                snprintf(payload, sizeof(payload),
                        "{\"name\":\"%s\",\"value\":%s,\"timestamp\":\"%s\"}",
                        manager.returnValues[i].name,
                        manager.returnValues[i].value,   // sem aspas, é número/bool
                        timestamp);
            }
            else
            {
                snprintf(payload, sizeof(payload),
                        "{\"name\":\"%s\",\"value\":\"%s\",\"timestamp\":\"%s\"}",
                        manager.returnValues[i].name,
                        manager.returnValues[i].value,   // com aspas, é texto
                        timestamp);
            }

            mqtt_publish(&MQTT_wrapper, manager.returnValues[i].topic, payload);
        }

        usleep(cycleMs * 1000);
    }

    mqtt_disconnect(&MQTT_wrapper);
    return 0;
}

static void mqtt_calback(char *topic, char *payload)
{
    pthread_mutex_lock(&connection_mutex);
    connection_manager_write(&manager, topic, payload);
    pthread_mutex_unlock(&connection_mutex);
}

static void get_iso_timestamp(char *buffer, int max_len)
{
    time_t now = time(NULL);
    struct tm *utc = gmtime(&now);

    strftime(buffer, max_len, "%Y-%m-%dT%H:%M:%SZ", utc);
}

static int is_numeric_or_bool(const char *value)
{
    if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0)
    {
        return 1;
    }

    int hasDigit = 0;
    for (int i = 0; value[i] != '\0'; i++)
    {
        if (isdigit((unsigned char)value[i]))
        {
            hasDigit = 1;
        }
        else if (value[i] != '.' && value[i] != '-' && value[i] != '+')
        {
            return 0;  // encontrou caractere que não é dígito nem parte de número
        }
    }

    return hasDigit;
}