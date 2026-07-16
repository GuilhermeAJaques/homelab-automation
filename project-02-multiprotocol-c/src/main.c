#include "MQTT/mqtt_client.h"
#include <string.h>

int main()
{
    MQTTClientWrapper wrapper;

    char host[20] = "192.168.0.102";
    int port = 1883;
    char username[20] = "";
    char password[20] = "";


    mqtt_init(&wrapper, host, port, username, password);
    mqtt_connect(&wrapper);

    char message[100] = "";

    while (1)
    {
        printf("Type exit to exit from the code");
        fgets(message, sizeof(message), stdin);

        if (strcmp(message, "exit") == 0) {
            break;
        }

        mqtt_publish(&wrapper, "test", message);
    }

    mqtt_disconnect(&wrapper);
}