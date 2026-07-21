#ifndef S7_CLIENT_H
#define S7_CLIENT_H

#include <snap7.h>

typedef struct {
    char ip[100];
    int rack;
    int slot;
    int connected;
    S7Object client;
} S7ClientWrapper;

typedef struct {
    int address;
    int bit;
} S7_Address_bit;

void s7_client_init(S7ClientWrapper *wrapper, const char *ip, int rack, int slot);
int s7_client_connect(S7ClientWrapper *wrapper);
void s7_client_disconnect(S7ClientWrapper *wrapper);
int s7_client_read(S7ClientWrapper *wrapper,int db_number,const char *offset,const char *datatype, char *value, int max_len);
int s7_client_write(S7ClientWrapper *wrapper,int db_number,const char *offset,const char *datatype,const char *value);

#endif