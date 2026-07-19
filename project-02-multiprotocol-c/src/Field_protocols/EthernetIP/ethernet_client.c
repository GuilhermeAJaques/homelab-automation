#include "ethernet_client.h"
#include <string.h>
#include <stdio.h>
#include "../../generalFunctions/string_functions/string_functions.h"

// Declare methods used
static int get_dt_size(const char *datatype);
static void convert_to_string(int32_t tag, const char *datatype,char *out_str, int max_len);

void ethernetip_client_init(EthernetIPClientWrapper *wrapper, const char *ip)
{
    strcpy(wrapper->ip, ip);
    wrapper->connected = 0;
}

int ethernetip_client_read(EthernetIPClientWrapper *wrapper, const char *tag_name, const char *datatype, char *value, int max_len)
{
    // Get datatype size
    int elem_size = get_dt_size((char *)datatype);

    // Build connection string
    char attr_str[300];
    snprintf(attr_str, sizeof(attr_str),
            "protocol=ab_eip&gateway=%s&path=1,0&cpu=LGX&elem_size=%d&elem_count=1&name=%s",
            wrapper->ip, elem_size, tag_name);

    int32_t tag = plc_tag_create(attr_str, 5000);
    int rc = plc_tag_read(tag, 5000);

    if (rc == PLCTAG_STATUS_OK)
    {
        convert_to_string(tag, (char *)datatype, value, max_len);
        plc_tag_destroy(tag);
        return 1;
    }
    else
    {
        printf("Error to read variable %s from the PLC %s: %i\n", tag_name, wrapper->ip, rc);
        return 0;
    }
    return 0;
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
    else if (strncmp(dt, "string", 6) == 0) // For string with size defined e.g.: STRING[10]
    {
        int custom_len = atoi(&dt[7]);
        return custom_len + 4;
    }
    else
    {
        printf("Wrong data type %s\n", datatype);
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
        snprintf(out_str, max_len, "%d", value);
    }
    else if (strcmp(dt, "lreal") == 0)
    {
        double value = plc_tag_get_float64(tag, 0);
        snprintf(out_str, max_len, "%f", value);
    }
    else if (strcmp(dt, "string") == 0)
    {
        
    }
    else if (strncmp(dt, "string", 6) == 0) // For string with size defined e.g.: STRING[10]
    {
        
    }
    else
    {
        printf("Wrong data type %s\n", datatype);
    }
}