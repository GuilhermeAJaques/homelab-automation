#include "MQTT/mqtt_client.h"
#include "generalFunctions/config_reader/config_reader.h"
#include "Field_protocols/EthernetIP/ethernet_client.h"
#include "Field_protocols/Modbus/modbus_client.h"
#include "Field_protocols/S7/s7_client.h"
#include "Field_protocols/OPCUA/opc-ua_client.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void mqtt_calback(char *topic, char *payload)
{
    printf("Received MQTT message %s: %s\n", topic, payload);
}

int main()
{

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
    char message[100] = "";
    while (1)
    {
        
    }

    mqtt_disconnect(&MQTT_wrapper);
    return 0;
}