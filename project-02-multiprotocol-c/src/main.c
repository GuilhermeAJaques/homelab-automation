#include "MQTT/mqtt_client.h"
#include "generalFunctions/config_reader/config_reader.h"
#include "Field_protocols/EthernetIP/ethernet_client.h"
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

    /*
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
*/
    // Simulation
    EthernetIPClientWrapper ethernet_wrapper;
    ethernetip_client_init(&ethernet_wrapper, "192.168.0.152");

    // ===============================================
    // ==================== Cycle ====================
    // ===============================================
    char message[100] = "";
    while (1)
    {
        printf("Entry value for simuOutBOOL: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        ethernetip_client_write(&ethernet_wrapper, "simuOutBOOL", "BOOL", message);

        printf("Entry value for simuOutDINT: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        ethernetip_client_write(&ethernet_wrapper, "simuOutDINT", "DINT", message);

        printf("Entry value for simuOutINT: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        ethernetip_client_write(&ethernet_wrapper, "simuOutINT", "INT", message);

        printf("Entry value for simuOutLINT: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        ethernetip_client_write(&ethernet_wrapper, "simuOutLINT", "LINT", message);

        printf("Entry value for simuOutLREAL: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        ethernetip_client_write(&ethernet_wrapper, "simuOutLREAL", "LREAL", message);

        printf("Entry value for simuOutREAL: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        ethernetip_client_write(&ethernet_wrapper, "simuOutREAL", "REAL", message);

        printf("Entry value for simuOutSINT: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        ethernetip_client_write(&ethernet_wrapper, "simuOutSINT", "SINT", message);

        printf("Entry value for simuOutSTRING: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        ethernetip_client_write(&ethernet_wrapper, "simuOutSTRING", "STRING", message);

        printf("Entry value for simuOutUDINT: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        ethernetip_client_write(&ethernet_wrapper, "simuOutUDINT", "UDINT", message);

        printf("Entry value for simuOutUINT: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        ethernetip_client_write(&ethernet_wrapper, "simuOutUINT", "UINT", message);

        printf("Entry value for simuOutUSINT: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        ethernetip_client_write(&ethernet_wrapper, "simuOutUSINT", "USINT", message);

        if (strcmp(message, "exit") == 0) {
            break;
        }
    }

    //mqtt_disconnect(&MQTT_wrapper);
    return 0;
}