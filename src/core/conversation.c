#include "beta_evolve.h"


// Generate base prompt template for agents
char* _prompt() {
    return ("You are an expert C programmer. Your task is to solve the given problem with high-quality, error-free C code.\n\n"
        "PROBLEM: %s\n\n"
        "CURRENT CODE: %s\n\n"
        "ERRORS TO FIX: %s\n\n"
        "ERROR CODES:\n"
        "Code 139: Segmentation Fault\n"
        "Code 1: Exit code 1 indicates a failure in the test suite\n\n"
        "INSTRUCTIONS:\n"
        "1. Analyze the current code and any errors carefully\n"
        "2. If there are runtime errors (segfaults, memory issues), focus on:\n"
        "   - Proper memory allocation/deallocation\n"
        "   - Null pointer checks\n"
        "   - Array bounds checking\n"
        "   - String handling safety\n"
        "3. If there are compilation errors, fix syntax and missing declarations\n"
        "4. Always include comprehensive test functions that validate your implementation\n"
        "5. Use defensive programming practices\n\n"
        "RESPONSE FORMAT:\n"
        "Provide a brief analysis (1-2 sentences) followed by your complete code solution.\n\n"
        "Analysis: [Your brief analysis of the issue and solution approach]\n\n"
        "```c\n"
        "// Your complete, working C code implementation\n"
        "```\n\n"
        "CRITICAL REQUIREMENTS:\n"
        "- Code must compile without warnings using: gcc -Wall -Wextra -std=c99\n"
        "- Code must run without segfaults or memory leaks\n"
        "- Include proper error handling and bounds checking\n"
        "- Test functions must thoroughly validate the implementation\n"
        "- If fixing existing code, preserve working parts and only fix problematic areas\n"
        "- All tests must exit with a return code of 0 for success\n"
        "- All tests must exit the program with a return code of 1 for failure\n"
        "- ZERO todos or placeholders or 'For Now' comments. You must implement them completely\n"
        "Your response:");
}

// Initialize conversation
void init_conversation(conversation_t *conv, const char *problem, config_t *config) {
    memset(conv, 0, sizeof(conversation_t));
    
    // Set config reference
    conv->config = config;
    
    // Allocate problem description
    conv->problem_description = malloc(config->max_prompt_size);
    if (conv->problem_description) {
        strncpy(conv->problem_description, problem, config->max_prompt_size - 1);
        conv->problem_description[config->max_prompt_size - 1] = '\0';
    }
    
    // Allocate current solution
    conv->current_solution = malloc(config->max_code_size);
    if (conv->current_solution) {
        memset(conv->current_solution, 0, config->max_code_size);
    }
    
    // Allocate messages array
    conv->max_messages = config->max_conversation_turns * 2;
    conv->messages = malloc(conv->max_messages * sizeof(message_t));
    if (conv->messages) {
        memset(conv->messages, 0, conv->max_messages * sizeof(message_t));
    }
    
    conv->message_count = 0;
    conv->iterations = 0;
    
    // Initialize test result with dynamic allocation
    memset(&conv->last_test_result, 0, sizeof(test_result_t));
    conv->last_test_result.error_message = malloc(config->max_response_size);
    conv->last_test_result.output = malloc(config->max_response_size);
    if (conv->last_test_result.error_message) {
        memset(conv->last_test_result.error_message, 0, config->max_response_size);
    }
    if (conv->last_test_result.output) {
        memset(conv->last_test_result.output, 0, config->max_response_size);
    }
    
    // Initialize code evolution context
    init_code_evolution(&conv->evolution);
}

// Add message to conversation
void add_message(conversation_t *conv, agent_type_t sender, const char *content) {
    if (!conv || !conv->messages || !conv->config) return;
    
    if (conv->message_count >= conv->max_messages) {
        // Shift messages to make room (simple sliding window)
        // Free the content of the first message
        if (conv->messages[0].content) {
            free(conv->messages[0].content);
        }
        
        for (int i = 0; i < conv->max_messages - 1; i++) {
            conv->messages[i] = conv->messages[i + 1];
        }
        conv->message_count = conv->max_messages - 1;
    }
    
    conv->messages[conv->message_count].sender = sender;
    conv->messages[conv->message_count].content = malloc(conv->config->max_response_size);
    if (conv->messages[conv->message_count].content) {
        strncpy(conv->messages[conv->message_count].content, content, conv->config->max_response_size - 1);
        conv->messages[conv->message_count].content[conv->config->max_response_size - 1] = '\0';
    }
    conv->messages[conv->message_count].timestamp = time(NULL);
    conv->message_count++;
}

// Print conversation history
void print_conversation(const conversation_t *conv) {
    if (!conv || !conv->config || conv->config->verbosity < VERBOSITY_VERBOSE) {
        return;
    }
    
    print_header("BETA EVOLVE CONVERSATION");
    log_message(conv->config, VERBOSITY_VERBOSE, "%sProblem:%s %s\n", C_EMPHASIS, C_RESET, 
               conv->problem_description ? conv->problem_description : "No problem description");
    log_message(conv->config, VERBOSITY_VERBOSE, "%sIteration:%s %d\n\n", C_EMPHASIS, C_RESET, conv->iterations);
    
    for (int i = 0; i < conv->message_count; i++) {
        const char *agent_name = conv->messages[i].sender == AGENT_FAST ? "Fast Agent" : "Reasoning Agent";
        const char *agent_color = conv->messages[i].sender == AGENT_FAST ? C_BRIGHT_BLUE : C_BRIGHT_MAGENTA;
        
        log_message(conv->config, VERBOSITY_VERBOSE, "%s%s[%s]:%s %s\n\n", 
                   agent_color, C_BOLD, agent_name, C_RESET,
                   conv->messages[i].content ? conv->messages[i].content : "No content");
    }
    
    if (conv->current_solution && strlen(conv->current_solution) > 0) {
        log_message(conv->config, VERBOSITY_VERBOSE, "%s=== CURRENT SOLUTION ===%s\n", C_HEADER, C_RESET);
        print_code_block(conv->current_solution, "c");
    }
    
    print_separator();
}

// Generate prompt for any agent type using dynamic strings
char* generate_agent_prompt(const conversation_t *conv, agent_type_t agent) {
    if (!conv || !conv->config) return NULL;
    
    // Check if the current solution contains evolution markers
    int has_evolution_markers = 0;
    if (conv->current_solution && strlen(conv->current_solution) > 0) {
        has_evolution_markers = (strstr(conv->current_solution, EVOLUTION_MARKER_START) != NULL);
    }
    
    // Use evolution-specific prompt if markers are detected
    if (has_evolution_markers) {
        return generate_evolution_prompt(conv, &conv->evolution, agent);
    }
    
    // Fall back to regular prompt generation
    dstring_t* prompt = dstring_create(conv->config->max_prompt_size * 2);
    if (!prompt) return NULL;
    
    const char *current_code = (conv->current_solution && strlen(conv->current_solution) > 0) ? conv->current_solution : "None";
    const char *errors = (conv->last_test_result.error_message && strlen(conv->last_test_result.error_message) > 0) ? conv->last_test_result.error_message : "None";
    
    // Generate role-specific context
    if (agent == AGENT_FAST) {
        dstring_append_format(prompt,
            "You are the FAST/DESIGN/IDEA AGENT (iteration %d). Your role is to quickly generate working code solutions and ideas.\n"
            "Focus on: Rapid prototyping, core functionality, and getting something that compiles and runs.\n"
            "Be efficient but ensure the code is syntactically correct and addresses the core problem.\n\n",
            conv->iterations);
    } else {
        dstring_append_format(prompt,
            "You are the REASONING AGENT (iteration %d). Your role is to analyze and improve code quality.\n"
            "Focus on: Error analysis, optimization, edge cases, memory safety, and robust testing.\n"
            "The Fast Agent has provided initial code. Your job is to:\n"
            "1. Fix any runtime/memory errors (segfaults, leaks, etc.)\n"
            "2. Improve error handling and edge case coverage\n"
            "3. Enhance testing to catch more issues\n"
            "4. Optimize for performance and maintainability\n"
            "5. Add comprehensive bounds checking and null pointer validation\n\n",
            conv->iterations);
        
        // Add conversation history for reasoning agent
        if (conv->message_count > 0) {
            dstring_append(prompt, "RECENT CONVERSATION:\n");
            int start = conv->message_count > 4 ? conv->message_count - 4 : 0;
            for (int i = start; i < conv->message_count; i++) {
                const char *agent_name = conv->messages[i].sender == AGENT_FAST ? "Fast" : "Reasoning";
                const char *content = conv->messages[i].content ? conv->messages[i].content : "No content";
                dstring_append_format(prompt, "%s: %.150s...\n", agent_name, content);
            }
            dstring_append(prompt, "\n");
        }
    }
    
    // Add the base prompt template
    dstring_append(prompt, _prompt());
    
    // Create final prompt with substitutions
    dstring_t* final_prompt = dstring_create(dstring_get(prompt) ? strlen(dstring_get(prompt)) + 1000 : 1000);
    if (!final_prompt) {
        dstring_destroy(prompt);
        return NULL;
    }
    
    const char *problem_desc = conv->problem_description ? conv->problem_description : "No problem description";
    dstring_append_format(final_prompt, dstring_get(prompt), problem_desc, current_code, errors);
    
    // Extract the final string
    char* result = NULL;
    if (dstring_get(final_prompt)) {
        result = strdup(dstring_get(final_prompt));
    }
    
    dstring_destroy(prompt);
    dstring_destroy(final_prompt);
    
    return result;
}

// Update current solution from reasoning agent response
void update_solution(conversation_t *conv, const char *reasoning_response) {
    if (!conv || !conv->current_solution || !conv->config) return;
    
    // Simple extraction - look for code blocks
    const char *code_start = strstr(reasoning_response, "```");
    if (code_start) {
        code_start += 3;
        // Skip language identifier if present
        const char *newline = strchr(code_start, '\n');
        if (newline) code_start = newline + 1;
        
        const char *code_end = strstr(code_start, "```");
        if (code_end) {
            size_t code_length = code_end - code_start;
            if (code_length < (size_t)(conv->config->max_code_size - 1)) {
                strncpy(conv->current_solution, code_start, code_length);
                conv->current_solution[code_length] = '\0';
                
                // Clean up trailing whitespace
                char *end = conv->current_solution + strlen(conv->current_solution) - 1;
                while (end > conv->current_solution && (*end == ' ' || *end == '\t' || *end == '\n')) {
                    *end = '\0';
                    end--;
                }
            }
        }
    }
}

// Cleanup conversation
void cleanup_conversation(conversation_t *conv) {
    if (!conv) return;
    
    // Free problem description
    if (conv->problem_description) {
        free(conv->problem_description);
        conv->problem_description = NULL;
    }
    
    // Free current solution
    if (conv->current_solution) {
        free(conv->current_solution);
        conv->current_solution = NULL;
    }
    
    // Free messages and their content
    if (conv->messages) {
        for (int i = 0; i < conv->message_count; i++) {
            if (conv->messages[i].content) {
                free(conv->messages[i].content);
                conv->messages[i].content = NULL;
            }
        }
        free(conv->messages);
        conv->messages = NULL;
    }
    
    // Free test result strings
    if (conv->last_test_result.error_message) {
        free(conv->last_test_result.error_message);
        conv->last_test_result.error_message = NULL;
    }
    
    if (conv->last_test_result.output) {
        free(conv->last_test_result.output);
        conv->last_test_result.output = NULL;
    }
    
    // Cleanup code evolution context
    cleanup_code_evolution(&conv->evolution);

    // Reset other fields
    conv->iterations = 0;
    conv->message_count = 0;
    conv->max_messages = 0;
    conv->config = NULL;
    
    // Note: This debug message can't use log_message since config is already NULL
    // Keep as printf for cleanup debugging if needed
    // printf("Conversation cleaned up.\n");
}

// Cleanup test result
void cleanup_test_result(test_result_t *test_result) {
    if (!test_result) return;
    
    if (test_result->error_message) {
        free(test_result->error_message);
        test_result->error_message = NULL;
    }
    
    if (test_result->output) {
        free(test_result->output);
        test_result->output = NULL;
    }
    
    test_result->syntax_ok = 0;
    test_result->compilation_ok = 0;
    test_result->execution_ok = 0;
}
