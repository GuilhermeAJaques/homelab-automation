#ifndef OPC_CLIENT_H
#define OPC_CLIENT_H

#include <open62541.h>

typedef struct {
    char url[100];
    int connected;
    UA_Client *client;
} OPCClientWrapper;

void opc_client_init(OPCClientWrapper *wrapper, const char *url);
int opc_client_connect(OPCClientWrapper *wrapper);
void opc_client_disconnect(OPCClientWrapper *wrapper);
int opc_client_read(OPCClientWrapper *wrapper, char *variableName, char *value, int max_len);
int opc_client_write(OPCClientWrapper *wrapper, char *variableName,const char *value);

#endif