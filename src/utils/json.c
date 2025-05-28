#include "json.h"
#include <string.h>

// Create a chat request JSON object using cJSON
cJSON* json_create_chat_request(const char *model, const char *message, double temperature) {
    cJSON *request = cJSON_CreateObject();
    if (!request) return NULL;
    
    // Add model if provided
    if (model && strlen(model) > 0 && strcmp(model, "null") != 0) {
        cJSON *model_json = cJSON_CreateString(model);
        if (model_json) {
            cJSON_AddItemToObject(request, "model", model_json);
        }
    }
    
    // Create messages array
    cJSON *messages = cJSON_CreateArray();
    if (!messages) {
        cJSON_Delete(request);
        return NULL;
    }
    
    // Create message object
    cJSON *msg = cJSON_CreateObject();
    if (!msg) {
        cJSON_Delete(messages);
        cJSON_Delete(request);
        return NULL;
    }
    
    cJSON *role = cJSON_CreateString("user");
    cJSON *content = cJSON_CreateString(message);
    
    if (!role || !content) {
        cJSON_Delete(role);
        cJSON_Delete(content);
        cJSON_Delete(msg);
        cJSON_Delete(messages);
        cJSON_Delete(request);
        return NULL;
    }
    
    cJSON_AddItemToObject(msg, "role", role);
    cJSON_AddItemToObject(msg, "content", content);
    cJSON_AddItemToArray(messages, msg);
    
    // Add messages array to request
    cJSON_AddItemToObject(request, "messages", messages);
    
    // Add temperature
    cJSON *temp = cJSON_CreateNumber(temperature);
    if (temp) {
        cJSON_AddItemToObject(request, "temperature", temp);
    }
    
    return request;
}

// Extract response content from chat completion JSON
const char* json_extract_chat_response(cJSON *response) {
    if (!response) return NULL;
    
    cJSON *choices = cJSON_GetObjectItem(response, "choices");
    if (!choices || !cJSON_IsArray(choices) || cJSON_GetArraySize(choices) == 0) {
        return NULL;
    }
    
    cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
    if (!first_choice) return NULL;
    
    cJSON *message = cJSON_GetObjectItem(first_choice, "message");
    if (!message) return NULL;
    
    cJSON *content = cJSON_GetObjectItem(message, "content");
    if (!content || !cJSON_IsString(content)) return NULL;
    
    return cJSON_GetStringValue(content);
}
