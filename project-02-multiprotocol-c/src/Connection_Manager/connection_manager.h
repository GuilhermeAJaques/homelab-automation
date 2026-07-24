#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include "Field_protocols/EthernetIP/ethernet_client.h"
#include "Field_protocols/Modbus/modbus_client.h"
#include "Field_protocols/S7/s7_client.h"
#include "Field_protocols/OPCUA/opc-ua_client.h"
#include "GPIO/gpio.h"

typedef enum {
    DRIVER_GPIO = 0,
    DRIVER_S7 = 1,
    DRIVER_ETHERNETIP = 2,
    DRIVER_MODBUS = 3,
    DRIVER_OPCUA = 4,
} DriverType;

typedef enum {
    READ_ONLY = 0,
    WRITE_ONLY = 1,
} AccessType;

typedef union {
    GPIOClientWrapper gpio;
    S7ClientWrapper s7;
    EthernetIPClientWrapper ethernetip;
    ModbusClientWrapper modbus;
    OPCClientWrapper opcua;
} DriverWrapper;

typedef struct {
    int offset;
    char topic[50];
} GPIO_Variable_Parameter;

typedef struct {
    int db_number;
    char name[20];
    char datatype[20];
    char offset[10];
    char topic[50];
} S7_Variable_Parameter;

typedef struct {
    char name[20];
    char datatype[20];
    char topic[50];
} Ethernet_Variable_Parameter;

typedef struct {
    char name[20];
    char address[16];
    char datatype[20];
    char topic[50];
} Modbus_Variable_Parameter;

typedef struct {
    char name[20];
    char topic[50];
} OPC_Variable_Parameter;

typedef union {
    GPIO_Variable_Parameter gpio;
    S7_Variable_Parameter s7;
    Ethernet_Variable_Parameter ethernetip;
    Modbus_Variable_Parameter modbus;
    OPC_Variable_Parameter opcua;
} VariableParameters;

typedef struct {
    AccessType access;
    VariableParameters parameters;
} Variables;

typedef struct {
    DriverType type;
    DriverWrapper wrapper;
    Variables *variables;
    int variableCount;
} Connection;

typedef struct {
    char name[100];
    char topic[100];
    char value[100];
} VariableValues;

typedef struct {
    Connection *connections;
    int connectionCount;
    VariableValues *returnValues;
    int totalVariables;
} ConnectionManager;

void connection_manager_init(ConnectionManager *manager);
void connection_manager_connect(ConnectionManager *manager);
void connection_manager_disconnect(ConnectionManager *manager);
void connection_manager_read_all(ConnectionManager *manager);
void connection_manager_read(ConnectionManager *manager, char *topic, VariableValues *value);
void connection_manager_write(ConnectionManager *manager, char *topic, char *value);


#endif