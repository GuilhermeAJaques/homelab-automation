#include "connection_manager.h"
#include "generalFunctions/config_reader/config_reader.h"
#include <dirent.h>
#include <string.h>
#include <stdio.h>

#define MAX_CONNECTIONS 20
#define MAX_VARIABLES 100

static int get_connections(const char *basePath, char path[][30]);
static void get_driver_parameters(Connection *conn, char *driverPath);
static int get_variables_csv(const char *basePath, char line[][100]);
static void get_variables_parameters(ConnectionManager *manager, Connection *conn, char lines[][100]);
static void break_csv_line(char *line, char delimiter, char *fields[]);
static AccessType get_access_type(char *access);

void connection_manager_init(ConnectionManager *manager)
{
    const char basePath[100] = "connections";
    char path[MAX_CONNECTIONS][30];

    // Get connection count
    manager->connectionCount = get_connections(basePath, path);

    // Allocate correct memory size for all connections
    manager->connections = malloc(manager->connectionCount * sizeof(Connection));

    // Check if was correct allocated
    if (manager->connections == NULL)
    {
        printf("Error allocating memory for connections\n");
        manager->connectionCount = 0;
        return;
    }

    manager->totalVariables = 0;

    // Pass by each connection
    for (int i = 0; i < manager->connectionCount; i++)
    {
        // Get driver txt file
        char driverPath[40];
        snprintf(driverPath, sizeof(driverPath), "%s/driver.txt", path[i]);

        // Get driver type
        char driverType[2];
        get_config_value(driverPath, "driver", driverType, sizeof(driverType));
        manager->connections[i].type = atoi(driverType);

        // Get correct parameters from driver.txt
        get_driver_parameters(&manager->connections[i], driverPath);

        // Get variables
        char variablesPath[40];
        snprintf(variablesPath, sizeof(variablesPath), "%s/variables.csv", path[i]);
        char line_variable[MAX_VARIABLES][100];
        manager->connections[i].variableCount = get_variables_csv(variablesPath, line_variable);

        // Allocate variable memorry
        manager->connections[i].variables = malloc(manager->connections[i].variableCount * sizeof(Variables));

        // Get all variable parameters
        get_variables_parameters(manager, &manager->connections[i], line_variable);
    }

    // Allocate memory for all variable values
    manager->returnValues = malloc(manager->totalVariables * sizeof(VariableValues));
}

void connection_manager_connect(ConnectionManager *manager)
{
    if (manager->connections == NULL)
    {
        printf("Connection manager not initialize");
    }
    else
    {
        for (int i = 0; i< manager->connectionCount; i++)
        {
            switch (manager->connections[i].type)
            {
                case DRIVER_GPIO:
                {
                    gpio_client_init(&manager->connections[i].wrapper.gpio, 
                                    manager->connections[i].wrapper.gpio.chipPath);
                    gpio_client_connect(&manager->connections[i].wrapper.gpio);
                    break;
                }
                case DRIVER_S7:
                {
                    s7_client_init(&manager->connections[i].wrapper.s7, 
                                manager->connections[i].wrapper.s7.ip, 
                                manager->connections[i].wrapper.s7.rack, 
                                manager->connections[i].wrapper.s7.slot);
                    s7_client_connect(&manager->connections[i].wrapper.s7);
                    break;
                }
                case DRIVER_ETHERNETIP:
                {
                    ethernetip_client_init(&manager->connections[i].wrapper.ethernetip, 
                                        manager->connections[i].wrapper.ethernetip.ip);
                    break;
                }
                case DRIVER_MODBUS:
                {
                    modbus_client_init(&manager->connections[i].wrapper.modbus, 
                                    manager->connections[i].wrapper.modbus.ip, 
                                    manager->connections[i].wrapper.modbus.port);
                    modbus_client_connect(&manager->connections[i].wrapper.modbus);
                    break;
                }
                case DRIVER_OPCUA:
                {
                    opc_client_init(&manager->connections[i].wrapper.opcua, 
                                    manager->connections[i].wrapper.opcua.url);
                    opc_client_connect(&manager->connections[i].wrapper.opcua);
                    break;
                }
            }
        }
    }
}

void connection_manager_disconnect(ConnectionManager *manager)
{
    if (manager->connections == NULL)
    {
        printf("Connection manager not initialize");
    }
    else
    {
        for (int i = 0; i< manager->connectionCount; i++)
        {
            switch (manager->connections[i].type)
            {
                case DRIVER_GPIO:
                {
                    gpio_client_disconnect(&manager->connections[i].wrapper.gpio);
                    break;
                }
                case DRIVER_S7:
                {
                    s7_client_disconnect(&manager->connections[i].wrapper.s7);
                    break;
                }
                case DRIVER_ETHERNETIP:
                {
                    // Ethernet/IP doesn't need disconnect method
                    break;
                }
                case DRIVER_MODBUS:
                {
                    modbus_client_disconnect(&manager->connections[i].wrapper.modbus);
                    break;
                }
                case DRIVER_OPCUA:
                {
                    opc_client_disconnect(&manager->connections[i].wrapper.opcua);
                    break;
                }
            }
        }
    }
}

void connection_manager_read_all(ConnectionManager *manager)
{
    if (manager->connections == NULL)
    {
        printf("Connection manager not initialize");
    }
    else
    {
        int returnIndex = 0;
        for (int conIndex = 0; conIndex < manager->connectionCount; conIndex++)
        {
            for (int varIndex = 0; varIndex < manager->connections[conIndex].variableCount; varIndex++)
            {
                if (manager->connections[conIndex].variables[varIndex].access == READ_ONLY)
                {
                    VariableParameters parameters = manager->connections[conIndex].variables[varIndex].parameters;
                    switch (manager->connections[conIndex].type)
                    {
                        case DRIVER_GPIO:
                        {
                            gpio_client_read(&manager->connections[conIndex].wrapper.gpio, 
                                             parameters.gpio.offset,
                                             manager->returnValues[returnIndex].value, 
                                             sizeof(manager->returnValues[returnIndex].value));
                            strcpy(manager->returnValues[returnIndex].topic, 
                                   parameters.gpio.topic);
                            strcpy(manager->returnValues[returnIndex].name, 
                                   parameters.gpio.topic);
                            returnIndex++;
                            break;
                        }
                        case DRIVER_S7:
                        {
                            s7_client_read(&manager->connections[conIndex].wrapper.s7,
                                           parameters.s7.db_number,
                                           parameters.s7.offset,
                                           parameters.s7.datatype,
                                           manager->returnValues[returnIndex].value,
                                           sizeof(manager->returnValues[returnIndex].value));
                            strcpy(manager->returnValues[returnIndex].topic, 
                                   parameters.s7.topic);
                            strcpy(manager->returnValues[returnIndex].name, 
                                   parameters.s7.name);
                            returnIndex++;
                            break;
                        }
                        case DRIVER_ETHERNETIP:
                        {
                            ethernetip_client_read(&manager->connections[conIndex].wrapper.ethernetip, 
                                                   parameters.ethernetip.name, 
                                                   parameters.ethernetip.datatype,
                                                   manager->returnValues[returnIndex].value,
                                                   sizeof(manager->returnValues[returnIndex].value));
                            strcpy(manager->returnValues[returnIndex].topic, 
                                   parameters.ethernetip.topic);
                            strcpy(manager->returnValues[returnIndex].name, 
                                   parameters.ethernetip.name);
                            returnIndex++;
                            break;
                        }
                        case DRIVER_MODBUS:
                        {
                            modbus_client_read(&manager->connections[conIndex].wrapper.modbus,
                                               parameters.modbus.address, 
                                               parameters.modbus.datatype,
                                               manager->returnValues[returnIndex].value,
                                               sizeof(manager->returnValues[returnIndex].value));
                            strcpy(manager->returnValues[returnIndex].topic, 
                                   parameters.modbus.topic);
                            strcpy(manager->returnValues[returnIndex].name, 
                                   parameters.modbus.name);
                            returnIndex++;
                            break;
                        }
                        case DRIVER_OPCUA:
                        {
                            opc_client_read(&manager->connections[conIndex].wrapper.opcua, 
                                            parameters.opcua.name,
                                            manager->returnValues[returnIndex].value,
                                            sizeof(manager->returnValues[returnIndex].value));
                            strcpy(manager->returnValues[returnIndex].topic, 
                                   parameters.opcua.topic);
                            strcpy(manager->returnValues[returnIndex].name, 
                                   parameters.opcua.name);
                            returnIndex++;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void connection_manager_read(ConnectionManager *manager, char *topic, VariableValues *value)
{
    if (manager->connections == NULL)
    {
        printf("Connection manager not initialize");
    }
    else
    {
        int finished = 0;
        for (int conIndex = 0; conIndex < manager->connectionCount && finished == 0; conIndex++)
        {
            for (int varIndex = 0; varIndex < manager->connections[conIndex].variableCount && finished == 0; varIndex++)
            {
                VariableParameters parameters = manager->connections[conIndex].variables[varIndex].parameters;
                switch (manager->connections[conIndex].type)
                {
                    case DRIVER_GPIO:
                    {
                        if (strcmp(manager->connections[conIndex].variables[varIndex].parameters.gpio.topic, topic) == 0)
                        {
                            if (manager->connections[conIndex].variables[varIndex].access == READ_ONLY)
                            {
                                gpio_client_read(&manager->connections[conIndex].wrapper.gpio, 
                                                 parameters.gpio.offset,
                                                 value->value, 
                                                 sizeof(value->value));
                                strcpy(value->topic, 
                                       parameters.gpio.topic);
                                strcpy(value->name, 
                                       parameters.gpio.topic);
                                finished = 1;
                            }
                            else
                            {
                                printf("Topic %s is write only", topic);
                                finished = 1;
                            }
                        }
                        break;
                    }
                    case DRIVER_S7:
                    {
                        if (strcmp(manager->connections[conIndex].variables[varIndex].parameters.s7.topic, topic) == 0)
                        {
                            if (manager->connections[conIndex].variables[varIndex].access == READ_ONLY)
                            {
                                s7_client_read(&manager->connections[conIndex].wrapper.s7,
                                                parameters.s7.db_number,
                                                parameters.s7.offset,
                                                parameters.s7.datatype,
                                                value->value, 
                                                sizeof(value->value));
                                strcpy(value->topic, 
                                       parameters.s7.topic);
                                strcpy(value->name, 
                                       parameters.s7.name);
                                finished = 1;
                            }
                            else
                            {
                                printf("Topic %s is write only", topic);
                                finished = 1;
                            }
                        }
                        break;
                    }
                    case DRIVER_ETHERNETIP:
                    {
                        if (strcmp(manager->connections[conIndex].variables[varIndex].parameters.ethernetip.topic, topic) == 0)
                        {
                            if (manager->connections[conIndex].variables[varIndex].access == READ_ONLY)
                            {
                                ethernetip_client_read(&manager->connections[conIndex].wrapper.ethernetip, 
                                                       parameters.ethernetip.name, 
                                                       parameters.ethernetip.datatype,
                                                       value->value, 
                                                       sizeof(value->value));
                                strcpy(value->topic, 
                                       parameters.ethernetip.topic);
                                strcpy(value->name, 
                                       parameters.ethernetip.name);
                                finished = 1;
                            }
                            else
                            {
                                printf("Topic %s is write only", topic);
                                finished = 1;
                            }
                        }
                        break;
                    }
                    case DRIVER_MODBUS:
                    {
                        if (strcmp(manager->connections[conIndex].variables[varIndex].parameters.modbus.topic, topic) == 0)
                        {
                            if (manager->connections[conIndex].variables[varIndex].access == READ_ONLY)
                            {
                                modbus_client_read(&manager->connections[conIndex].wrapper.modbus,
                                                   parameters.modbus.address, 
                                                   parameters.modbus.datatype,
                                                   value->value, 
                                                   sizeof(value->value));
                                strcpy(value->topic, 
                                       parameters.modbus.topic);
                                strcpy(value->name, 
                                       parameters.modbus.name);
                                finished = 1;
                            }
                            else
                            {
                                printf("Topic %s is write only", topic);
                                finished = 1;
                            }
                        }
                        break;
                    }
                    case DRIVER_OPCUA:
                    {
                        if (strcmp(manager->connections[conIndex].variables[varIndex].parameters.opcua.topic, topic) == 0)
                        {
                            if (manager->connections[conIndex].variables[varIndex].access == READ_ONLY)
                            {
                                opc_client_read(&manager->connections[conIndex].wrapper.opcua, 
                                                parameters.opcua.name,
                                                value->value, 
                                                sizeof(value->value));
                                strcpy(value->topic, 
                                       parameters.opcua.topic);
                                strcpy(value->name, 
                                       parameters.opcua.name);
                                finished = 1;
                            }
                            else
                            {
                                printf("Topic %s is write only", topic);
                                finished = 1;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}

void connection_manager_write(ConnectionManager *manager, char *topic, char *value)
{    
    if (manager->connections == NULL)
    {
        printf("Connection manager not initialize");
    }
    else
    {
        int finished = 0;
        for (int conIndex = 0; conIndex < manager->connectionCount && finished == 0; conIndex++)
        {
            for (int varIndex = 0; varIndex < manager->connections[conIndex].variableCount && finished == 0; varIndex++)
            {
                VariableParameters parameters = manager->connections[conIndex].variables[varIndex].parameters;
                switch (manager->connections[conIndex].type)
                {
                    case DRIVER_GPIO:
                    {
                        if (strcmp(manager->connections[conIndex].variables[varIndex].parameters.gpio.topic, topic) == 0)
                        {
                            if (manager->connections[conIndex].variables[varIndex].access == WRITE_ONLY)
                            {
                                gpio_client_write(&manager->connections[conIndex].wrapper.gpio, 
                                                  parameters.gpio.offset,
                                                  value);
                                finished = 1;
                            }
                            else
                            {
                                printf("Topic %s is read only", topic);
                                finished = 1;
                            }
                        }
                        break;
                    }
                    case DRIVER_S7:
                    {
                        if (strcmp(manager->connections[conIndex].variables[varIndex].parameters.s7.topic, topic) == 0)
                        {
                            if (manager->connections[conIndex].variables[varIndex].access == WRITE_ONLY)
                            {
                                s7_client_write(&manager->connections[conIndex].wrapper.s7,
                                                parameters.s7.db_number,
                                                parameters.s7.offset,
                                                parameters.s7.datatype,
                                                value);
                                finished = 1;
                            }
                            else
                            {
                                printf("Topic %s is read only", topic);
                                finished = 1;
                            }
                        }
                        break;
                    }
                    case DRIVER_ETHERNETIP:
                    {
                        if (strcmp(manager->connections[conIndex].variables[varIndex].parameters.ethernetip.topic, topic) == 0)
                        {
                            if (manager->connections[conIndex].variables[varIndex].access == WRITE_ONLY)
                            {
                                ethernetip_client_write(&manager->connections[conIndex].wrapper.ethernetip, 
                                                       parameters.ethernetip.name, 
                                                       parameters.ethernetip.datatype,
                                                       value);
                                finished = 1;
                            }
                            else
                            {
                                printf("Topic %s is read only", topic);
                                finished = 1;
                            }
                        }
                        break;
                    }
                    case DRIVER_MODBUS:
                    {
                        if (strcmp(manager->connections[conIndex].variables[varIndex].parameters.modbus.topic, topic) == 0)
                        {
                            if (manager->connections[conIndex].variables[varIndex].access == WRITE_ONLY)
                            {
                                modbus_client_write(&manager->connections[conIndex].wrapper.modbus,
                                                   parameters.modbus.address, 
                                                   parameters.modbus.datatype,
                                                   value);
                                finished = 1;
                            }
                            else
                            {
                                printf("Topic %s is read only", topic);
                                finished = 1;
                            }
                        }
                        break;
                    }
                    case DRIVER_OPCUA:
                    {
                        if (strcmp(manager->connections[conIndex].variables[varIndex].parameters.opcua.topic, topic) == 0)
                        {
                            if (manager->connections[conIndex].variables[varIndex].access == WRITE_ONLY)
                            {
                                opc_client_write(&manager->connections[conIndex].wrapper.opcua, 
                                                parameters.opcua.name,
                                                value);
                                finished = 1;
                            }
                            else
                            {
                                printf("Topic %s is read only", topic);
                                finished = 1;
                            }
                        }
                    }
                }
            }
        }
    }
}

static int get_connections(const char *basePath, char path[][30])
{
    // Start to search all folder inside connection
    DIR *dir = opendir(basePath);

    if (dir == NULL)
    {
        printf("Error opening directory: %s \n", basePath);
        return 0;
    }

    int count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, "Connection", 10) == 0)
        {
            if (count >= MAX_CONNECTIONS)
            {
                printf("Maximum connection reached");
                closedir(dir);
                return count;
            }
            snprintf(path[count], 30, "%s/%s", basePath, entry->d_name);
            count++;
        }
    }

    closedir(dir);
    return count;
}

static void get_driver_parameters(Connection *conn, char *driverPath)
{
    switch (conn->type)
    {
        case DRIVER_GPIO:
        {
            get_config_value(driverPath, "chip", conn->wrapper.gpio.chipPath, sizeof(conn->wrapper.gpio.chipPath));
            break;
        }
        case DRIVER_S7:
        {
            get_config_value(driverPath, "ip", conn->wrapper.s7.ip, sizeof(conn->wrapper.s7.ip));
            char value[5];
            get_config_value(driverPath, "rack", value, sizeof(value));
            conn->wrapper.s7.rack = atoi(value);
            get_config_value(driverPath, "slot", value, sizeof(value));
            conn->wrapper.s7.slot = atoi(value);
            break;
        }
        case DRIVER_ETHERNETIP:
        {
            get_config_value(driverPath, "ip", conn->wrapper.ethernetip.ip, sizeof(conn->wrapper.ethernetip.ip));
            break;
        }
        case DRIVER_MODBUS:
        {
            get_config_value(driverPath, "ip", conn->wrapper.modbus.ip, sizeof(conn->wrapper.modbus.ip));
            char value[5];
            get_config_value(driverPath, "port", value, sizeof(value));
            conn->wrapper.modbus.port = atoi(value);
            break;
        }
        case DRIVER_OPCUA:
        {
            get_config_value(driverPath, "url", conn->wrapper.opcua.url, sizeof(conn->wrapper.opcua.url));
            break;
        }
    }
}

static int get_variables_csv(const char *basePath, char line[][100])
{
    // Open csv
    FILE *file = fopen(basePath, "r");
    if (file == NULL)
    {
        printf("Error opening file: %s\n", basePath);
        return 0;
    }

    // Read csv
    int count = 0;
    char read_line[300];

    while (fgets(read_line, sizeof(read_line), file) != NULL)
    {
        if (count >= MAX_VARIABLES)
        {
            printf("Maximum variables reached");
            fclose(file);
            return count;
        }
        strcpy(line[count], read_line);
        count++;
    }

    fclose(file);
    return count;
}

static void get_variables_parameters(ConnectionManager *manager, Connection *conn, char lines[][100])
{
    // Read csv
    char *fields[conn->variableCount][6];
    for (int i = 0; i < conn->variableCount; i++)
    {
        break_csv_line(lines[i], ';', fields[i]);
    }

    // Get variables parameters
    for (int i = 0; i < conn->variableCount; i++)
    {
        switch (conn->type)
        {
            case DRIVER_GPIO:
            {
                strcpy(conn->variables[i].parameters.gpio.topic, fields[i][1]);
                conn->variables[i].parameters.gpio.offset = atoi(fields[i][0]);
                conn->variables[i].access = get_access_type(fields[i][2]);
                break;
            }
            case DRIVER_S7:
            {
                conn->variables[i].parameters.s7.db_number = atoi(fields[i][0]);
                strcpy(conn->variables[i].parameters.s7.name, fields[i][1]);
                strcpy(conn->variables[i].parameters.s7.datatype, fields[i][2]);
                strcpy(conn->variables[i].parameters.s7.offset, fields[i][3]);
                strcpy(conn->variables[i].parameters.s7.topic, fields[i][4]);
                conn->variables[i].access = get_access_type(fields[i][5]);
                break;
            }
            case DRIVER_ETHERNETIP:
            {
                strcpy(conn->variables[i].parameters.ethernetip.name, fields[i][0]);
                strcpy(conn->variables[i].parameters.ethernetip.datatype, fields[i][1]);
                strcpy(conn->variables[i].parameters.ethernetip.topic, fields[i][2]);
                conn->variables[i].access = get_access_type(fields[i][3]);
                break;
            }
            case DRIVER_MODBUS:
            {
                strcpy(conn->variables[i].parameters.modbus.name, fields[i][0]);
                strcpy(conn->variables[i].parameters.modbus.address, fields[i][1]);
                strcpy(conn->variables[i].parameters.modbus.datatype, fields[i][2]);
                strcpy(conn->variables[i].parameters.modbus.topic, fields[i][3]);
                conn->variables[i].access = get_access_type(fields[i][4]);
                break;
            }
            case DRIVER_OPCUA:
            {
                strcpy(conn->variables[i].parameters.opcua.name, fields[i][0]);
                strcpy(conn->variables[i].parameters.opcua.topic, fields[i][1]);
                conn->variables[i].access = get_access_type(fields[i][2]);
                break;
            }
        }
        if (conn->variables[i].access == READ_ONLY)
        {
            manager->totalVariables++;
        }
    }
}

static void break_csv_line(char *line, char delimiter, char *fields[])
{
    char delimStr[3] = { delimiter, '\n', '\0' };
    int count = 0;

    char *token = strtok(line, delimStr);

    while (token != NULL && count < 20)
    {
        fields[count] = token;
        count++;
        token = strtok(NULL, delimStr);
    }
}

static AccessType get_access_type(char *access)
{
    if (strcmp(access, "r") == 0)
    {
        return READ_ONLY;
    }
    else
    {
        return WRITE_ONLY;
    }
}