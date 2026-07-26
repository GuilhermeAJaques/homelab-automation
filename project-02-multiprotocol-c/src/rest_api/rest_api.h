#ifndef REST_API_H
#define REST_API_H

#include "Connection_Manager/connection_manager.h"
#include <microhttpd.h>
#include <pthread.h>

typedef struct {
    struct MHD_Daemon *daemon;
    ConnectionManager *manager;
    pthread_mutex_t *connection_mutex;
} RestApiWrapper;

int rest_api_start(RestApiWrapper *wrapper, ConnectionManager *manager, pthread_mutex_t *mutex, int port);
void rest_api_stop(RestApiWrapper *wrapper);

#endif