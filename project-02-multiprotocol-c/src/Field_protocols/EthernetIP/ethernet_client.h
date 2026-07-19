#ifndef ETHERNETIP_CLIENT_H
#define ETHERNETIP_CLIENT_H

#include <libplctag.h>

typedef struct {
    char ip[100];
    int connected;
} EthernetIPClientWrapper;

void ethernetip_client_init(EthernetIPClientWrapper *wrapper, const char *ip);
int ethernetip_client_read(EthernetIPClientWrapper *wrapper, const char *tag_name, const char *datatype, char *value, int max_len);
int ethernetip_client_write(EthernetIPClientWrapper *wrapper, const char *tag_name, const char *datatype, const char *value);
#endif