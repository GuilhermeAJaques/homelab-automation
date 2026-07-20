#ifndef MODBUS_CLIENT_H
#define MODBUS_CLIENT_H

#include <modbus/modbus.h>
// Constants

#define MODBUS_ACCESS_READ 0
#define MODBUS_ACCESS_WRITE 1

typedef struct {
    char ip[100];
    int port;
    modbus_t *client;
    int connected;
} ModbusClientWrapper;

typedef struct {
    int addr;
    int fc;
} Modbus_Address_FC;

void modbus_client_init(ModbusClientWrapper *wrapper, const char *ip, int port);
int modbus_client_connect(ModbusClientWrapper *wrapper);
int modbus_client_read(ModbusClientWrapper *wrapper, const char *address, const char *datatype, char *value, int max_len);
int modbus_client_write(ModbusClientWrapper *wrapper, const char *address, const char *datatype, const char *value);
void modbus_client_disconnect(ModbusClientWrapper *wrapper);

#endif