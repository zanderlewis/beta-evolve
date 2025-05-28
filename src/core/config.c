#include "config.h"
#include "toml.h"
#include "colors.h"

// Check if endpoint is a local server
int is_local_server(const char *endpoint) {
    return (strstr(endpoint, "localhost") != NULL || 
            strstr(endpoint, "127.0.0.1") != NULL ||
            strstr(endpoint, "::1") != NULL);
}

// Load configuration from TOML file
int load_config(config_t *config, const char *config_file) {
    // Initialize new fields
    memset(config->problem_prompt_file, 0, sizeof(config->problem_prompt_file));
    memset(config->args, 0, sizeof(config->args));
    memset(config->evolution_file_path, 0, sizeof(config->evolution_file_path));
    memset(config->test_command, 0, sizeof(config->test_command));
    config->enable_evolution = 0;
    config->loaded_problem_prompt = NULL;
    
    FILE *file = fopen(config_file, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open config file %s\n", config_file);
        return -1;
    }

    char errbuf[200];
    toml_table_t *toml = toml_parse_file(file, errbuf, sizeof(errbuf));
    fclose(file);
    
    if (!toml) {
        fprintf(stderr, "Error: Invalid TOML in config file: %s\n", errbuf);
        return -1;
    }
    
    // Load endpoints first to determine if we're using local servers
    toml_datum_t fast_endpoint = toml_string_in(toml, "fast_model_endpoint");
    if (fast_endpoint.ok) {
        strcpy(config->fast_model_endpoint, fast_endpoint.u.s);
        free(fast_endpoint.u.s);
    } else {
        strcpy(config->fast_model_endpoint, "https://api.openai.com/v1/chat/completions");
    }
    
    toml_datum_t reasoning_endpoint = toml_string_in(toml, "reasoning_model_endpoint");
    if (reasoning_endpoint.ok) {
        strcpy(config->reasoning_model_endpoint, reasoning_endpoint.u.s);
        free(reasoning_endpoint.u.s);
    } else {
        strcpy(config->reasoning_model_endpoint, "https://api.openai.com/v1/chat/completions");
    }
    
    // Check if we're using local servers
    int fast_is_local = is_local_server(config->fast_model_endpoint);
    int reasoning_is_local = is_local_server(config->reasoning_model_endpoint);
    
    // Handle API keys - allow empty for any endpoint
    toml_datum_t fast_api_key = toml_string_in(toml, "fast_model_api_key");
    if (fast_api_key.ok && strlen(fast_api_key.u.s) > 0) {
        strcpy(config->fast_model_api_key, fast_api_key.u.s);
        free(fast_api_key.u.s);
    } else {
        strcpy(config->fast_model_api_key, "null");
        if (fast_is_local) {
            printf("Info: Using local server for fast model, API key not required\n");
        } else {
            printf("Info: No API key provided for fast model\n");
        }
        if (fast_api_key.ok) free(fast_api_key.u.s);
    }
    
    toml_datum_t reasoning_api_key = toml_string_in(toml, "reasoning_model_api_key");
    if (reasoning_api_key.ok && strlen(reasoning_api_key.u.s) > 0) {
        strcpy(config->reasoning_model_api_key, reasoning_api_key.u.s);
        free(reasoning_api_key.u.s);
    } else {
        strcpy(config->reasoning_model_api_key, "null");
        if (reasoning_is_local) {
            printf("Info: Using local server for reasoning model, API key not required\n");
        } else {
            printf("Info: No API key provided for reasoning model\n");
        }
        if (reasoning_api_key.ok) free(reasoning_api_key.u.s);
    }
    
    // Handle model names - allow empty for servers that host only one model
    toml_datum_t fast_model_name = toml_string_in(toml, "fast_model_name");
    if (fast_model_name.ok && strlen(fast_model_name.u.s) > 0) {
        strcpy(config->fast_model_name, fast_model_name.u.s);
        free(fast_model_name.u.s);
    } else {
        strcpy(config->fast_model_name, "null");
        if (fast_is_local) {
            printf("Info: No model name specified for local fast model server (using server default)\n");
        } else {
            printf("Info: No model name specified for fast model (using server default)\n");
        }
        if (fast_model_name.ok) free(fast_model_name.u.s);
    }
    
    toml_datum_t reasoning_model_name = toml_string_in(toml, "reasoning_model_name");
    if (reasoning_model_name.ok && strlen(reasoning_model_name.u.s) > 0) {
        strcpy(config->reasoning_model_name, reasoning_model_name.u.s);
        free(reasoning_model_name.u.s);
    } else {
        strcpy(config->reasoning_model_name, "null");
        if (reasoning_is_local) {
            printf("Info: No model name specified for local reasoning model server (using server default)\n");
        } else {
            printf("Info: No model name specified for reasoning model (using server default)\n");
        }
        if (reasoning_model_name.ok) free(reasoning_model_name.u.s);
    }

    // Load iteration count with default value of 3
    toml_datum_t iterations = toml_int_in(toml, "iterations");
    if (iterations.ok && iterations.u.i > 0) {
        config->iterations = (int)iterations.u.i;
    } else {
        config->iterations = 3;  // Default value
        printf("Info: Using default iteration count: %d\n", config->iterations);
    }

    // Load flexible configuration parameters with defaults
    toml_datum_t max_response_size = toml_int_in(toml, "max_response_size");
    if (max_response_size.ok && max_response_size.u.i > 0) {
        config->max_response_size = (int)max_response_size.u.i;
    } else {
        config->max_response_size = DEFAULT_MAX_RESPONSE_SIZE;
    }

    toml_datum_t max_prompt_size = toml_int_in(toml, "max_prompt_size");
    if (max_prompt_size.ok && max_prompt_size.u.i > 0) {
        config->max_prompt_size = (int)max_prompt_size.u.i;
    } else {
        config->max_prompt_size = DEFAULT_MAX_PROMPT_SIZE;
    }

    toml_datum_t max_conversation_turns = toml_int_in(toml, "max_conversation_turns");
    if (max_conversation_turns.ok && max_conversation_turns.u.i > 0) {
        config->max_conversation_turns = (int)max_conversation_turns.u.i;
    } else {
        config->max_conversation_turns = DEFAULT_MAX_CONVERSATION_TURNS;
    }

    toml_datum_t max_code_size = toml_int_in(toml, "max_code_size");
    if (max_code_size.ok && max_code_size.u.i > 0) {
        config->max_code_size = (int)max_code_size.u.i;
    } else {
        config->max_code_size = DEFAULT_MAX_CODE_SIZE;
    }

    // Load verbosity setting
    toml_datum_t verbosity = toml_int_in(toml, "verbosity");
    if (verbosity.ok && verbosity.u.i >= -1 && verbosity.u.i <= 2) {
        config->verbosity = verbosity.u.i;
    } else {
        config->verbosity = VERBOSITY_NORMAL; // Default verbosity (0)
    }

    // Load color setting
    toml_datum_t use_colors = toml_bool_in(toml, "use_colors");
    if (use_colors.ok) {
        config->use_colors = use_colors.u.b;
    } else {
        config->use_colors = 1; // Default to enabled
    }

    // Initialize colors based on config
    if (config->use_colors) {
        colors_enable();
    } else {
        colors_disable();
    }
    colors_init();

    // Load problem prompt file if specified
    toml_datum_t problem_prompt_file = toml_string_in(toml, "problem_prompt_file");
    if (problem_prompt_file.ok && strlen(problem_prompt_file.u.s) > 0) {
        strcpy(config->problem_prompt_file, problem_prompt_file.u.s);
        free(problem_prompt_file.u.s);
        
        // Try to load the file immediately
        if (load_problem_prompt_file(config, config->problem_prompt_file) != 0) {
            printf("Warning: Failed to load problem prompt file '%s'\n", config->problem_prompt_file);
            config->loaded_problem_prompt = NULL;
        } else {
            printf("Info: Loaded problem description from '%s'\n", config->problem_prompt_file);
        }
    } else {
        strcpy(config->problem_prompt_file, "");
        config->loaded_problem_prompt = NULL;
        if (problem_prompt_file.ok) free(problem_prompt_file.u.s);
    }

    // Load additional arguments if specified
    toml_datum_t args = toml_string_in(toml, "args");
    if (args.ok && strlen(args.u.s) > 0) {
        strcpy(config->args, args.u.s);
        free(args.u.s);
        printf("Info: Using additional arguments: '%s'\n", config->args);
    } else {
        strcpy(config->args, "");
        if (args.ok) free(args.u.s);
    }

    // Load evolution configuration
    toml_datum_t evolution_file_path = toml_string_in(toml, "evolution_file_path");
    if (evolution_file_path.ok && strlen(evolution_file_path.u.s) > 0) {
        strcpy(config->evolution_file_path, evolution_file_path.u.s);
        free(evolution_file_path.u.s);
        printf("Info: Evolution file: '%s'\n", config->evolution_file_path);
    } else {
        strcpy(config->evolution_file_path, "");
        if (evolution_file_path.ok) free(evolution_file_path.u.s);
    }

    toml_datum_t test_command = toml_string_in(toml, "test_command");
    if (test_command.ok && strlen(test_command.u.s) > 0) {
        strcpy(config->test_command, test_command.u.s);
        free(test_command.u.s);
        printf("Info: Test command: '%s'\n", config->test_command);
    } else {
        strcpy(config->test_command, "");
        if (test_command.ok) free(test_command.u.s);
    }

    toml_datum_t enable_evolution = toml_bool_in(toml, "enable_evolution");
    if (enable_evolution.ok) {
        config->enable_evolution = enable_evolution.u.b;
    } else {
        config->enable_evolution = 0; // Default to disabled
    }

    if (config->enable_evolution) {
        printf("Info: Evolution mode enabled\n");
    }

    toml_free(toml);
    return 0;
}

// Load problem description from file
int load_problem_prompt_file(config_t *config, const char *prompt_file_path) {
    if (!config || !prompt_file_path || strlen(prompt_file_path) == 0) {
        return -1;
    }
    
    FILE *file = fopen(prompt_file_path, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open problem prompt file %s\n", prompt_file_path);
        return -1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0 || file_size > config->max_prompt_size * 2) {
        fprintf(stderr, "Error: Problem prompt file size invalid or too large (%ld bytes)\n", file_size);
        fclose(file);
        return -1;
    }
    
    // Free existing prompt if any
    if (config->loaded_problem_prompt) {
        free(config->loaded_problem_prompt);
        config->loaded_problem_prompt = NULL;
    }
    
    // Allocate memory for prompt content
    config->loaded_problem_prompt = malloc(file_size + 1);
    if (!config->loaded_problem_prompt) {
        fprintf(stderr, "Error: Memory allocation failed for problem prompt file\n");
        fclose(file);
        return -1;
    }
    
    // Read file content
    size_t bytes_read = fread(config->loaded_problem_prompt, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "Error: Failed to read complete problem prompt file\n");
        free(config->loaded_problem_prompt);
        config->loaded_problem_prompt = NULL;
        return -1;
    }
    
    // Null terminate and clean up whitespace
    config->loaded_problem_prompt[file_size] = '\0';
    
    // Remove trailing whitespace
    char *end = config->loaded_problem_prompt + strlen(config->loaded_problem_prompt) - 1;
    while (end > config->loaded_problem_prompt && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    
    return 0;
}

// Free config resources
void free_config(config_t *config) {
    if (!config) return;
    
    if (config->loaded_problem_prompt) {
        free(config->loaded_problem_prompt);
        config->loaded_problem_prompt = NULL;
    }
}
