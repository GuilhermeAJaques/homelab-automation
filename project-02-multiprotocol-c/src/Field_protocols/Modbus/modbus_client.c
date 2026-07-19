#include <string.h>
#include <stdio.h>
#include "modbus_client.h"

void modbus_client_init(ModbusClientWrapper *wrapper, const char *ip, int port)
{
    strcpy(wrapper->ip, ip);
    wrapper->port = port;
    wrapper->connected = 0;
}

int modbus_client_connect(ModbusClientWrapper *wrapper)
{
    wrapper->client = modbus_new_tcp(wrapper->ip, wrapper->port);

    if (wrapper->client == NULL)
    {
        printf("Error to create Modbus TCP client\n");
        return -1;
    }

    int result = modbus_connect(wrapper->client);

    if (result == 0)
    {
        printf("Connected to Modbus TCP server\n");
        wrapper->connected = 1;
        return result;
    }
    else
    {
        printf("Error to Modbus TCP server\n");
        wrapper->connected = 0;
        return result;
    }
}

void modbus_client_disconnect(ModbusClientWrapper *wrapper)
{
    // Disconect from Modbus TCP server
    modbus_close(wrapper->client);
    printf("Disconnected from Modbus TCP server\n");
    wrapper->connected = 0;

    // Release memory area
    modbus_free(wrapper->client);
}