#include "MQTT/mqtt_client.h"
#include "config_reader/config_reader.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void mqtt_calback(char *topic, char *payload)
{
    printf("Received MQTT message %s: %s\n", topic, payload);
}

int main()
{
    MQTTClientWrapper wrapper;

    // Read mqtt parameter
    char mqttConfFile[20] = "MQTT/mqttConf.txt";
    char mqttCredFile[20] = "MQTT/.env";

    char host[20];
    char portStr[10];
    char username[20];
    char password[20];
    
    // Read files
    get_config_value(mqttConfFile, "host", host, 20);
    get_config_value(mqttConfFile, "port", portStr, 20);
    get_config_value(mqttCredFile, "MQTT_USERNAME", username, 20);
    get_config_value(mqttCredFile, "MQTT_PASSWORD", password, 20);

    int port = atoi(portStr); // Convert to integer

    // Start MQTT
    mqtt_init(&wrapper, host, port, username, password);
    mqtt_set_message_callback(&wrapper, mqtt_calback);
    mqtt_connect(&wrapper);
    mqtt_subscribe(&wrapper, "#");

    char message[100] = "";

    while (1)
    {
        printf("Type exit to exit from the code or MQTT message:\n");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';

        if (strcmp(message, "exit") == 0) {
            break;
        }

        mqtt_publish(&wrapper, "test", message);
    }

    mqtt_disconnect(&wrapper);
    return 0;
}