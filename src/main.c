#include "beta_evolve.h"
#include "argparse.h"

// Run the dual-AI collaboration
int run_collaboration(const char *problem, config_t *config) {
    conversation_t conv;
    init_conversation(&conv, problem, config);
    
    print_header("Beta Evolve: Starting dual-AI collaboration");
    log_message(config, VERBOSITY_NORMAL, "%sProblem:%s %s\n\n", C_EMPHASIS, C_RESET, problem);
    
    int max_error_iterations = config->iterations * 3; // Allow up to 3x normal iterations for error fixing
    int total_iterations = 0;
    
    for (int iteration = 0; iteration < config->iterations || has_code_errors(&conv.last_test_result); iteration++) {
        // Safety check to prevent infinite loops
        if (total_iterations >= max_error_iterations) {
            log_message(config, VERBOSITY_NORMAL, 
                       "%s🛑 Maximum iterations reached (%d). Stopping to prevent infinite loop.%s\n", 
                       C_WARNING, max_error_iterations, C_RESET);
            break;
        }
        
        conv.iterations = iteration + 1;
        total_iterations++;
        
        // Log iteration start with appropriate verbosity
        log_iteration_start(config, conv.iterations, total_iterations);
        
        // Show current error status if we have a solution
        if (strlen(conv.current_solution) > 0) {
            log_code_status(config, &conv.last_test_result);
        }
        
        // Check if this is a normal iteration or error fix iteration
        if (iteration < config->iterations) {
            // Normal iteration: Use both fast and reasoning agents
            
            // Fast Agent turn
            log_message(config, VERBOSITY_NORMAL, "%s🏃 Fast Agent thinking...%s\n", C_INFO, C_RESET);
            char *fast_prompt = generate_agent_prompt(&conv, AGENT_FAST);
            if (!fast_prompt) {
                log_message(config, VERBOSITY_NORMAL, "%sError: Failed to generate fast agent prompt%s\n", C_ERROR, C_RESET);
                return -1;
            }
            
            char *fast_response = call_ai_model(fast_prompt, AGENT_FAST, config);
            
            // Log the AI interaction in debug mode
            log_ai_interaction(config, AGENT_FAST, fast_prompt, fast_response);
            free(fast_prompt);
            
            if (!fast_response) {
                log_message(config, VERBOSITY_NORMAL, "%sError: Fast agent failed to respond%s\n", C_ERROR, C_RESET);
                return -1;
            }
            
            // Validate and clean the response
            char *cleaned_fast_response = validate_and_clean_response(fast_response);
            free(fast_response);
            
            if (!cleaned_fast_response) {
                log_message(config, VERBOSITY_NORMAL, "%sError: Failed to validate fast agent response%s\n", C_ERROR, C_RESET);
                return -1;
            }
            
            add_message(&conv, AGENT_FAST, cleaned_fast_response);
            
            // Extract and show analysis if present
            const char *analysis = strstr(cleaned_fast_response, "Analysis:");
            if (analysis && config->verbosity >= VERBOSITY_VERBOSE) {
                const char *analysis_end = strstr(analysis, "\n\n```");
                if (analysis_end) {
                    int analysis_len = analysis_end - analysis;
                    log_message(config, VERBOSITY_VERBOSE, "%sFast Agent Analysis:%s %.*s\n", 
                               C_INFO, C_RESET, analysis_len, analysis);
                }
            }
            log_message(config, VERBOSITY_NORMAL, "%s🏃 Fast Agent provided code solution%s\n\n", C_SUCCESS, C_RESET);
            
            // Reasoning Agent turn
            log_message(config, VERBOSITY_NORMAL, "%s🧠 Reasoning Agent analyzing...%s\n", C_INFO, C_RESET);
            char *reasoning_prompt = generate_agent_prompt(&conv, AGENT_REASONING);
            if (!reasoning_prompt) {
                log_message(config, VERBOSITY_NORMAL, "%sError: Failed to generate reasoning agent prompt%s\n", C_ERROR, C_RESET);
                free(cleaned_fast_response);
                return -1;
            }
            
            char *reasoning_response = call_ai_model(reasoning_prompt, AGENT_REASONING, config);
            
            // Log the AI interaction in debug mode
            log_ai_interaction(config, AGENT_REASONING, reasoning_prompt, reasoning_response);
            free(reasoning_prompt);
            
            if (!reasoning_response) {
                log_message(config, VERBOSITY_NORMAL, "%sError: Reasoning agent failed to respond%s\n", C_ERROR, C_RESET);
                free(cleaned_fast_response);
                return -1;
            }
            
            // Validate and clean the response
            char *cleaned_reasoning_response = validate_and_clean_response(reasoning_response);
            free(reasoning_response);
            
            if (!cleaned_reasoning_response) {
                log_message(config, VERBOSITY_NORMAL, "%sError: Failed to validate reasoning agent response%s\n", C_ERROR, C_RESET);
                free(cleaned_fast_response);
                return -1;
            }
            
            add_message(&conv, AGENT_REASONING, cleaned_reasoning_response);
            
            // Extract and show analysis if present
            const char *reasoning_analysis = strstr(cleaned_reasoning_response, "Analysis:");
            if (reasoning_analysis && config->verbosity >= VERBOSITY_VERBOSE) {
                const char *analysis_end = strstr(reasoning_analysis, "\n\n```");
                if (analysis_end) {
                    int analysis_len = analysis_end - reasoning_analysis;
                    log_message(config, VERBOSITY_VERBOSE, "%sReasoning Agent Analysis:%s %.*s\n", 
                               C_INFO, C_RESET, analysis_len, reasoning_analysis);
                }
            }
            log_message(config, VERBOSITY_NORMAL, "%s🧠 Reasoning Agent provided refined solution%s\n\n", C_SUCCESS, C_RESET);
            
            // Update solution with testing
            update_solution_with_testing(&conv, cleaned_reasoning_response);
            
            // Cleanup
            free(cleaned_fast_response);
            free(cleaned_reasoning_response);
        } else {
            // Error fix iteration: Only use reasoning agent for bug fixes
            log_message(config, VERBOSITY_NORMAL, "%s🧠 Reasoning Agent fixing bugs...%s\n", C_INFO, C_RESET);
            char *reasoning_prompt = generate_agent_prompt(&conv, AGENT_REASONING);
            if (!reasoning_prompt) {
                log_message(config, VERBOSITY_NORMAL, "%sError: Failed to generate reasoning agent prompt%s\n", C_ERROR, C_RESET);
                return -1;
            }
            
            char *reasoning_response = call_ai_model(reasoning_prompt, AGENT_REASONING, config);
            
            // Log the AI interaction in debug mode
            log_ai_interaction(config, AGENT_REASONING, reasoning_prompt, reasoning_response);
            free(reasoning_prompt);
            
            if (!reasoning_response) {
                log_message(config, VERBOSITY_NORMAL, "%sError: Reasoning agent failed to respond%s\n", C_ERROR, C_RESET);
                return -1;
            }
            
            // Validate and clean the response
            char *cleaned_reasoning_response = validate_and_clean_response(reasoning_response);
            free(reasoning_response);
            
            if (!cleaned_reasoning_response) {
                log_message(config, VERBOSITY_NORMAL, "%sError: Failed to validate reasoning agent response%s\n", C_ERROR, C_RESET);
                return -1;
            }
            
            add_message(&conv, AGENT_REASONING, cleaned_reasoning_response);
            
            // Extract and show analysis if present
            const char *reasoning_analysis = strstr(cleaned_reasoning_response, "Analysis:");
            if (reasoning_analysis && config->verbosity >= VERBOSITY_VERBOSE) {
                const char *analysis_end = strstr(reasoning_analysis, "\n\n```");
                if (analysis_end) {
                    int analysis_len = analysis_end - reasoning_analysis;
                    log_message(config, VERBOSITY_VERBOSE, "%sReasoning Agent Analysis:%s %.*s\n", 
                               C_INFO, C_RESET, analysis_len, reasoning_analysis);
                }
            }
            log_message(config, VERBOSITY_NORMAL, "%s🧠 Reasoning Agent provided bug fix%s\n\n", C_SUCCESS, C_RESET);
            
            // Update solution with testing
            update_solution_with_testing(&conv, cleaned_reasoning_response);
            
            // Cleanup
            free(cleaned_reasoning_response);
        }
        
        // Show progress
        if (strlen(conv.current_solution) > 0) {
            log_message(config, VERBOSITY_NORMAL, "%s💡 Current solution updated!%s\n", C_SUCCESS, C_RESET);
            
            // Check if we've resolved all errors
            if (!has_code_errors(&conv.last_test_result)) {
                log_message(config, VERBOSITY_NORMAL, "%s🎉 All code errors resolved! Code compiles and runs successfully.%s\n", C_SUCCESS, C_RESET);
                if (iteration >= config->iterations) {
                    log_message(config, VERBOSITY_NORMAL, "%s✅ Error fixing phase completed.%s\n", C_SUCCESS, C_RESET);
                    break;
                }
            }
            printf("\n");
        }
        
        // Add delay between iterations
        sleep(1);
    }
    
    // Print final conversation and solution
    print_conversation(&conv);
    
    // Save solution to file
    if (strlen(conv.current_solution) > 0) {
        char filename[256];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(filename, sizeof(filename), "solution_%Y%m%d_%H%M%S.c", tm_info);
        
        FILE *solution_file = fopen(filename, "w");
        if (solution_file) {
            fprintf(solution_file, "/*\n * Beta Evolve Generated Solution\n * Problem: %s\n */\n\n", problem);
            fprintf(solution_file, "%s\n", conv.current_solution);
            fclose(solution_file);
            log_message(config, VERBOSITY_NORMAL, "%s💾 Solution saved to: %s%s\n", C_SUCCESS, filename, C_RESET);
        }
    }
    
    cleanup_conversation(&conv);
    return 0;
}

int main(int argc, char *argv[]) {
    // Create argument parser
    argparser_t *parser = argparse_create("beta-evolve", 
        "Beta Evolve v1.0 - Dual-AI Coding Collaboration System");
    
    // Add command line arguments
    argparse_add_string(parser, "problem", 'p', 
        "Problem description to solve", false, NULL);
    argparse_add_string(parser, "prompt-file", 'f', 
        "File containing problem description", false, NULL);
    argparse_add_string(parser, "config", 'c', 
        "Configuration file to use", false, "config.toml");
    argparse_add_int(parser, "iterations", 'i', 
        "Number of collaboration iterations", false, 10);
    argparse_add_flag(parser, "verbose", 'v', 
        "Enable verbose output");
    argparse_add_flag(parser, "help", 'h', 
        "Show this help message");
    
    // Parse arguments
    if (!argparse_parse(parser, argc, argv)) {
        argparse_print_usage(parser);
        argparse_destroy(parser);
        return 1;
    }
    
    // Handle help request
    if (parser->help_requested || argparse_is_set(parser, "help")) {
        argparse_print_help(parser);
        argparse_destroy(parser);
        return 0;
    }
    
    // Get parsed arguments
    const char *problem_description = argparse_get_string(parser, "problem");
    const char *prompt_file_override = argparse_get_string(parser, "prompt-file");
    const char *config_file = argparse_get_string(parser, "config");
    int iterations_override = argparse_get_int(parser, "iterations");
    bool verbose = argparse_get_bool(parser, "verbose");
    
    // Handle positional arguments (backward compatibility)
    if (!problem_description && argparse_get_positional_count(parser) > 0) {
        problem_description = argparse_get_positional(parser, 0);
    }
    
    // Print verbose information
    if (verbose) {
        printf("🔧 Beta Evolve v1.0 - Debug Information\n");
        printf("Config file: %s\n", config_file);
        printf("Problem: %s\n", problem_description ? problem_description : "(from file)");
        printf("Prompt file: %s\n", prompt_file_override ? prompt_file_override : "(default)");
        printf("Iterations override: %d\n", iterations_override);
        printf("Verbose mode: %s\n\n", verbose ? "enabled" : "disabled");
    }

    // Delete any existing log file
    remove("beta-evolve.log");
    
    // Load configuration
    config_t config;
    if (load_config(&config, config_file) != 0) {
        fprintf(stderr, "Error: Failed to load configuration from '%s'. Please create the config file.\n", config_file);
        argparse_destroy(parser);
        return 1;
    }
    
    // Initialize colors based on config
    if (config.use_colors) {
        colors_enable();
    } else {
        colors_disable();
    }
    colors_init();
    
    // Override iterations if specified
    if (iterations_override > 0) {
        config.iterations = iterations_override;
        if (verbose) {
            printf("Overriding iterations to: %d\n", iterations_override);
        }
    }
    
    // Override prompt file if specified via command line
    if (prompt_file_override && strlen(prompt_file_override) > 0) {
        if (load_problem_prompt_file(&config, prompt_file_override) != 0) {
            fprintf(stderr, "Error: Failed to load prompt file '%s'\n", prompt_file_override);
            free_config(&config);
            argparse_destroy(parser);
            return 1;
        }
        if (verbose) {
            printf("Info: Using command line prompt file: %s\n", prompt_file_override);
        }
    }
    
    // Determine final problem description
    const char *final_problem = NULL;
    if (problem_description && strlen(problem_description) > 0) {
        // Command line problem takes priority
        final_problem = problem_description;
        if (verbose) {
            printf("Info: Using command line problem description\n");
        }
    } else if (config.loaded_problem_prompt) {
        // Use loaded prompt file
        final_problem = config.loaded_problem_prompt;
        if (verbose) {
            const char *source_file = (prompt_file_override && strlen(prompt_file_override) > 0) 
                                     ? prompt_file_override 
                                     : config.problem_prompt_file;
            printf("Info: Using problem description from file: %s\n", source_file);
        }
    } else {
        fprintf(stderr, "Error: No problem description provided.\n");
        fprintf(stderr, "Either provide it using --problem/-p or specify a prompt file with --prompt-file/-f\n");
        fprintf(stderr, "Or set problem_prompt_file in the config file.\n");
        free_config(&config);
        argparse_destroy(parser);
        return 1;
    }
    
    // Validate configuration - allow null API keys
    if ((strlen(config.fast_model_api_key) == 0 || strcmp(config.fast_model_api_key, "null") == 0) &&
        (strlen(config.reasoning_model_api_key) == 0 || strcmp(config.reasoning_model_api_key, "null") == 0)) {
        log_message(&config, VERBOSITY_NORMAL, "%sInfo: No API keys configured - using servers that don't require authentication%s\n", C_INFO, C_RESET);
    }
    
    print_header("Beta Evolve v1.0 - Dual-AI Coding Collaboration");
    log_message(&config, VERBOSITY_NORMAL, "%sFast Model:%s %s\n", C_EMPHASIS, C_RESET,
           (strcmp(config.fast_model_name, "null") == 0) ? "Server Default" : config.fast_model_name);
    log_message(&config, VERBOSITY_NORMAL, "%sReasoning Model:%s %s\n", C_EMPHASIS, C_RESET,
           (strcmp(config.reasoning_model_name, "null") == 0) ? "Server Default" : config.reasoning_model_name);
    log_message(&config, VERBOSITY_NORMAL, "%sIteration Count:%s %d\n\n", C_EMPHASIS, C_RESET, config.iterations);
    
    // Show problem description
    log_message(&config, VERBOSITY_NORMAL, "%s🎯 Problem:%s %s\n\n", C_EMPHASIS, C_RESET, final_problem);
    
    // Run collaboration
    int result = run_collaboration(final_problem, &config);
    
    // Cleanup
    free_config(&config);
    argparse_destroy(parser);
    
    if (result == 0) {
        log_message(&config, VERBOSITY_NORMAL, "%s✅ Beta Evolve collaboration completed successfully!%s\n", C_SUCCESS, C_RESET);
    } else {
        log_message(&config, VERBOSITY_NORMAL, "%s❌ Beta Evolve collaboration failed.%s\n", C_ERROR, C_RESET);
    }
    
    return result;
}
