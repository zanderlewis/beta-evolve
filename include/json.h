#ifndef JSON_H
#define JSON_H

#include <cJSON.h>
#include <stdbool.h>

// Utility functions for creating chat requests and extracting responses
cJSON* json_create_chat_request(const char *model, const char *message, double temperature);
const char* json_extract_chat_response(cJSON *response);

#endif // JSON_H
