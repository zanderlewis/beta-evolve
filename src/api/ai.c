#include "ai.h"
#include "json.h"
#include <sys/wait.h>

// Call AI model via API using curl
char* call_ai_model(const char* prompt, agent_type_t agent, config_t *config) {
    if (!prompt || !config) {
        fprintf(stderr, "Error: Invalid parameters for AI model call\n");
        return NULL;
    }
    
    // Get model configuration
    const char *endpoint = agent == AGENT_FAST ? config->fast_model_endpoint : config->reasoning_model_endpoint;
    const char *model_name = agent == AGENT_FAST ? config->fast_model_name : config->reasoning_model_name;
    const char *api_key = agent == AGENT_FAST ? config->fast_model_api_key : config->reasoning_model_api_key;
    
    // Create JSON request
    double temperature = agent == AGENT_FAST ? 0.8 : 0.3;
    cJSON *request_json = json_create_chat_request(model_name, prompt, temperature);
    if (!request_json) {
        fprintf(stderr, "Error: Failed to create JSON request\n");
        return NULL;
    }
    
    char *json_string = cJSON_Print(request_json);
    cJSON_Delete(request_json);
    
    if (!json_string) {
        fprintf(stderr, "Error: Failed to stringify JSON request\n");
        return NULL;
    }
    
    // Create temporary files for curl
    char temp_request_file[] = "/tmp/beta_evolve_request_XXXXXX";
    char temp_response_file[] = "/tmp/beta_evolve_response_XXXXXX";
    
    int request_fd = mkstemp(temp_request_file);
    int response_fd = mkstemp(temp_response_file);
    
    if (request_fd == -1 || response_fd == -1) {
        fprintf(stderr, "Error: Failed to create temporary files\n");
        free(json_string);
        return NULL;
    }
    
    // Write JSON to temp file
    write(request_fd, json_string, strlen(json_string));
    close(request_fd);
    free(json_string);
    
    // Build curl command
    char curl_command[2048];
    
    if (api_key && strlen(api_key) > 0 && strcmp(api_key, "null") != 0) {
        snprintf(curl_command, sizeof(curl_command),
            "curl -s -X POST '%s' "
            "-H 'Content-Type: application/json' "
            "-H 'Authorization: Bearer %s' "
            "-d @%s "
            "-o %s",
            endpoint, api_key, temp_request_file, temp_response_file);
    } else {
        printf("Info: Skipping Authorization header (no API key provided)\n");
        snprintf(curl_command, sizeof(curl_command),
            "curl -s -X POST '%s' "
            "-H 'Content-Type: application/json' "
            "-d @%s "
            "-o %s",
            endpoint, temp_request_file, temp_response_file);
    }
    
    // Make HTTP request
    printf("%s Agent: Making API call...\n", agent == AGENT_FAST ? "Fast" : "Reasoning");
    int curl_result = system(curl_command);
    
    // Clean up request file
    unlink(temp_request_file);
    
    if (curl_result != 0) {
        fprintf(stderr, "Error: curl command failed\n");
        close(response_fd);
        unlink(temp_response_file);
        return NULL;
    }
    
    // Read response from temp file
    close(response_fd);
    FILE *response_file = fopen(temp_response_file, "r");
    if (!response_file) {
        fprintf(stderr, "Error: Failed to open response file\n");
        unlink(temp_response_file);
        return NULL;
    }
    
    // Get file size
    fseek(response_file, 0, SEEK_END);
    long file_size = ftell(response_file);
    fseek(response_file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fprintf(stderr, "Error: Empty response received\n");
        fclose(response_file);
        unlink(temp_response_file);
        return NULL;
    }
    
    // Read response data
    char *response_data = malloc(file_size + 1);
    if (!response_data) {
        fprintf(stderr, "Error: Failed to allocate memory for response\n");
        fclose(response_file);
        unlink(temp_response_file);
        return NULL;
    }
    
    fread(response_data, 1, file_size, response_file);
    response_data[file_size] = '\0';
    
    fclose(response_file);
    unlink(temp_response_file);
    
    // Parse JSON response
    cJSON *response_json = cJSON_Parse(response_data);
    if (!response_json) {
        const char *error_ptr = cJSON_GetErrorPtr();
        fprintf(stderr, "Error: Failed to parse response JSON\n");
        if (error_ptr != NULL) {
            fprintf(stderr, "JSON parse error before: %s\n", error_ptr);
        }
        fprintf(stderr, "Raw response: %s\n", response_data);
        free(response_data);
        return NULL;
    }
    
    free(response_data);
    
    // Check for API errors
    cJSON *error_obj = cJSON_GetObjectItem(response_json, "error");
    if (error_obj) {
        cJSON *error_message = cJSON_GetObjectItem(error_obj, "message");
        const char *error_text = cJSON_GetStringValue(error_message);
        fprintf(stderr, "API Error: %s\n", error_text ? error_text : "Unknown error");
        cJSON_Delete(response_json);
        return NULL;
    }
    
    // Extract response content
    const char *content = json_extract_chat_response(response_json);
    if (!content) {
        fprintf(stderr, "Error: Failed to extract content from response\n");
        cJSON_Delete(response_json);
        return NULL;
    }
    
    char *result = strdup(content);
    
    // Cleanup
    cJSON_Delete(response_json);
    
    // Write to log file
    FILE *log_file = fopen("beta-evolve.log", "a");
    if (log_file) {
        fprintf(log_file, "Agent: %s\nPrompt: %s\nResponse: %s\n\n", 
                agent == AGENT_FAST ? "Fast" : "Reasoning", prompt, result ? result : "No response");
        fclose(log_file);
    } else {
        fprintf(stderr, "Error: Failed to open log file for writing\n");
    }
    
    return result;
}

// Validate and clean AI response
char* validate_and_clean_response(const char* response) {
    if (!response) return NULL;
    
    // Find the start of the code block
    const char *code_start = strstr(response, "```c");
    
    // If no code block with 'c' specified, look for any code block
    if (!code_start) {
        code_start = strstr(response, "```");
    }
    
    if (!code_start) {
        // No code block found, try to salvage by wrapping response
        char *cleaned = malloc(strlen(response) + 100);
        if (cleaned) {
            snprintf(cleaned, strlen(response) + 100, 
                "Analysis: Attempting to fix the code.\n\n```c\n%s\n```", response);
        }
        return cleaned;
    }
    
    // Find the end of the code block
    const char *code_end = strstr(code_start + 3, "```");
    if (!code_end) {
        // Incomplete code block, add closing marker
        char *cleaned = malloc(strlen(response) + 10);
        if (cleaned) {
            snprintf(cleaned, strlen(response) + 10, "%s\n```", response);
        }
        return cleaned;
    }
    
    // Response appears valid, return a copy
    return strdup(response);
}
