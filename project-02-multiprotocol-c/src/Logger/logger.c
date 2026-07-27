#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "generalFunctions/config_reader/config_reader.h"
#include <stdarg.h>

static char loki_host[100] = "";
static int loki_port = 0;
static int initialized = 0;

static const char* get_criticality_string(Criticality c) {
    switch(c) {
        case LOG_INFO: return "Info";
        case LOG_WARNING: return "Warning";
        case LOG_ERROR: return "Error";
        case LOG_CRITICAL: return "Critical";
        default: return "Unknown";
    }
}

static const char* get_class_string(LogClass c) {
    switch(c) {
        case CLASS_GENERAL: return "General";
        case CLASS_REST_API: return "Rest API";
        case CLASS_GPIO: return "GPIO";
        case CLASS_CONNECTION_MANAGER: return "Connection manager";
        case CLASS_MQTT: return "MQTT";
        case CLASS_ETHERNET: return "Ethernet/IP";
        case CLASS_MODBUS: return "Modbus TCP";
        case CLASS_OPC: return "OPC-UA";
        case CLASS_S7: return "S7 Connection";
        default: return "Unknown";
    }
}

// This function adds \ before some "
static void escape_json_string(const char *input, char *output, int max_len) 
{
    int j = 0;
    for (int i = 0; input[i] != '\0' && j < max_len - 3; i++) 
    {
        if (input[i] == '"' || input[i] == '\\') 
        {
            output[j++] = '\\';
        }
        output[j++] = input[i];
    }
    output[j] = '\0';
}

// Initialize standard parameters
static void logger_init(void) 
{
    if (initialized) return;

    char portStr[10];
    get_config_value("Logger/logConf.txt", "host", loki_host, sizeof(loki_host));
    get_config_value("Logger/logConf.txt", "port", portStr, sizeof(portStr));
    
    if (strlen(portStr) > 0) 
    {
        loki_port = atoi(portStr);
    }
    initialized = 1;
}

// Send payload to socket
static void send_to_loki(const char *payload) 
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    // Two seconds timeout to not break the main task
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(loki_port);

    // Resolve hostname (e.g.: localhost) or IP
    struct hostent *he = gethostbyname(loki_host);
    if (he == NULL) 
    {
        close(sock);
        return;
    }
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    // Try to connect, if fail return null
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
    {
        close(sock);
        return;
    }

    // Assembly HTTP header
    char http_request[4096];
    snprintf(http_request, sizeof(http_request),
             "POST /loki/api/v1/push HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s", 
             loki_host, 
             loki_port, 
             strlen(payload), 
             payload);

    // Send payload
    if (send(sock, http_request, strlen(http_request), 0) < 0) {
        close(sock);
        return;
    }
    char dummy_buffer[512];
    // This recv make this function wait untill complete to send the message
    // Without that, some messages are not sent
    recv(sock, dummy_buffer, sizeof(dummy_buffer) - 1, 0); 
    close(sock);
}

void logger_log(LogClass className, Criticality criticality, const char *format, ...) 
{
    const char *crit_str = get_criticality_string(criticality);
    const char *class_str = get_class_string(className);

    // Assembly the format into description
    char description[512];
    va_list args;
    va_start(args, format);
    vsnprintf(description, sizeof(description), format, args);
    va_end(args);
    
    // Print in console
    char crit_upper[20];
    strncpy(crit_upper, crit_str, sizeof(crit_upper));
    for (int i = 0; crit_upper[i]; i++) 
    {
        crit_upper[i] = toupper((unsigned char)crit_upper[i]);
    }
    printf("[%s] [%s] %s\n", crit_upper, class_str, description);

    // Start connection with loki
    if (!initialized) 
    {
        logger_init();
    }
    if (strlen(loki_host) == 0 || loki_port == 0) 
    {
        return;
    }

    // Get current timestamp in ns
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned long long timestamp_ns = (unsigned long long)ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    char escaped_desc[1024];
    escape_json_string(description, escaped_desc, sizeof(escaped_desc));


    // Assembly json
    char payload[2048];
    snprintf(payload, sizeof(payload),
             "{"
             "\"streams\": ["
             "  {"
             "    \"stream\": {"
             "      \"class\": \"%s\","
             "      \"criticality\": \"%s\""
             "    },"
             "    \"values\": ["
             "      [\"%llu\", \"%s\"]"
             "    ]"
             "  }"
             "]"
             "}", 
             class_str, 
             crit_str, 
             timestamp_ns, 
             escaped_desc);

    send_to_loki(payload);
}