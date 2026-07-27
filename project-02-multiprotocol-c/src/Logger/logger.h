#ifndef LOGGER_H
#define LOGGER_H

// Equivalente à classe enum Criticality
typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL
} Criticality;

// Equivalente à classe enum Class
typedef enum {
    CLASS_GENERAL,
    CLASS_REST_API,
    CLASS_GPIO,
    CLASS_CONNECTION_MANAGER,
    CLASS_MQTT,
    CLASS_ETHERNET,
    CLASS_MODBUS,
    CLASS_OPC,
    CLASS_S7
} LogClass;

void logger_log(LogClass className, Criticality criticality, const char *format, ...);

#endif