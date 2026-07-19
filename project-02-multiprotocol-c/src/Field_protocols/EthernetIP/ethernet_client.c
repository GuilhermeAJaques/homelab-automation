#include "ethernet_client.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../generalFunctions/string_functions/string_functions.h"

// Declare methods used
static int get_dt_size(const char *datatype);
static void convert_to_string(int32_t tag, const char *datatype,char *out_str, int max_len);
static void convert_from_string(int32_t tag,const char *datatype, const char *value);

void ethernetip_client_init(EthernetIPClientWrapper *wrapper, const char *ip)
{
    strcpy(wrapper->ip, ip);
    wrapper->connected = 0;
}

int ethernetip_client_read(EthernetIPClientWrapper *wrapper, const char *tag_name, const char *datatype, char *value, int max_len)
{
    // Get datatype size
    int elem_size = get_dt_size(datatype);

    // Build connection string
    char attr_str[300];
    snprintf(attr_str, sizeof(attr_str),
            "protocol=ab_eip&gateway=%s&path=1,0&cpu=LGX&elem_size=%d&elem_count=1&name=%s",
            wrapper->ip, elem_size, tag_name);

    int32_t tag = plc_tag_create(attr_str, 5000);

    if (tag < 0)
    {
        printf("Error creating tag %s: %s\n", tag_name, plc_tag_decode_error(tag));
        return 0;
    }

    int rc = plc_tag_read(tag, 5000);

    if (rc == PLCTAG_STATUS_OK)
    {
        convert_to_string(tag, datatype, value, max_len);
        plc_tag_destroy(tag);
        return 1;
    }
    else
    {
        printf("Error to read variable %s from the PLC %s: %i\n", tag_name, wrapper->ip, rc);
        plc_tag_destroy(tag);
        return 0;
    }
    return 0;
}

int ethernetip_client_write(EthernetIPClientWrapper *wrapper, const char *tag_name, const char *datatype, const char *value)
{
    // Get datatype size
    int elem_size = get_dt_size(datatype);

    // Build connection string
    char attr_str[300];
    snprintf(attr_str, sizeof(attr_str),
            "protocol=ab_eip&gateway=%s&path=1,0&cpu=LGX&elem_size=%d&elem_count=1&name=%s",
            wrapper->ip, elem_size, tag_name);

    int32_t tag = plc_tag_create(attr_str, 5000);

    if (tag < 0)
    {
        printf("Error creating tag %s: %s\n", tag_name, plc_tag_decode_error(tag));
        return 0;
    }

    // Convert value from string to final datatype
    convert_from_string(tag, datatype, value);

    // Write value on PLC
    int rc = plc_tag_write(tag, 5000);

    if (rc != PLCTAG_STATUS_OK)
    {
        printf("Error writing variable %s to PLC %s: %d\n", tag_name, wrapper->ip, rc);
        plc_tag_destroy(tag);
        return 0;
    }

    plc_tag_destroy(tag);
    return 1;
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
    else if (strcmp(dt, "sint") == 0)
    {
        return 1;
    }
    else if (strcmp(dt, "usint") == 0)
    {
        return 1;
    }
    else if (strcmp(dt, "int") == 0)
    {
        return 2;
    }
    else if (strcmp(dt, "uint") == 0)
    {
        return 2;
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
    else if (strcmp(dt, "lint") == 0)
    {
        return 8;
    }
    else if (strcmp(dt, "lreal") == 0)
    {
        return 8;
    }
    else if (strcmp(dt, "string") == 0)
    {
        return 88;
    }
    else
    {
        printf("Wrong data type %s\n", datatype);
        return 0;
    }
}

static void convert_to_string(int32_t tag,const char *datatype, char *out_str, int max_len)
{    
    // Get datatype
    char dt[20];
    strcpy(dt, datatype);
    to_lowercase(dt); // Used to be not case sensitive

    // Get variable parameters
    if (strcmp(dt, "bool") == 0)
    {
        uint8_t value = plc_tag_get_uint8(tag, 0);
        snprintf(out_str, max_len, value ? "true" : "false");
    }
    else if (strcmp(dt, "sint") == 0)
    {
        int8_t value = plc_tag_get_int8(tag, 0);
        snprintf(out_str, max_len, "%d", value);
    }
    else if (strcmp(dt, "usint") == 0)
    {
        uint8_t value = plc_tag_get_uint8(tag, 0);
        snprintf(out_str, max_len, "%d", value);
    }
    else if (strcmp(dt, "int") == 0)
    {
        int16_t value = plc_tag_get_int16(tag, 0);
        snprintf(out_str, max_len, "%d", value);
    }
    else if (strcmp(dt, "uint") == 0)
    {
        uint16_t value = plc_tag_get_int16(tag, 0);
        snprintf(out_str, max_len, "%d", value);
    }
    else if (strcmp(dt, "dint") == 0)
    {
        int32_t value = plc_tag_get_int32(tag, 0);
        snprintf(out_str, max_len, "%d", value);
    }
    else if (strcmp(dt, "udint") == 0)
    {
        uint32_t value = plc_tag_get_uint32(tag, 0);
        snprintf(out_str, max_len, "%d", value);
    }
    else if (strcmp(dt, "real") == 0)
    {
        float value = plc_tag_get_float32(tag, 0);
        snprintf(out_str, max_len, "%f", value);
    }
    else if (strcmp(dt, "lint") == 0)
    {
        int64_t value = plc_tag_get_int64(tag, 0);
        snprintf(out_str, max_len, "%ld", value);
    }
    else if (strcmp(dt, "lreal") == 0)
    {
        double value = plc_tag_get_float64(tag, 0);
        snprintf(out_str, max_len, "%f", value);
    }
    else if (strcmp(dt, "string") == 0)
    {
        // The first 4 bytes from string variable carry the size of the variable
        int32_t str_len = plc_tag_get_int32(tag, 0);
        if (str_len > max_len - 1) 
        {
            str_len = max_len - 1;
        }
        for (int i = 0; i < str_len; i++) 
        {
            // Build string value
            // 4 + i = It's for the first 4 bytes that carry the string size
            out_str[i] = (char)plc_tag_get_uint8(tag, 4 + i);
        }
        out_str[str_len] = '\0'; // Add the string terminator
    }
    else
    {
        printf("Wrong data type %s\n", datatype);
    }
}

static void convert_from_string(int32_t tag,const char *datatype, const char *value)
{    
    // Get datatype
    char dt[20];
    strcpy(dt, datatype);
    to_lowercase(dt); // Used to be not case sensitive

    // Get variable parameters
    if (strcmp(dt, "bool") == 0)
    {
        char val_lower[10];
        strcpy(val_lower, value);
        to_lowercase(val_lower);

        int bool_value = 0;

        if (strcmp(val_lower, "true") == 0 || strcmp(val_lower, "1") == 0)
        {
            bool_value = 1;
        }
        else if (strcmp(val_lower, "false") == 0 || strcmp(val_lower, "0") == 0)
        {
            bool_value = 0;
        }
        else
        {
            printf("Invalid boolean value: %s (expected true/false/1/0)\n", value);
        }

        plc_tag_set_uint8(tag, 0, bool_value);
    }
    else if (strcmp(dt, "sint") == 0)
    {
        plc_tag_set_int8(tag, 0, atoi(value));
    }
    else if (strcmp(dt, "usint") == 0)
    {
        plc_tag_set_uint8(tag, 0, atoi(value));
    }
    else if (strcmp(dt, "int") == 0)
    {
        plc_tag_set_int16(tag, 0, atoi(value));
    }
    else if (strcmp(dt, "uint") == 0)
    {
        plc_tag_set_int16(tag, 0, atoi(value));
    }
    else if (strcmp(dt, "dint") == 0)
    {
        plc_tag_set_int32(tag, 0, atoi(value));
    }
    else if (strcmp(dt, "udint") == 0)
    {
        plc_tag_set_uint32(tag, 0, atoi(value));
    }
    else if (strcmp(dt, "real") == 0)
    {
        plc_tag_set_float32(tag, 0, atof(value));
    }
    else if (strcmp(dt, "lint") == 0)
    {
        plc_tag_set_int64(tag, 0, atol(value));
    }
    else if (strcmp(dt, "lreal") == 0)
    {
        plc_tag_set_float64(tag, 0, atof(value));
    }
    else if (strcmp(dt, "string") == 0)
    {
        // The first 4 bytes from string variable carry the size of the variable
        int str_len = strlen(value);
        plc_tag_set_int32(tag, 0, str_len);
        for (int i = 0; i < str_len; i++) 
        {
            // Build string value
            // 4 + i = It's for the first 4 bytes that carry the string size
            plc_tag_set_uint8(tag, 4 + i, (uint8_t)value[i]);
        }
    }
    else
    {
        printf("Wrong data type %s\n", datatype);
    }
}