#include "MQTT/mqtt_client.h"
#include "generalFunctions/config_reader/config_reader.h"
#include "Field_protocols/EthernetIP/ethernet_client.h"
#include "Field_protocols/Modbus/modbus_client.h"
#include "Field_protocols/S7/s7_client.h"
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

    S7ClientWrapper s7_wrapper;
    s7_client_init(&s7_wrapper, "192.168.0.200", 0, 1);
    s7_client_connect(&s7_wrapper);

    // ===============================================
    // ==================== Cycle ====================
    // ===============================================
    char message[100] = "";
    while (1)
    {
        printf("Type exit to exit");
        fgets(message, sizeof(message), stdin);
        char message[100];
        int db_number = 1;  // ajuste conforme o DB real

        printf("Entry value for byte: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "269", "BYTE", message);

        printf("Entry value for bool[0]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "270.0", "BOOL", message);

        printf("Entry value for bool[1]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "270.1", "BOOL", message);

        printf("Entry value for bool[2]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "270.2", "BOOL", message);

        printf("Entry value for bool[3]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "270.3", "BOOL", message);

        printf("Entry value for bool[4]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "270.4", "BOOL", message);

        printf("Entry value for bool[5]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "270.5", "BOOL", message);

        printf("Entry value for bool[6]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "270.6", "BOOL", message);

        printf("Entry value for bool[7]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "270.7", "BOOL", message);

        printf("Entry value for bool[8]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "271.0", "BOOL", message);

        printf("Entry value for bool[9]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "271.1", "BOOL", message);

        printf("Entry value for bool[10]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "271.2", "BOOL", message);

        printf("Entry value for bool[11]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "271.3", "BOOL", message);

        printf("Entry value for bool[12]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "271.4", "BOOL", message);

        printf("Entry value for bool[13]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "271.5", "BOOL", message);

        printf("Entry value for bool[14]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "271.6", "BOOL", message);

        printf("Entry value for bool[15]: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "271.7", "BOOL", message);

        printf("Entry value for sint: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "272", "SINT", message);

        printf("Entry value for usint: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "273", "USINT", message);

        printf("Entry value for word: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "274", "WORD", message);

        printf("Entry value for int: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "276", "INT", message);

        printf("Entry value for uint: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "278", "UINT", message);

        printf("Entry value for dword: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "280", "DWORD", message);

        printf("Entry value for dint: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "284", "DINT", message);

        printf("Entry value for udint: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "288", "UDINT", message);

        printf("Entry value for real: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "292", "REAL", message);

        printf("Entry value for lreal: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "296", "LREAL", message);

        printf("Entry value for string20: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "304", "STRING[20]", message);

        printf("Entry value for string: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0';
        s7_client_write(&s7_wrapper, db_number, "326", "STRING", message);

        if (strcmp(message, "exit") == 0) {
            break;
        }
    }

    mqtt_disconnect(&MQTT_wrapper);
    return 0;
}