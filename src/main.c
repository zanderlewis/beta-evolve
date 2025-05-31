#include "beta_evolve.h"
#include "argparse.h"

// Run the dual-AI collaboration
int run_collaboration(const char *problem, config_t *config) {
    conversation_t conv;
    init_conversation(&conv, problem, config);
    
    // Initialize evolution if enabled
    if (config->enable_evolution && strlen(config->evolution_file_path) > 0) {
        log_message(config, VERBOSITY_NORMAL, "%s🧬 Evolution mode enabled for file: %s%s\n", 
                   C_INFO, config->evolution_file_path, C_RESET);
        
        // Read the initial evolution file
        char *file_content = read_evolution_file(config->evolution_file_path);
        if (file_content) {
            // Parse evolution regions
            parse_evolution_regions(&conv.evolution, file_content);
            
            if (conv.evolution.region_count > 0) {
                log_message(config, VERBOSITY_NORMAL, "%s🧬 Found %d evolution regions%s\n", 
                           C_INFO, conv.evolution.region_count, C_RESET);
                conv.evolution.evolution_enabled = 1;
                
                // Set the current solution to the file content
                if (strlen(file_content) < (unsigned long)config->max_code_size) {
                    strcpy(conv.current_solution, file_content);
                }
            }
            free(file_content);
        } else {
            log_message(config, VERBOSITY_NORMAL, "%s❌ Failed to read evolution file: %s%s\n", 
                       C_ERROR, config->evolution_file_path, C_RESET);
        }
    }
    
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
            
            // Run code evolution if evolution markers are detected
            if (conv.current_solution && strstr(conv.current_solution, EVOLUTION_MARKER_START)) {
                log_message(config, VERBOSITY_NORMAL, "%s🧬 Evolution markers detected - running code evolution...%s\n", 
                           C_INFO, C_RESET);
                evolve_code_regions(&conv, &conv.evolution);
                
                if (config->verbosity >= VERBOSITY_VERBOSE) {
                    log_message(config, VERBOSITY_VERBOSE, 
                               "%s🧬 Evolution status: %d regions, generation %d%s\n",
                               C_INFO, conv.evolution.region_count, 
                               conv.evolution.current_generation, C_RESET);
                }
            }
            
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
        snprintf(filename, sizeof(filename), "solution_%04d%02d%02d_%02d%02d%02d.c",
                tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        
        FILE *file = fopen(filename, "w");
        if (file) {
            fprintf(file, "%s\n", conv.current_solution);
            fclose(file);
            log_message(config, VERBOSITY_NORMAL, "%s💾 Solution saved to: %s%s\n", C_SUCCESS, filename, C_RESET);
            
            // Run final comprehensive evaluation if enabled
            if (config->enable_comprehensive_evaluation) {
                log_message(config, VERBOSITY_NORMAL, "%s🏁 Running final comprehensive evaluation...%s\n", C_INFO, C_RESET);
                
                evaluation_result_t final_eval = evaluate_code_comprehensive(
                    filename, conv.current_solution, &config->eval_criteria, config);
                
                // Display final evaluation summary
                printf("\n%s=== FINAL EVALUATION SUMMARY ===%s\n", C_HEADER, C_RESET);
                printf("%sOverall Score:%s %.1f/100\n", C_EMPHASIS, C_RESET, final_eval.overall_score);
                printf("%sCorrectness:%s %.1f/100\n", C_EMPHASIS, C_RESET, final_eval.correctness_score);
                printf("%sPerformance:%s %.1f/100\n", C_EMPHASIS, C_RESET, final_eval.performance_score);
                printf("%sCode Quality:%s %.1f/100\n", C_EMPHASIS, C_RESET, final_eval.quality_score);
                
                if (final_eval.passed_criteria) {
                    printf("%s✅ All evaluation criteria met!%s\n", C_SUCCESS, C_RESET);
                } else {
                    printf("%s⚠️  Some criteria not met - see evaluation report for details%s\n", C_WARNING, C_RESET);
                }
                
                // Save final evaluation report
                if (final_eval.detailed_report && strlen(config->evaluation_output_file) > 0) {
                    char final_report_path[1024];
                    snprintf(final_report_path, sizeof(final_report_path), "final_%s", config->evaluation_output_file);
                    
                    FILE *report_file = fopen(final_report_path, "w");
                    if (report_file) {
                        fprintf(report_file, "%s", final_eval.detailed_report);
                        fclose(report_file);
                        printf("%s📄 Final evaluation report saved to: %s%s\n", C_INFO, final_report_path, C_RESET);
                    }
                }
                
                // Show performance summary
                if (config->verbosity >= VERBOSITY_NORMAL) {
                    printf("\n%sPerformance Summary:%s\n", C_EMPHASIS, C_RESET);
                    printf("  Execution Time: %.2f ms\n", final_eval.performance.execution_time_ms);
                    printf("  Memory Usage: %ld KB\n", final_eval.performance.memory_usage_kb);
                    printf("  Throughput: %.1f ops/sec\n", final_eval.performance.throughput);
                    
                    printf("\n%sCode Quality Summary:%s\n", C_EMPHASIS, C_RESET);
                    printf("  Lines of Code: %d\n", final_eval.quality.lines_of_code);
                    printf("  Cyclomatic Complexity: %d\n", final_eval.quality.cyclomatic_complexity);
                    printf("  Test Coverage: %.1f%%\n", final_eval.quality.test_coverage_percent);
                    printf("  Maintainability Index: %.1f/100\n", final_eval.quality.maintainability_index);
                }
                
                cleanup_evaluation_result(&final_eval);
            }
        } else {
            log_message(config, VERBOSITY_NORMAL, "%s❌ Failed to save solution to file%s\n", C_ERROR, C_RESET);
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
    argparse_add_flag(parser, "debug", 'd', 
        "Enable debug output with detailed logs");
    argparse_add_flag(parser, "evaluation", 'e', 
        "Enable comprehensive evaluation with performance and quality analysis");
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
