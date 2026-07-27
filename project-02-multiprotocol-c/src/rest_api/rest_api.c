#include "rest_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "Logger/logger.h"

typedef struct {
    char data[500];
    size_t size;
} PostData;

static int is_numeric_or_bool(const char *value)
{
    if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0)
    {
        return 1;
    }

    int hasDigit = 0;
    for (int i = 0; value[i] != '\0'; i++)
    {
        if (isdigit((unsigned char)value[i]))
        {
            hasDigit = 1;
        }
        else if (value[i] != '.' && value[i] != '-' && value[i] != '+')
        {
            return 0;
        }
    }
    return hasDigit;
}

static void get_iso_timestamp(char *buffer, int max_len)
{
    time_t now = time(NULL);
    struct tm *utc = gmtime(&now);
    strftime(buffer, max_len, "%Y-%m-%dT%H:%M:%SZ", utc);
}

static enum MHD_Result send_response(struct MHD_Connection *connection, unsigned int status_code, const char *json)
{
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(json), 
                                                                    (void *)json,
                                                                    MHD_RESPMEM_MUST_COPY);

    MHD_add_response_header(response, "Content-Type", "application/json");
    enum MHD_Result ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

static void append_variable_json(char *json, VariableValues *var, int isFirst)
{
    char item[500];
    char timestamp[30];
    get_iso_timestamp(timestamp, sizeof(timestamp));

    if (is_numeric_or_bool(var->value))
    {
        snprintf(item, sizeof(item),
                 "%s{\"name\":\"%s\",\"value\":%s,\"topic\":\"%s\",\"timestamp\":\"%s\"}",
                 isFirst ? "" : ",", var->name, var->value, var->topic, timestamp);
    }
    else
    {
        snprintf(item, sizeof(item),
                 "%s{\"name\":\"%s\",\"value\":\"%s\",\"topic\":\"%s\",\"timestamp\":\"%s\"}",
                 isFirst ? "" : ",", var->name, var->value, var->topic, timestamp);
    }

    strcat(json, item);
}

static enum MHD_Result handle_get_variables(RestApiWrapper *wrapper, struct MHD_Connection *connection)
{
    pthread_mutex_lock(wrapper->connection_mutex);
    connection_manager_read_all(wrapper->manager);

    char json[4096];
    strcpy(json, "[");

    for (int i = 0; i < wrapper->manager->totalVariables; i++)
    {
        append_variable_json(json, &wrapper->manager->returnValues[i], i == 0);
    }
    strcat(json, "]");

    pthread_mutex_unlock(wrapper->connection_mutex);

    return send_response(connection, 200, json);
}

static enum MHD_Result handle_get_variable(RestApiWrapper *wrapper, struct MHD_Connection *connection, const char *topic)
{
    VariableValues value;

    pthread_mutex_lock(wrapper->connection_mutex);
    connection_manager_read(wrapper->manager, (char *)topic, &value);
    pthread_mutex_unlock(wrapper->connection_mutex);

    if (strlen(value.value) == 0)
    {
        return send_response(connection, 404, "{\"error\":\"Variable not found\"}");
    }

    char json[500] = "";
    append_variable_json(json, &value, 1);

    return send_response(connection, 200, json);
}

static int extract_json_string_field(const char *json, const char *field, char *out, int max_len)
{
    char search[50];
    snprintf(search, sizeof(search), "\"%s\"", field);

    char *pos = strstr(json, search);
    if (pos == NULL) 
    {
        return 0;
    }
    pos += strlen(search);

    while (*pos == ' ' || *pos == ':') 
    {
        pos++;
    }

    if (*pos != '"') 
    {
        return 0;
    }
    pos++;

    char *end = strchr(pos, '"');
    if (end == NULL) 
    {
        return 0;
    }

    int len = (int)(end - pos);
    if (len >= max_len) 
    {
        len = max_len - 1;
    }

    strncpy(out, pos, len);
    out[len] = '\0';
    return 1;
}

static enum MHD_Result handle_write(RestApiWrapper *wrapper, struct MHD_Connection *connection, const char *body)
{
    char topic[100];
    char value[100];

    // Get topic from json
    int result = extract_json_string_field(body, "topic", topic, sizeof(topic));
    if (!result)
    {
        return send_response(connection, 400, "{\"error\":\"Missing topic\"}");
    }

    // Get value from json
    result = extract_json_string_field(body, "value", value, sizeof(value));
    if (!result)
    {
        return send_response(connection, 400, "{\"error\":\"Missing value\"}");
    }

    // Write to the variable
    pthread_mutex_lock(wrapper->connection_mutex);
    connection_manager_write(wrapper->manager, topic, value);
    pthread_mutex_unlock(wrapper->connection_mutex);

    return send_response(connection, 200, "{\"status\":\"ok\"}");
}

static enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
                                      const char *url,
                                      const char *method,
                                      const char *version,
                                      const char *upload_data,
                                      size_t *upload_data_size,
                                      void **req_cls)
{
    RestApiWrapper *wrapper = (RestApiWrapper *)cls;

    if (*req_cls == NULL)
    {
        PostData *post_data = malloc(sizeof(PostData));
        post_data->size = 0;
        post_data->data[0] = '\0';
        *req_cls = post_data;
        return MHD_YES;
    }

    PostData *post_data = (PostData *)*req_cls;

    // Write variable
    if (strcmp(method, MHD_HTTP_METHOD_POST) == 0)
    {
        // Body is send in some requests
        // Until receive upload_data_size == 0, needs to build the entire body
        // When receibe upload_data_size == 0, means that the body ends
        if (*upload_data_size != 0)
        {
            size_t copy_len = *upload_data_size;
            // Overflow protection
            if (post_data->size + copy_len >= sizeof(post_data->data))
            {
                copy_len = sizeof(post_data->data) - post_data->size - 1;
            }
            // Move new data after the last position from last request
            memcpy(post_data->data + post_data->size, upload_data, copy_len);
            post_data->size += copy_len;
            post_data->data[post_data->size] = '\0';
            *upload_data_size = 0;
            return MHD_YES;
        }

        // After complete to build body call write method
        enum MHD_Result ret = handle_write(wrapper, connection, post_data->data);
        // Need to release the post memory
        free(post_data);
        *req_cls = NULL;
        return ret;
    }
    else if (strcmp(method, MHD_HTTP_METHOD_GET) == 0)
    {
        enum MHD_Result ret;

        // Read all variables
        if (strcmp(url, "/variables") == 0)
        {
            ret = handle_get_variables(wrapper, connection);
        }
        // Read a specific variable
        else if (strncmp(url, "/variable/", 10) == 0)
        {
            ret = handle_get_variable(wrapper, connection, url + 10);
        }
        else
        {
            ret = send_response(connection, 404, "{\"error\":\"Not found\"}");
        }

        free(post_data);
        *req_cls = NULL;
        return ret;
    }

    free(post_data);
    *req_cls = NULL;
    return send_response(connection, 405, "{\"error\":\"Method not allowed\"}");
}

int rest_api_start(RestApiWrapper *wrapper, ConnectionManager *manager, pthread_mutex_t *mutex, int port)
{
    wrapper->manager = manager;
    wrapper->connection_mutex = mutex;

    wrapper->daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD,
                                       (uint16_t)port, 
                                       NULL, 
                                       NULL,
                                       &handle_request, 
                                       wrapper,
                                       MHD_OPTION_END);

    if (wrapper->daemon == NULL)
    {
        logger_log(CLASS_REST_API, LOG_ERROR , "Error starting REST API on port %d", port);
        return 0;
    }

    logger_log(CLASS_REST_API, LOG_INFO , "REST API listening on port %d", port);
    return 1;
}

void rest_api_stop(RestApiWrapper *wrapper)
{
    if (wrapper->daemon != NULL)
    {
        MHD_stop_daemon(wrapper->daemon);
    }
}