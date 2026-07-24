#include "MQTT/mqtt_client.h"
#include "Connection_Manager/connection_manager.h"
#include "generalFunctions/config_reader/config_reader.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

ConnectionManager manager;

void mqtt_calback(char *topic, char *payload)
{
    connection_manager_write(&manager, topic, payload);
}

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
    mqtt_subscribe(&MQTT_wrapper, "#");


    // ===============================================
    // ==================== Cycle ====================
    // ===============================================
    while (1)
    {
        connection_manager_read_all(&manager);
        for (int i = 0; i < manager.totalVariables; i++)
        {
            mqtt_publish(&MQTT_wrapper, 
                         manager.returnValues[i].topic, 
                         manager.returnValues[i].value);
        }
    }

    mqtt_disconnect(&MQTT_wrapper);
    return 0;
}