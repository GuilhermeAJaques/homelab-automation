#include "s7_client.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../generalFunctions/string_functions/string_functions.h"

static int get_dt_size(const char *datatype);
static S7_Address_bit get_address_bit(const char *fullAddress);
static void convert_to_string(uint8_t buffer[256], const char *datatype, char *value, int max_len);
static void convert_from_string(const char *value, const char *datatype, uint8_t buffer[256]);
static int is_connected(S7ClientWrapper *wrapper);

void s7_client_init(S7ClientWrapper *wrapper, const char *ip, int rack, int slot)
{
    wrapper->client = Cli_Create();
    strcpy(wrapper->ip, ip);
    wrapper->rack = rack;
    wrapper->slot = slot;
    wrapper->connected = 0;
}

int s7_client_connect(S7ClientWrapper *wrapper)
{
    int rc = Cli_ConnectTo(wrapper->client, wrapper->ip, wrapper->rack, wrapper->slot);

    if (rc == 0)
    {
        wrapper->connected = 1;
        printf("Connected to PLC %s by S7 connection\n", wrapper->ip);
        return rc;
    }
    else
    {
        char error_text[256];
        Cli_ErrorText(rc, error_text, sizeof(error_text));
        printf("Fail to connect to PLC %s by S7 connection: %s\n", wrapper->ip, error_text);
        return rc;
    }
}

void s7_client_disconnect(S7ClientWrapper *wrapper)
{
    int rc = Cli_Disconnect(wrapper->client);

    if (rc == 0)
    {
        wrapper->connected = 0;
        Cli_Destroy(&wrapper->client);
        printf("Disconnected to PLC %s by S7 connection\n", wrapper->ip);
    }
    else
    {
        char error_text[256];
        Cli_ErrorText(rc, error_text, sizeof(error_text));
        printf("Fail to disconect to PLC %s by S7 connection: %s\n", wrapper->ip, error_text);
    }
}

int s7_client_read(S7ClientWrapper *wrapper,int db_number,const char *offset,const char *datatype, char *value, int max_len)
{
    if (!is_connected(wrapper))
    {
        printf("Connection lost, attempting to reconnect...\n");
        wrapper->connected = 0;
        Cli_Destroy(&wrapper->client);
        wrapper->client = Cli_Create();
        s7_client_connect(wrapper);
    }

    S7_Address_bit addr_bit = get_address_bit(offset);
    uint8_t buffer[256];
    int rc = Cli_DBRead(wrapper->client, db_number, addr_bit.address, get_dt_size(datatype), buffer);
    char dt[20];
    strcpy(dt, datatype);
    to_lowercase(dt);

    if (rc != 0)
    {
        char error_text[256];
        Cli_ErrorText(rc, error_text, sizeof(error_text));
        printf("Fail to read on offset %s to PLC %s by S7 connection: %s\n", offset, wrapper->ip, error_text);
        return rc;
    }

    if (strcmp(dt, "bool") == 0)
    {
        int bool_value = (buffer[0] >> addr_bit.bit) & 0x01;
        snprintf(value, max_len, bool_value ? "true" : "false");
    }
    else
    {
        convert_to_string(buffer, datatype, value, max_len);
    }
    return rc;
}

int s7_client_write(S7ClientWrapper *wrapper,int db_number,const char *offset,const char *datatype,const char *value)
{
    if (!is_connected(wrapper))
    {
        printf("Connection lost, attempting to reconnect...\n");
        wrapper->connected = 0;
        Cli_Destroy(&wrapper->client);
        wrapper->client = Cli_Create();
        s7_client_connect(wrapper);
    }

    S7_Address_bit addr_bit = get_address_bit(offset);
    uint8_t buffer[256];

    convert_from_string(value, datatype, buffer);
    
    char dt[20];
    strcpy(dt, datatype);
    to_lowercase(dt);
    int rc;
    if (strcmp(dt, "bool") == 0)
    {
        // Read first, to just modify the bit inside the byte
        rc = Cli_DBRead(wrapper->client, db_number, addr_bit.address, 1, buffer);
        if (rc != 0)
        {
            printf("Error reading byte before writing bool at offset %s\n", offset);
            return rc;
        }

        int bool_value = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) ? 1 : 0;

        if (bool_value)
        {
            // Number 1 create a byte in binary, like 0 0 0 0 0 0 0 1.

            // << shift for left, move the bit number to the binary create,
            // e.g.: addr_bit.bit = 3, shift left 3 times: 0 0 0 0 1 0 0 0.

            // |= "or" move to buffer byte, copare buffer bit with binary create bit
            // e.g.: buffer[0] = 0 0 1 1 0 0 0 1
            //   binary create = 0 0 0 0 1 0 0 0
            // Afeter execute 
            // "or" logic      = 0 0 1 1 1 0 0 1

            // Result: Set to true the desired bit
                
            buffer[0] |= (1 << addr_bit.bit);
        }
        else
        {
            // Number 1 create a byte in binary, like 0 0 0 0 0 0 0 1.

            // << shift for left, move the bit number to the binary create,
            // e.g.: addr_bit.bit = 3, shift left 3 times: 0 0 0 0 1 0 0 0.

            // ~ not, invert all bits e.g.: 1 1 1 1 0 1 1 1

            // &= "and" move to buffer byte, copare buffer bit with binary create bit
            // e.g.: buffer[0] = 0 0 1 1 1 0 0 1
            //   binary create = 1 1 1 1 0 1 1 1
            // Afeter execute 
            // "and" logic     = 0 0 1 1 0 0 0 1

            // Result: Set to false the desired bit
                
            buffer[0] &= ~(1 << addr_bit.bit);
        }

        rc = Cli_DBWrite(wrapper->client, db_number, addr_bit.address, 1, buffer);
    }
    else
    {
        rc = Cli_DBWrite(wrapper->client, db_number, addr_bit.address, get_dt_size(datatype), buffer);
    }
    
    if (rc != 0)
    {
        char error_text[256];
        Cli_ErrorText(rc, error_text, sizeof(error_text));
        printf("Fail to read on offset %s to PLC %s by S7 connection: %s\n", offset, wrapper->ip, error_text);
    }
    return rc;
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
        return 2;
    }
    else if (strcmp(dt, "int") == 0)
    {
        return 2;
    }
    else if (strcmp(dt, "uint") == 0)
    {
        return 2;
    }
    else if (strcmp(dt, "dword") == 0)
    {
        return 4;
    }
    else if (strcmp(dt, "dint") == 0)
    {
        return 4;
    }
    else if (strcmp(dt, "udint") == 0)
    {
        return 4;
    }
    else if (strcmp(dt, "real") == 0)
    {
        return 4;
    }
    else if (strcmp(dt, "lreal") == 0)
    {
        return 8;
    }
    else if (strcmp(dt, "string") == 0)
    {
        return 256;
    }
    else if (strncmp(dt, "string", 6) == 0)  // For non standard strings
    {
        return atoi(&dt[7]) + 2;
    }
    else
    {
        printf("Wrong data type %s\n", datatype);
        return 0;
    }
}

static S7_Address_bit get_address_bit(const char *fullAddress)
{
    const char *dot_pos = strchr(fullAddress, '.');
    S7_Address_bit out;
    out.address = 0;
    out.bit = 0;
    if (dot_pos != NULL)
    {
        int byte_len = dot_pos - fullAddress;
        char byte_str[10];
        strncpy(byte_str, fullAddress, byte_len);
        byte_str[byte_len] = '\0';

        out.address = atoi(byte_str);
        out.bit = atoi(dot_pos + 1);
    }
    else
    {
        out.address = atoi(fullAddress);
        out.bit = 0;
    }
    return out;
}

static void convert_to_string(uint8_t buffer[256], const char *datatype, char *value, int max_len)
{
    char dt[20];
    strcpy(dt, datatype);
    to_lowercase(dt);

    if (strcmp(dt, "byte") == 0)
    {
        snprintf(value, max_len, "%u", buffer[0]);
    }
    else if (strcmp(dt, "sint") == 0)
    {
        snprintf(value, max_len, "%d", (int8_t)buffer[0]);
    }
    else if (strcmp(dt, "usint") == 0)
    {
        snprintf(value, max_len, "%u", buffer[0]);
    }
    else if (strcmp(dt, "word") == 0)
    {
        uint16_t val = ((uint16_t)buffer[0] << 8) | buffer[1];
        snprintf(value, max_len, "%u", val);
    }
    else if (strcmp(dt, "int") == 0)
    {
        int16_t val = ((int16_t)buffer[0] << 8) | buffer[1];
        snprintf(value, max_len, "%d", val);
    }
    else if (strcmp(dt, "uint") == 0)
    {
        uint16_t val = ((uint16_t)buffer[0] << 8) | buffer[1];
        snprintf(value, max_len, "%u", val);
    }
    else if (strcmp(dt, "dword") == 0)
    {
        uint32_t val = ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) |
                       ((uint32_t)buffer[2] << 8) | buffer[3];
        snprintf(value, max_len, "%u", val);
    }
    else if (strcmp(dt, "dint") == 0)
    {
        int32_t val = ((int32_t)buffer[0] << 24) | ((int32_t)buffer[1] << 16) |
                      ((int32_t)buffer[2] << 8) | buffer[3];
        snprintf(value, max_len, "%d", val);
    }
    else if (strcmp(dt, "udint") == 0)
    {
        uint32_t val = ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) |
                       ((uint32_t)buffer[2] << 8) | buffer[3];
        snprintf(value, max_len, "%u", val);
    }
    else if (strcmp(dt, "real") == 0)
    {
        uint32_t combined = ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) |
                            ((uint32_t)buffer[2] << 8) | buffer[3];
        union { uint32_t in; float out; } converter;
        converter.in = combined;
        snprintf(value, max_len, "%f", converter.out);
    }
    else if (strcmp(dt, "lreal") == 0)
    {
        uint64_t combined = 0;
        for (int i = 0; i < 8; i++)
        {
            combined = (combined << 8) | buffer[i];
        }
        union { uint64_t in; double out; } converter;
        converter.in = combined;
        snprintf(value, max_len, "%f", converter.out);
    }
    else if (strncmp(dt, "string", 6) == 0)
    {
        int actual_len = buffer[1];
        if (actual_len > max_len - 1) actual_len = max_len - 1;
        memcpy(value, &buffer[2], actual_len);
        value[actual_len] = '\0';
    }
    else
    {
        printf("Wrong data type %s\n", datatype);
    }
}

static void convert_from_string(const char *value, const char *datatype, uint8_t buffer[256])
{
    char dt[20];
    strcpy(dt, datatype);
    to_lowercase(dt);

    if (strcmp(dt, "byte") == 0)
    {
        buffer[0] = (uint8_t)atoi(value);
    }
    else if (strcmp(dt, "usint") == 0)
    {
        buffer[0] = (uint8_t)atoi(value);
    }
    else if (strcmp(dt, "sint") == 0)
    {
        buffer[0] = (uint8_t)(int8_t)atoi(value);
    }
    else if (strcmp(dt, "word") == 0)
    {
        uint16_t val = (uint16_t)atoi(value);
        buffer[0] = (val >> 8) & 0xFF;
        buffer[1] = val & 0xFF;
    }
    else if (strcmp(dt, "uint") == 0)
    {
        uint16_t val = (uint16_t)atoi(value);
        buffer[0] = (val >> 8) & 0xFF;
        buffer[1] = val & 0xFF;
    }
    else if (strcmp(dt, "int") == 0)
    {
        int16_t val = (int16_t)atoi(value);
        buffer[0] = (val >> 8) & 0xFF;
        buffer[1] = val & 0xFF;
    }
    else if (strcmp(dt, "dword") == 0)
    {
        uint32_t val = (uint32_t)atoi(value);
        buffer[0] = (val >> 24) & 0xFF;
        buffer[1] = (val >> 16) & 0xFF;
        buffer[2] = (val >> 8) & 0xFF;
        buffer[3] = val & 0xFF;
    }
    else if (strcmp(dt, "udint") == 0)
    {
        uint32_t val = (uint32_t)atoi(value);
        buffer[0] = (val >> 24) & 0xFF;
        buffer[1] = (val >> 16) & 0xFF;
        buffer[2] = (val >> 8) & 0xFF;
        buffer[3] = val & 0xFF;
    }
    else if (strcmp(dt, "dint") == 0)
    {
        int32_t val = (int32_t)atoi(value);
        buffer[0] = (val >> 24) & 0xFF;
        buffer[1] = (val >> 16) & 0xFF;
        buffer[2] = (val >> 8) & 0xFF;
        buffer[3] = val & 0xFF;
    }
    else if (strcmp(dt, "real") == 0)
    {
        float val = atof(value);
        union { float in; uint32_t out; } converter;
        converter.in = val;
        buffer[0] = (converter.out >> 24) & 0xFF;
        buffer[1] = (converter.out >> 16) & 0xFF;
        buffer[2] = (converter.out >> 8) & 0xFF;
        buffer[3] = converter.out & 0xFF;
    }
    else if (strcmp(dt, "lreal") == 0)
    {
        double val = atof(value);
        union { double in; uint64_t out; } converter;
        converter.in = val;
        for (int i = 0; i < 8; i++)
        {
            buffer[i] = (converter.out >> (56 - i * 8)) & 0xFF;
        }
    }
    else if (strncmp(dt, "string", 6) == 0)
    {
        int max_declared = get_dt_size(datatype) - 2;  // tira os 2 bytes de cabeçalho
        int len = strlen(value);
        if (len > max_declared) len = max_declared;

        buffer[0] = (uint8_t)max_declared;   // tamanho máximo
        buffer[1] = (uint8_t)len;             // tamanho atual
        memcpy(&buffer[2], value, len);
    }
    else
    {
        printf("Wrong data type %s\n", datatype);
    }
}

static int is_connected(S7ClientWrapper *wrapper)
{
    int connected = 0;
    Cli_GetConnected(wrapper->client, &connected);
    return connected;
}