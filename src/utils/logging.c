#include "beta_evolve.h"

// Log a message based on verbosity level
void log_message(config_t *config, int level, const char* format, ...) {
    if (!config || !format || level > config->verbosity) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    // Choose appropriate prefix and color based on level
    const char* prefix = "";
    const char* color = C_RESET;
    
    switch (level) {
        case VERBOSITY_QUIET:
            // No prefix for quiet messages
            break;
        case VERBOSITY_NORMAL:
            color = C_INFO;
            break;
        case VERBOSITY_VERBOSE:
            prefix = "VERBOSE: ";
            color = C_CYAN;
            break;
        case VERBOSITY_DEBUG:
            prefix = "DEBUG: ";
            color = C_DEBUG;
            break;
    }
    
    printf("%s%s", color, prefix);
    vprintf(format, args);
    printf("%s", C_RESET);
    
    va_end(args);
}

// Log iteration start with appropriate detail level
void log_iteration_start(config_t *config, int iteration, int total_iterations) {
    if (!config) return;
    
    if (config->verbosity >= VERBOSITY_NORMAL) {
        if (iteration <= config->iterations) {
            printf("%s%s--- Iteration %d (Normal) ---%s\n", 
                   C_HEADER, C_BOLD, iteration, C_RESET);
        } else {
            printf("%s%s--- Error Fix Iteration %d (Extra) ---%s\n", 
                   C_WARNING, C_BOLD, total_iterations, C_RESET);
        }
    }
    
    if (config->verbosity >= VERBOSITY_VERBOSE) {
        print_progress_bar(iteration, config->iterations, "Progress");
    }
}

// Log code status with colored output
void log_code_status(config_t *config, const test_result_t* test_result) {
    if (!config || !test_result || config->verbosity < VERBOSITY_NORMAL) {
        return;
    }
    
    printf("%s📊 Code Status:%s ", C_EMPHASIS, C_RESET);
    
    // Syntax status
    printf("Syntax=%s%s%s, ", 
           test_result->syntax_ok ? C_SUCCESS : C_ERROR,
           test_result->syntax_ok ? "✅" : "❌",
           C_RESET);
    
    // Compilation status
    printf("Compilation=%s%s%s, ", 
           test_result->compilation_ok ? C_SUCCESS : C_ERROR,
           test_result->compilation_ok ? "✅" : "❌",
           C_RESET);
    
    // Execution status
    printf("Execution=%s%s%s\n", 
           test_result->execution_ok ? C_SUCCESS : C_ERROR,
           test_result->execution_ok ? "✅" : "❌",
           C_RESET);
    
    // Show additional details in verbose mode
    if (config->verbosity >= VERBOSITY_VERBOSE) {
        if (!test_result->syntax_ok || !test_result->compilation_ok || !test_result->execution_ok) {
            printf("%s🔧 Issues found - agents will focus on fixing these problems%s\n", C_WARNING, C_RESET);
            
            if (test_result->error_message && strlen(test_result->error_message) > 0) {
                int max_len = (config->verbosity >= VERBOSITY_DEBUG) ? 1000 : 200;
                printf("%s📋 Error details:%s %.200s%s\n", 
                       C_ERROR, C_RESET,
                                              test_result->error_message,
                       (int)strlen(test_result->error_message) > max_len ? "..." : "");
            }
        } else {
            printf("%s✅ Code is working correctly!%s\n", C_SUCCESS, C_RESET);
        }
        printf("\n");
    }
}

// Log AI interaction (only in debug mode)
void log_ai_interaction(config_t *config, agent_type_t agent, const char* prompt, const char* response) {
    if (!config || config->verbosity < VERBOSITY_DEBUG) {
        return;
    }
    
    const char* agent_name = (agent == AGENT_FAST) ? "Fast Agent" : "Reasoning Agent";
    const char* agent_color = (agent == AGENT_FAST) ? C_BRIGHT_BLUE : C_BRIGHT_MAGENTA;
    
    printf("%s%s=== %s Interaction ===%s\n", agent_color, C_BOLD, agent_name, C_RESET);
    
    if (prompt) {
        printf("%sPrompt:%s\n", C_EMPHASIS, C_RESET);
        print_code_block(prompt, "prompt");
    }
    
    if (response) {
        printf("%sResponse:%s\n", C_EMPHASIS, C_RESET);
        print_code_block(response, "response");
    }
    
    print_separator();
}

// Log error details with appropriate styling
void log_error_details(config_t *config, const char* error_message) {
    if (!config || !error_message || config->verbosity < VERBOSITY_NORMAL) {
        return;
    }
    
    printf("%s%sError Details:%s\n", C_ERROR, C_BOLD, C_RESET);
    printf("%s%s%s\n", C_ERROR, error_message, C_RESET);
    
    if (config->verbosity >= VERBOSITY_VERBOSE) {
        print_separator();
    }
}
