#ifndef MODBUS_CLIENT_H
#define MODBUS_CLIENT_H

#include <modbus/modbus.h>

typedef struct {
    char ip[100];
    int port;
    modbus_t *client;
    int connected;
} ModbusClientWrapper;

void modbus_client_init(ModbusClientWrapper *wrapper, const char *ip, int port);
int modbus_client_connect(ModbusClientWrapper *wrapper);
int modbus_client_read(ModbusClientWrapper *wrapper, const char *address, const char *datatype);
int modbus_client_write(ModbusClientWrapper *wrapper, const char *address, const char *datatype, int value);
void modbus_client_disconnect(ModbusClientWrapper *wrapper);

#endif