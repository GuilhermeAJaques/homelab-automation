#include "opc-ua_client.h"
#include <string.h>
#include <stdio.h>

static int is_connected(OPCClientWrapper *wrapper);
static int find_node(OPCClientWrapper *wrapper, UA_NodeId startNode, const char *variableName, UA_NodeId *foundNode, int depth);
static void convert_to_string(UA_Variant *variant, char *value, int max_len);
static void convert_from_string(const char *value, UA_Variant *originalVariant, UA_Variant *outVariant);

void opc_client_init(OPCClientWrapper *wrapper, const char *url)
{
    wrapper->client = UA_Client_new();
    strcpy(wrapper->url, url);
    wrapper->connected = 0;
    wrapper->cacheCount = 0;
}

int opc_client_connect(OPCClientWrapper *wrapper)
{
    int rc = UA_Client_connect(wrapper->client, wrapper->url);

    if (rc == UA_STATUSCODE_GOOD)
    {
        printf("Connected to OPC-UA server: %s\n", wrapper->url);
        wrapper->connected = 1;
    }
    else
    {
        printf("Fail to connect to OPC-UA server: %s with code: %d\n", wrapper->url, rc);
    }
    return rc;
}

void opc_client_disconnect(OPCClientWrapper *wrapper)
{
    int rc = UA_Client_disconnect(wrapper->client);

    
    if (rc == UA_STATUSCODE_GOOD)
    {
        printf("Disconeected to OPC-UA server: %s\n", wrapper->url);
        UA_Client_delete(wrapper->client);
        wrapper->connected = 0;
    }
    else
    {
        printf("Fail to disconnect to OPC-UA server: %s with code: %d\n", wrapper->url, rc);
    }
}

int opc_client_read(OPCClientWrapper *wrapper, char *variableName, char *value, int max_len)
{
    if (!is_connected(wrapper))
    {
        printf("Connection lost, attempting to reconnect...\n");
        UA_Client_disconnect(wrapper->client);
        int rc = UA_Client_connect(wrapper->client, wrapper->url);
        if (rc != UA_STATUSCODE_GOOD)
        {
            printf("Reconnect failed\n");
            return -1;
        }
        wrapper->connected = 1;
    }

    // Start a recursice search to get node ID
    UA_NodeId node;
    UA_NodeId startNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    int rc = find_node(wrapper, startNode, variableName, &node, 0);
    if (rc == 0)
    {
        printf("Variable %s not found\n", variableName);
        return -1;
    }

    // Read value
    UA_Variant uaValue;
    UA_Variant_init(&uaValue);
    rc = UA_Client_readValueAttribute(wrapper->client, node, &uaValue);
    if (rc != UA_STATUSCODE_GOOD)
    {
        printf("Fail to read the variable %s to OPC-UA server: %s with code: %d\n",variableName, wrapper->url, rc);
        UA_Variant_clear(&uaValue);
        return rc;
    }

    // Convert to string
    convert_to_string(&uaValue, value, max_len);
    UA_Variant_clear(&uaValue);
    return rc;
}

int opc_client_write(OPCClientWrapper *wrapper, char *variableName,const char *value)
{
    if (!is_connected(wrapper))
    {
        printf("Connection lost, attempting to reconnect...\n");
        UA_Client_disconnect(wrapper->client);
        int rc = UA_Client_connect(wrapper->client, wrapper->url);
        if (rc != UA_STATUSCODE_GOOD)
        {
            printf("Reconnect failed\n");
            return -1;
        }
        wrapper->connected = 1;
    }

    // Start a recursice search to get node ID
    UA_NodeId node;
    UA_NodeId startNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    int rc = find_node(wrapper, startNode, variableName, &node, 0);
    if (rc == 0)
    {
        printf("Variable %s not found\n", variableName);
        return -1;
    }

    // Read desired variable to get the datatype
    UA_Variant uaDatatype;
    UA_Variant_init(&uaDatatype);
    rc = UA_Client_readValueAttribute(wrapper->client, node, &uaDatatype);
    if (rc != UA_STATUSCODE_GOOD)
    {
        printf("Error reading current value to determine type: %s\n", variableName);
        UA_Variant_clear(&uaDatatype);
        return rc;
    }

    // Now write value with correct datatype
    UA_Variant uaValue;
    convert_from_string(value, &uaDatatype, &uaValue);
    rc = UA_Client_writeValueAttribute(wrapper->client, node, &uaValue);

    UA_Variant_clear(&uaDatatype);
    UA_Variant_clear(&uaValue);

    if (rc != UA_STATUSCODE_GOOD)
    {
        printf("Error writing variable %s: %d\n", variableName, rc);
        return rc;
    }

    return rc;
}

static int is_connected(OPCClientWrapper *wrapper)
{
    UA_SessionState sessionState;
    UA_Client_getState(wrapper->client, NULL, &sessionState, NULL);
    return sessionState == UA_SESSIONSTATE_ACTIVATED;
}

static int find_node(OPCClientWrapper *wrapper, UA_NodeId startNode, const char *variableName, UA_NodeId *foundNode, int depth)
{
    // Check recursive deep
    if (depth > 20) 
    {
        return 0;
    }

    if (depth == 0) // Just execute in the first run
    {
        for (int i = 0; i < wrapper->cacheCount; i++)
        {
            if (strcmp(wrapper->cache[i].variableName, variableName) == 0)
            {
                UA_NodeId_copy(&wrapper->cache[i].nodeId, foundNode);
                return 1;
            }
        }
    }

    // Create instance for method to browse variable
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = startNode;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.includeSubtypes = true;
    bd.nodeClassMask = UA_NODECLASS_UNSPECIFIED;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    UA_BrowseResult result = UA_Client_browse(wrapper->client, NULL, 0, &bd);
    // Get desired node name
    UA_String targetName = UA_STRING((char *)variableName);

    // Start to search in all childs of the current node
    int found = 0;
    int keepGoing = 1;
    while (keepGoing && !found)
    {
        // Pass by each child inside the current node
        for (size_t i = 0; i < result.referencesSize && !found; i++)
        {
            // Get current node name
            UA_ReferenceDescription *ref = &result.references[i];

            // If match the node ID name
            if (targetName.length == ref->browseName.name.length &&
                strncmp((char *)targetName.data, (char *)ref->browseName.name.data, targetName.length) == 0)
            {
                UA_NodeId_copy(&ref->nodeId.nodeId, foundNode);
                
                // Save in cache
                if (wrapper->cacheCount < MAX_CACHED_NODES)
                {
                    UA_NodeId_copy(&ref->nodeId.nodeId, &wrapper->cache[wrapper->cacheCount].nodeId);
                    strcpy(wrapper->cache[wrapper->cacheCount].variableName, variableName);
                    wrapper->cacheCount += 1;
                }

                found = 1;
            }
            else
            {
                // If not match node name, but the node has childs starts a recursion
                if (ref->nodeClass == UA_NODECLASS_OBJECT)
                {
                    UA_NodeId childNode = ref->nodeId.nodeId;
                    found = find_node(wrapper, childNode, variableName, foundNode, depth + 1);
                }
            }
        }

        // If not found in the recursion, but this node ID has more childs, check each of then
        // This is needed because this librarie separate the childs in more than one group
        if (!found && result.continuationPoint.length > 0)
        {
            UA_BrowseResult nextResult = UA_Client_browseNext(wrapper->client, false, result.continuationPoint);
            UA_BrowseResult_clear(&result);
            result = nextResult;
        }
        else
        {
            keepGoing = 0;
        }
    }

    UA_BrowseResult_clear(&result);
    return found;
}

static void convert_to_string(UA_Variant *variant, char *value, int max_len)
{
    if (variant->type == &UA_TYPES[UA_TYPES_BOOLEAN])
    {
        UA_Boolean val = *(UA_Boolean *)variant->data;
        snprintf(value, max_len, val ? "true" : "false");
    }
    else if (variant->type == &UA_TYPES[UA_TYPES_SBYTE])
    {
        UA_SByte val = *(UA_SByte *)variant->data;
        snprintf(value, max_len, "%d", val);
    }
    else if (variant->type == &UA_TYPES[UA_TYPES_BYTE])
    {
        UA_Byte val = *(UA_Byte *)variant->data;
        snprintf(value, max_len, "%u", val);
    }
    else if (variant->type == &UA_TYPES[UA_TYPES_INT16])
    {
        UA_Int16 val = *(UA_Int16 *)variant->data;
        snprintf(value, max_len, "%d", val);
    }
    else if (variant->type == &UA_TYPES[UA_TYPES_UINT16])
    {
        UA_UInt16 val = *(UA_UInt16 *)variant->data;
        snprintf(value, max_len, "%u", val);
    }
    else if (variant->type == &UA_TYPES[UA_TYPES_INT32])
    {
        UA_Int32 val = *(UA_Int32 *)variant->data;
        snprintf(value, max_len, "%d", val);
    }
    else if (variant->type == &UA_TYPES[UA_TYPES_UINT32])
    {
        UA_UInt32 val = *(UA_UInt32 *)variant->data;
        snprintf(value, max_len, "%u", val);
    }
    else if (variant->type == &UA_TYPES[UA_TYPES_FLOAT])
    {
        UA_Float val = *(UA_Float *)variant->data;
        snprintf(value, max_len, "%f", val);
    }
    else if (variant->type == &UA_TYPES[UA_TYPES_DOUBLE])
    {
        UA_Double val = *(UA_Double *)variant->data;
        snprintf(value, max_len, "%f", val);
    }
    else if (variant->type == &UA_TYPES[UA_TYPES_STRING])
    {
        UA_String *str = (UA_String *)variant->data;
        int len = (int)str->length;
        if (len > max_len - 1) 
        {
            len = max_len - 1;
        }
        memcpy(value, str->data, len);
        value[len] = '\0';
    }
    else
    {
        printf("Unknown OPC-UA data type\n");
        snprintf(value, max_len, "?");
    }
}

static void convert_from_string(const char *value, UA_Variant *originalVariant, UA_Variant *outVariant)
{
    UA_Variant_init(outVariant);

    if (originalVariant->type == &UA_TYPES[UA_TYPES_BOOLEAN])
    {
        UA_Boolean val = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) ? true : false;
        UA_Variant_setScalarCopy(outVariant, &val, &UA_TYPES[UA_TYPES_BOOLEAN]);
    }
    else if (originalVariant->type == &UA_TYPES[UA_TYPES_SBYTE])
    {
        UA_SByte val = (UA_SByte)atoi(value);
        UA_Variant_setScalarCopy(outVariant, &val, &UA_TYPES[UA_TYPES_SBYTE]);
    }
    else if (originalVariant->type == &UA_TYPES[UA_TYPES_BYTE])
    {
        UA_Byte val = (UA_Byte)atoi(value);
        UA_Variant_setScalarCopy(outVariant, &val, &UA_TYPES[UA_TYPES_BYTE]);
    }
    else if (originalVariant->type == &UA_TYPES[UA_TYPES_INT16])
    {
        UA_Int16 val = (UA_Int16)atoi(value);
        UA_Variant_setScalarCopy(outVariant, &val, &UA_TYPES[UA_TYPES_INT16]);
    }
    else if (originalVariant->type == &UA_TYPES[UA_TYPES_UINT16])
    {
        UA_UInt16 val = (UA_UInt16)atoi(value);
        UA_Variant_setScalarCopy(outVariant, &val, &UA_TYPES[UA_TYPES_UINT16]);
    }
    else if (originalVariant->type == &UA_TYPES[UA_TYPES_INT32])
    {
        UA_Int32 val = (UA_Int32)atoi(value);
        UA_Variant_setScalarCopy(outVariant, &val, &UA_TYPES[UA_TYPES_INT32]);
    }
    else if (originalVariant->type == &UA_TYPES[UA_TYPES_UINT32])
    {
        UA_UInt32 val = (UA_UInt32)atoi(value);
        UA_Variant_setScalarCopy(outVariant, &val, &UA_TYPES[UA_TYPES_UINT32]);
    }
    else if (originalVariant->type == &UA_TYPES[UA_TYPES_FLOAT])
    {
        UA_Float val = (UA_Float)atof(value);
        UA_Variant_setScalarCopy(outVariant, &val, &UA_TYPES[UA_TYPES_FLOAT]);
    }
    else if (originalVariant->type == &UA_TYPES[UA_TYPES_DOUBLE])
    {
        UA_Double val = (UA_Double)atof(value);
        UA_Variant_setScalarCopy(outVariant, &val, &UA_TYPES[UA_TYPES_DOUBLE]);
    }
    else if (originalVariant->type == &UA_TYPES[UA_TYPES_STRING])
    {
        UA_String val = UA_STRING((char *)value);
        UA_Variant_setScalarCopy(outVariant, &val, &UA_TYPES[UA_TYPES_STRING]);
    }
    else
    {
        printf("Unknown OPC-UA data type for write\n");
    }
}