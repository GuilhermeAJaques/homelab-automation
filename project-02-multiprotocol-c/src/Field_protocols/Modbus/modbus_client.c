#include <string.h>
#include <stdio.h>
#include "modbus_client.h"
#include <stdlib.h>
#include "../../generalFunctions/string_functions/string_functions.h"
#include <errno.h>



// Declare functions
static void is_not_connected(ModbusClientWrapper *wrapper, int err);
static Modbus_Address_FC getAddress_FC(char *fullAddress, int access);
static int get_dt_size(const char *datatype);
static void convert_to_string(uint16_t registers[50],const char *datatype, char *value, int max_len);
static void convert_from_string(const char *value,const char *datatype, uint16_t registers[50]);

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
        printf("Error to connect Modbus TCP server: %d\n", result);
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

int modbus_client_read(ModbusClientWrapper *wrapper, const char *address, const char *datatype, char *value, int max_len)
{
    if (!wrapper->connected)
    {
        // If not connected yed, connect
        // This flow is more commom for recconection cases
        modbus_client_connect(wrapper);
    }

    Modbus_Address_FC addr_fc = getAddress_FC((char *)address, MODBUS_ACCESS_READ);

    if (addr_fc.fc == -1)
    {
        printf("Invalid address: %s\n", address);
        return 0;
    }

    int nb = get_dt_size(datatype);
    uint16_t registers[50];
    uint8_t bits[1];
    int rc;

    if (addr_fc.fc == 1)
    {
        rc = modbus_read_bits(wrapper->client, addr_fc.addr, 1, bits);
    }
    else if (addr_fc.fc == 2)
    {
        rc = modbus_read_input_bits(wrapper->client, addr_fc.addr, 1, bits);
    }
    else if (addr_fc.fc == 3)
    {
        rc = modbus_read_registers(wrapper->client, addr_fc.addr, nb, registers);
    }
    else if (addr_fc.fc == 4)
    {
        rc = modbus_read_input_registers(wrapper->client, addr_fc.addr, nb, registers);
    }
    else
    {
        printf("Unknown function code for address %s\n", address);
        return 0;
    }

    if (rc == -1)
    {
        printf("Error reading Modbus address %s: %s\n", address, modbus_strerror(errno));
        is_not_connected(wrapper, errno);
        return 0;
    }
    else
    {
        if (addr_fc.fc >= 3)
        {
            convert_to_string(registers, datatype, value, max_len);
            return 1;
        }
        else
        {
            snprintf(value, max_len, bits[0] ? "true" : "false");
            return 1;
        }
    }
}

int modbus_client_write(ModbusClientWrapper *wrapper, const char *address, const char *datatype, const char *value)
{
    if (!wrapper->connected)
    {
        // If not connected yed, connect
        // This flow is more commom for recconection cases
        modbus_client_connect(wrapper);
    }

    Modbus_Address_FC addr_fc = getAddress_FC((char *)address, MODBUS_ACCESS_WRITE);

    if (addr_fc.fc == -1)
    {
        printf("Invalid address: %s\n", address);
        return 0;
    }
    int nb = get_dt_size(datatype);
    
    int rc;
    if (addr_fc.fc == 5)
    {
        int bool_value = 0;
        if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0)
        {
            bool_value = 1;
        }
        else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0)
        {
            bool_value = 0;
        }
        else
        {
            printf("Invalid boolean value: %s (expected true/false/1/0)\n", value);
        }

        rc = modbus_write_bit(wrapper->client, addr_fc.addr, bool_value);
    }
    else if (addr_fc.fc == 6)
    {
        uint16_t registers[50];
        convert_from_string(value, datatype, registers);
        rc = modbus_write_register(wrapper->client, addr_fc.addr, registers[0]);
    }
    else if (addr_fc.fc == 16)
    {
        uint16_t registers[50];
        convert_from_string(value, datatype, registers);
        rc = modbus_write_registers(wrapper->client, addr_fc.addr, nb, registers);
    }
    else
    {
        printf("Unknown function code for address %s\n", address);
        return 0;
    }

    if (rc == -1)
    {
        printf("Error writing Modbus address %s: %s\n", address, modbus_strerror(errno));
        is_not_connected(wrapper, errno);
        return 0;
    }
    return 1;
}

static void is_not_connected(ModbusClientWrapper *wrapper, int err)
{
    if (err == ECONNRESET || // Connection reseted
        err == ETIMEDOUT ||  // Timeout
        err == EPIPE ||  // Connection broken
        err == ECONNREFUSED) // Connection refused
    {
        printf("Connection lost, attempting to reconnect...\n");
        wrapper->connected = 0;
        modbus_close(wrapper->client);
        modbus_free(wrapper->client);
        modbus_client_connect(wrapper);
    }
}

static Modbus_Address_FC getAddress_FC(char *fullAddress, int access)
{
    Modbus_Address_FC out;
    out.fc = -1;
    out.addr = -1;
    // ===========================
    // ========= Get FC ==========
    // ===========================
    // Get address start
    char prefix[4];
    strncpy(prefix, fullAddress, 3);
    prefix[3] = '\0';
    to_lowercase(prefix);

    if (access == MODBUS_ACCESS_READ) // Read
    {
        if (strcmp(prefix, "%mx") == 0 || 
            strcmp(prefix, "%qx") == 0)
        {
            out.fc = 1;
        }
        else if (strcmp(prefix, "%ix") == 0)
        {
            out.fc = 2;
        }
        else if (strcmp(prefix, "%mw") == 0 || 
                 strcmp(prefix, "%qw") == 0 ||
                 strcmp(prefix, "%md") == 0 || 
                 strcmp(prefix, "%mb") == 0 ||
                 strcmp(prefix, "%qb") == 0 || 
                 strcmp(prefix, "%qd") == 0)
        {
            out.fc = 3;
        }
        else if (strcmp(prefix, "%iw") == 0 || 
                 strcmp(prefix, "%id") == 0 || 
                 strcmp(prefix, "%ib") == 0)
        {
            out.fc = 4;
        }
        else
        {
            printf("Unknown address prefix: %s\n", prefix);
        }
    }
    else if (access == MODBUS_ACCESS_WRITE) // write
    {
        if (strcmp(prefix, "%mx") == 0 ||
            strcmp(prefix, "%ix") == 0 ||
            strcmp(prefix, "%qx") == 0)
        {
            out.fc = 5;
        }
        else if (strcmp(prefix, "%mw") == 0 ||
                 strcmp(prefix, "%qw") == 0 ||
                 strcmp(prefix, "%iw") == 0)
        {
            out.fc = 6;
        }
        else if (strcmp(prefix, "%md") == 0 || 
                 strcmp(prefix, "%qd") == 0 ||
                 strcmp(prefix, "%id") == 0 || 
                 strcmp(prefix, "%mb") == 0 ||
                 strcmp(prefix, "%qb") == 0 || 
                 strcmp(prefix, "%ib") == 0)
        {
            out.fc = 16;
        }
        else
        {
            printf("Unknown address prefix: %s\n", prefix);
        }
    }
    else
    {
        printf("Wrong access mode on getAddress_FC: %i", access);
    }

    // ===========================
    // ======= Get Address =======
    // ===========================
    const char *address_part = fullAddress + 3;

    // Check if has '.'
    const char *dot_pos = strchr(address_part, '.');
    if (dot_pos != NULL)
    {
        int byte_len = dot_pos - address_part;
        char byte_str[10];
        strncpy(byte_str, address_part, byte_len);
        byte_str[byte_len] = '\0';

        int byte = atoi(byte_str);
        int bit = atoi(dot_pos + 1);

        if (byte % 2 == 0)
        {
            byte += 1;
        }
        else
        {
            byte -= 1;
        }

        out.addr = (byte * 8) + bit;
    }
    else if (strcmp(prefix, "%md") == 0 || 
             strcmp(prefix, "%qd") == 0 ||
             strcmp(prefix, "%id") == 0)
    {
        out.addr = atoi(address_part) * 2;
    }
    else if (strcmp(prefix, "%mb") == 0 || 
             strcmp(prefix, "%qb") == 0)
    {
        out.addr = atoi(address_part) / 2;
    }
    else
    {
        out.addr = atoi(address_part);
    }

    return out;
}

static int get_dt_size(const char *datatype)
{
    char dt[20];
    strcpy(dt, datatype);
    to_lowercase(dt);

    if (strcmp(dt, "bool") == 0)
    {
        return 1;
    }
    else if (strcmp(dt, "byte") == 0)
    {
        return 1;
    }
    else if (strcmp(dt, "sint") == 0)
    {
        return 1;
    }
    else if (strcmp(dt, "usint") == 0)
    {
        return 1;
    }
    else if (strcmp(dt, "word") == 0)
    {
        return 1;
    }
    else if (strcmp(dt, "int") == 0)
    {
        return 1;
    }
    else if (strcmp(dt, "uint") == 0)
    {
        return 1;
    }
    else if (strcmp(dt, "dword") == 0)
    {
        return 2;
    }
    else if (strcmp(dt, "dint") == 0)
    {
        return 2;
    }
    else if (strcmp(dt, "udint") == 0)
    {
        return 2;
    }
    else if (strcmp(dt, "real") == 0)
    {
        return 2;
    }
    else if (strcmp(dt, "lword") == 0)
    {
        return 4;
    }
    else if (strcmp(dt, "lint") == 0)
    {
        return 4;
    }
    else if (strcmp(dt, "lreal") == 0)
    {
        return 4;
    }
    else if (strcmp(dt, "string") == 0)
    {
        return 40;
    }
    else if (strncmp(dt, "string", 6) == 0)  // For non standard strings
    {
        int custom_len = atoi(&dt[7]);
        return (custom_len + 1) / 2;
    }
    else
    {
        printf("Wrong data type %s\n", datatype);
        return 0;
    }
}

static void convert_to_string(uint16_t registers[50],const char *datatype, char *value, int max_len)
{
    char dt[20];
    strcpy(dt, datatype);
    to_lowercase(dt);

    if (strcmp(dt, "byte") == 0)
    {
        uint8_t valueIn = (uint8_t)(registers[0] & 0xFF);
        snprintf(value, max_len, "%u", valueIn);
    }
    else if (strcmp(dt, "sint") == 0)
    {
        int8_t valueIn = (int8_t)(registers[0] & 0xFF);
        snprintf(value, max_len, "%d", valueIn);
    }
    else if (strcmp(dt, "usint") == 0)
    {
        uint8_t valueIn = (uint8_t)(registers[0] & 0xFF);
        snprintf(value, max_len, "%u", valueIn);
    }
    else if (strcmp(dt, "word") == 0)
    {
        // Register 1 = High value
        // Register 0 = Low value value
        uint16_t combined = registers[0];

        // Put two variables at same address position
        union 
        {
            uint16_t in;
            uint16_t out;
        } converter;

        // Convert to string
        converter.in = combined;
        snprintf(value, max_len, "%d", converter.out);
    }
    else if (strcmp(dt, "int") == 0)
    {
        // Register 1 = High value
        // Register 0 = Low value value
        uint16_t combined = registers[0];

        // Put two variables at same address position
        union 
        {
            uint16_t in;
            int16_t out;
        } converter;

        // Convert to string
        converter.in = combined;
        snprintf(value, max_len, "%d", converter.out);
    }
    else if (strcmp(dt, "uint") == 0)
    {
        // Register 1 = High value
        // Register 0 = Low value value
        uint16_t combined = registers[0];

        // Put two variables at same address position
        union 
        {
            uint16_t in;
            uint16_t out;
        } converter;

        // Convert to string
        converter.in = combined;
        snprintf(value, max_len, "%u", converter.out);
    }
    else if (strcmp(dt, "dword") == 0)
    {
        // Register 1 = High value
        // Register 0 = Low value value
        uint32_t combined = ((uint32_t)registers[1] << 16) | registers[0];

        // Put two variables at same address position
        union 
        {
            uint32_t in;
            uint32_t out;
        } converter;

        // Convert to string
        converter.in = combined;
        snprintf(value, max_len, "%u", converter.out);
    }
    else if (strcmp(dt, "dint") == 0)
    {
        // Register 1 = High value
        // Register 0 = Low value value
        uint32_t combined = ((uint32_t)registers[1] << 16) | registers[0];

        // Put two variables at same address position
        union 
        {
            uint32_t in;
            int32_t out;
        } converter;

        // Convert to string
        converter.in = combined;
        snprintf(value, max_len, "%d", converter.out);
    }
    else if (strcmp(dt, "udint") == 0)
    {
        // Register 1 = High value
        // Register 0 = Low value value
        uint32_t combined = ((uint32_t)registers[1] << 16) | registers[0];

        // Put two variables at same address position
        union 
        {
            uint32_t in;
            uint32_t out;
        } converter;

        // Convert to string
        converter.in = combined;
        snprintf(value, max_len, "%u", converter.out);
    }
    else if (strcmp(dt, "real") == 0)
    {
        // Register 1 = High value
        // Register 0 = Low value value
        uint32_t combined = ((uint32_t)registers[1] << 16) | registers[0];

        // Put two variables at same address position
        union 
        {
            uint32_t in;
            float out;
        } converter;

        // Convert to string
        converter.in = combined;
        snprintf(value, max_len, "%f", converter.out);
    }
    else if (strcmp(dt, "lword") == 0)
    {
        // Register 1 = High value
        // Register 0 = Low value value
        uint64_t combined = ((uint64_t)registers[3] << 48) | 
                            ((uint64_t)registers[2] << 32) |
                            ((uint64_t)registers[1] << 16) |
                            registers[0];

        // Put two variables at same address position
        union 
        {
            uint64_t in;
            uint64_t out;
        } converter;

        // Convert to string
        converter.in = combined;
        snprintf(value, max_len, "%lu", converter.out);
    }
    else if (strcmp(dt, "lint") == 0)
    {
        // Register 1 = High value
        // Register 0 = Low value value
        uint64_t combined = ((uint64_t)registers[3] << 48) | 
                            ((uint64_t)registers[2] << 32) |
                            ((uint64_t)registers[1] << 16) |
                            registers[0];

        // Put two variables at same address position
        union 
        {
            uint64_t in;
            int64_t out;
        } converter;

        // Convert to string
        converter.in = combined;
        snprintf(value, max_len, "%ld", converter.out);
    }
    else if (strcmp(dt, "lreal") == 0)
    {
        // Register 1 = High value
        // Register 0 = Low value value
        uint64_t combined = ((uint64_t)registers[3] << 48) | 
                            ((uint64_t)registers[2] << 32) |
                            ((uint64_t)registers[1] << 16) |
                            registers[0];

        // Put two variables at same address position
        union 
        {
            uint64_t in;
            double out;
        } converter;

        // Convert to string
        converter.in = combined;
        snprintf(value, max_len, "%f", converter.out);
    }
    else if (strncmp(dt, "string", 6) == 0)
    {
        int nb_registers = get_dt_size(datatype);
        int char_index = 0;

        for (int i = 0; i < nb_registers && char_index < max_len - 1; i++)
        {
            uint16_t reg = registers[i];

            // Get the value for each byte
            uint8_t high_byte = (reg >> 8) & 0xFF;
            uint8_t low_byte = reg & 0xFF;

            // Swap byte, low byte become high byte, and high byte become low byte
            if (char_index < max_len - 1)
            {
                value[char_index++] = (char)low_byte;
            }
            if (char_index < max_len - 1)
            {
                value[char_index++] = (char)high_byte;
            }
        }

        // Add string end char
        value[char_index] = '\0';

        // Remove non used bytes
        while (char_index > 0 && (value[char_index - 1] == '\0' || value[char_index - 1] == ' '))
        {
            char_index--;
        }
        value[char_index] = '\0';
    }
    else
    {
        printf("Wrong data type %s\n", datatype);
    }
}

static void convert_from_string(const char *value,const char *datatype, uint16_t registers[50])
{
    char dt[20];
    strcpy(dt, datatype);
    to_lowercase(dt);

    if (strcmp(dt, "byte") == 0)
    {
        uint8_t val = (uint8_t)atoi(value);
        registers[0] = val;
    }
    else if (strcmp(dt, "sint") == 0)
    {
        int8_t val = (int8_t)atoi(value);
        registers[0] = (uint16_t)(uint8_t)val;
    }
    else if (strcmp(dt, "usint") == 0)
    {
        uint8_t val = (uint8_t)atoi(value);
        registers[0] = val;
    }
    else if (strcmp(dt, "word") == 0)
    {
        registers[0] = (uint16_t)atoi(value);
    }
    else if (strcmp(dt, "int") == 0)
    {
        registers[0] = (int16_t)atoi(value);
    }
    else if (strcmp(dt, "uint") == 0)
    {
        registers[0] = (uint16_t)atoi(value);
    }
    else if (strcmp(dt, "dword") == 0)
    {
        uint32_t val = atoi(value);
        uint32_t out = (uint32_t)val;

        registers[0] = out & 0xFFFF;
        registers[1] = (out >> 16) & 0xFFFF;
    }
    else if (strcmp(dt, "dint") == 0)
    {
        int32_t val = atoi(value);
        int32_t out = (int32_t)val;

        registers[0] = out & 0xFFFF;
        registers[1] = (out >> 16) & 0xFFFF;
    }
    else if (strcmp(dt, "udint") == 0)
    {
        uint32_t val = atoi(value);
        uint32_t out = (uint32_t)val;

        registers[0] = out & 0xFFFF;
        registers[1] = (out >> 16) & 0xFFFF;
    }
    else if (strcmp(dt, "real") == 0)
    {
        float val = atof(value);  // repara: atof, não atoi! float precisa de ponto decimal

        union
        {
            float in;
            uint32_t out;
        } converter;

        converter.in = val;

        registers[0] = converter.out & 0xFFFF;
        registers[1] = (converter.out >> 16) & 0xFFFF;
    }
    else if (strcmp(dt, "lword") == 0)
    {
        uint64_t val = atoll(value);
        uint64_t out = (uint64_t)val;

        registers[0] = out & 0xFFFF;
        registers[1] = (out >> 16) & 0xFFFF;
        registers[2] = (out >> 32) & 0xFFFF;
        registers[3] = (out >> 48) & 0xFFFF;
    }
    else if (strcmp(dt, "lint") == 0)
    {
        int64_t val = atoll(value);
        int64_t out = (int64_t)val;

        registers[0] = out & 0xFFFF;
        registers[1] = (out >> 16) & 0xFFFF;
        registers[2] = (out >> 32) & 0xFFFF;
        registers[3] = (out >> 48) & 0xFFFF;
    }
    else if (strcmp(dt, "lreal") == 0)
    {
        double val = atof(value);

        union
        {
            double in;
            uint64_t out;
        } converter;

        converter.in = val;

        registers[0] = converter.out & 0xFFFF;
        registers[1] = (converter.out >> 16) & 0xFFFF;
        registers[2] = (converter.out >> 32) & 0xFFFF;
        registers[3] = (converter.out >> 48) & 0xFFFF;
    }
    else if (strncmp(dt, "string", 6) == 0)
    {
        int len = strlen(value);
        int max_registers = get_dt_size(datatype);;

        for (int i = 0; i < max_registers; i++)
        {
            uint8_t low_byte = (i * 2 < len) ? (uint8_t)value[i * 2] : 0;
            uint8_t high_byte = (i * 2 + 1 < len) ? (uint8_t)value[i * 2 + 1] : 0;

            registers[i] = ((uint16_t)high_byte << 8) | low_byte;
        }
    }
    else
    {
        printf("Wrong data type %s\n", datatype);
    }
}