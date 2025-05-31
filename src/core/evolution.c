#include "beta_evolve.h"
#include <regex.h>

// Initialize code evolution context
void init_code_evolution(code_evolution_t *evolution) {
    if (!evolution) return;
    
    memset(evolution, 0, sizeof(code_evolution_t));
    evolution->evolution_enabled = 0;
    evolution->current_generation = 0;
    evolution->total_generations = 0;
    evolution->base_code = NULL;
    evolution->region_count = 0;
}

// Cleanup code evolution context
void cleanup_code_evolution(code_evolution_t *evolution) {
    if (!evolution) return;
    
    // Free all region contents
    for (int i = 0; i < evolution->region_count; i++) {
        if (evolution->regions[i].content) {
            free(evolution->regions[i].content);
            evolution->regions[i].content = NULL;
        }
    }
    
    // Free base code
    if (evolution->base_code) {
        free(evolution->base_code);
        evolution->base_code = NULL;
    }
    
    evolution->region_count = 0;
    evolution->evolution_enabled = 0;
}

// Parse evolution regions from code using comment markers
int parse_evolution_regions(code_evolution_t *evolution, const char *code) {
    if (!evolution || !code) return -1;
    
    // Clear existing regions
    cleanup_code_evolution(evolution);
    init_code_evolution(evolution);
    
    // Check if code contains evolution markers
    if (!strstr(code, EVOLUTION_MARKER_START)) {
        evolution->evolution_enabled = 0;
        return 0; // No evolution regions found, not an error
    }
    
    evolution->evolution_enabled = 1;
    
    // Create a copy of the code to work with
    char *code_copy = strdup(code);
    if (!code_copy) return -1;
    
    // Split code into lines for easier processing
    char *lines[4096]; // Support up to 4096 lines
    int line_count = 0;
    char *line = strtok(code_copy, "\n");
    
    while (line && line_count < 4095) {
        lines[line_count] = strdup(line);
        line_count++;
        line = strtok(NULL, "\n");
    }
    
    // Parse evolution regions
    int region_idx = 0;
    int in_region = 0;
    int start_line = -1;
    char current_description[MAX_EVOLUTION_DESCRIPTION] = {0};
    dstring_t *current_content = NULL;
    
    for (int i = 0; i < line_count && region_idx < MAX_EVOLUTION_REGIONS; i++) {
        char *trimmed = lines[i];
        
        // Skip leading whitespace
        while (*trimmed && (*trimmed == ' ' || *trimmed == '\t')) {
            trimmed++;
        }
        
        if (strstr(trimmed, EVOLUTION_MARKER_START)) {
            if (in_region) {
                // Error: nested regions not allowed
                fprintf(stderr, "Warning: Nested evolution regions detected at line %d\n", i + 1);
                continue;
            }
            
            in_region = 1;
            start_line = i;
            
            // Extract description if present
            char *desc_start = strstr(trimmed, EVOLUTION_MARKER_START);
            desc_start += strlen(EVOLUTION_MARKER_START);
            
            // Skip whitespace and potential colon
            while (*desc_start && (*desc_start == ' ' || *desc_start == '\t' || *desc_start == ':')) {
                desc_start++;
            }
            
            if (*desc_start) {
                strncpy(current_description, desc_start, MAX_EVOLUTION_DESCRIPTION - 1);
                current_description[MAX_EVOLUTION_DESCRIPTION - 1] = '\0';
                
                // Remove trailing whitespace and comments
                char *end = current_description + strlen(current_description) - 1;
                while (end > current_description && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
                    *end = '\0';
                    end--;
                }
            } else {
                snprintf(current_description, sizeof(current_description), "region_%d", region_idx + 1);
            }
            
            current_content = dstring_create(1024);
            
        } else if (strstr(trimmed, EVOLUTION_MARKER_END)) {
            if (!in_region) {
                fprintf(stderr, "Warning: Evolution end marker without start at line %d\n", i + 1);
                continue;
            }
            
            // Save the current region
            if (current_content && region_idx < MAX_EVOLUTION_REGIONS) {
                evolution->regions[region_idx].content = strdup(dstring_get(current_content));
                strncpy(evolution->regions[region_idx].description, current_description, MAX_EVOLUTION_DESCRIPTION - 1);
                evolution->regions[region_idx].start_line = start_line;
                evolution->regions[region_idx].end_line = i;
                evolution->regions[region_idx].generation = 0;
                evolution->regions[region_idx].fitness_score = 0.0;
                
                region_idx++;
            }
            
            if (current_content) {
                dstring_destroy(current_content);
                current_content = NULL;
            }
            
            in_region = 0;
            start_line = -1;
            memset(current_description, 0, sizeof(current_description));
            
        } else if (in_region && current_content) {
            // Add line to current region content
            dstring_append(current_content, lines[i]);
            dstring_append(current_content, "\n");
        }
    }
    
    evolution->region_count = region_idx;
    
    // Clean up line copies
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(code_copy);
    
    if (current_content) {
        dstring_destroy(current_content);
    }
    
    return evolution->region_count;
}

// Assemble complete code from base code and evolved regions
char* assemble_evolved_code(const code_evolution_t *evolution, const char *original_code) {
    if (!evolution || !evolution->evolution_enabled || !original_code) {
        return original_code ? strdup(original_code) : NULL;
    }
    
    // If no regions to evolve, return original code
    if (evolution->region_count == 0) {
        return strdup(original_code);
    }
    
    dstring_t *assembled = dstring_create(strlen(original_code) * 2);
    if (!assembled) return NULL;
    
    // Split original code into lines for processing
    char *code_copy = strdup(original_code);
    char **lines = NULL;
    int line_count = 0;
    int capacity = 100;
    
    lines = malloc(capacity * sizeof(char*));
    if (!lines) {
        free(code_copy);
        dstring_destroy(assembled);
        return NULL;
    }
    
    // Parse lines
    char *line = strtok(code_copy, "\n");
    while (line) {
        if (line_count >= capacity) {
            capacity *= 2;
            lines = realloc(lines, capacity * sizeof(char*));
            if (!lines) {
                free(code_copy);
                dstring_destroy(assembled);
                return NULL;
            }
        }
        lines[line_count] = strdup(line);
        line_count++;
        line = strtok(NULL, "\n");
    }
    
    // Reconstruct the file, replacing evolved regions
    int i = 0;
    while (i < line_count) {
        char *current_line = lines[i];
        
        // Check if this line is an evolution region start
        if (strstr(current_line, EVOLUTION_MARKER_START)) {
            // Find corresponding region in evolution context
            int region_idx = -1;
            for (int r = 0; r < evolution->region_count; r++) {
                if (evolution->regions[r].start_line <= i && evolution->regions[r].end_line >= i) {
                    region_idx = r;
                    break;
                }
            }
            
            // Add the start marker with original description
            dstring_append_format(assembled, "%s\n", current_line);
            
            // Add evolved content if available
            if (region_idx >= 0 && evolution->regions[region_idx].content) {
                dstring_append(assembled, evolution->regions[region_idx].content);
                if (evolution->regions[region_idx].content[strlen(evolution->regions[region_idx].content)-1] != '\n') {
                    dstring_append(assembled, "\n");
                }
            } else {
                // Fall back to original content between markers
                i++; // Skip start marker
                while (i < line_count && !strstr(lines[i], EVOLUTION_MARKER_END)) {
                    dstring_append_format(assembled, "%s\n", lines[i]);
                    i++;
                }
            }
            
            // Find and add the end marker
            while (i < line_count && !strstr(lines[i], EVOLUTION_MARKER_END)) {
                i++;
            }
            if (i < line_count) {
                dstring_append_format(assembled, "%s\n", lines[i]);
            }
        } else {
            // Regular line, add as-is
            dstring_append_format(assembled, "%s\n", current_line);
        }
        i++;
    }
    
    // Clean up
    for (int j = 0; j < line_count; j++) {
        free(lines[j]);
    }
    free(lines);
    free(code_copy);
    
    char *result = strdup(dstring_get(assembled));
    dstring_destroy(assembled);
    
    return result;
}

// Extract content from a specific evolution region
int extract_evolution_region_content(const char *code, const char *region_desc, char **content) {
    if (!code || !region_desc || !content) return -1;
    
    *content = NULL;
    
    // Find the start marker with description
    char start_pattern[512];
    snprintf(start_pattern, sizeof(start_pattern), "%s: %s", EVOLUTION_MARKER_START, region_desc);
    
    char *start_pos = strstr(code, start_pattern);
    if (!start_pos) {
        // Try without colon
        snprintf(start_pattern, sizeof(start_pattern), "%s %s", EVOLUTION_MARKER_START, region_desc);
        start_pos = strstr(code, start_pattern);
    }
    
    if (!start_pos) return -1;
    
    // Find the end of the start line
    char *content_start = strchr(start_pos, '\n');
    if (!content_start) return -1;
    content_start++; // Skip the newline
    
    // Find the end marker
    char *end_pos = strstr(content_start, EVOLUTION_MARKER_END);
    if (!end_pos) return -1;
    
    // Calculate content length
    size_t content_length = end_pos - content_start;
    
    // Allocate and copy content
    *content = malloc(content_length + 1);
    if (!*content) return -1;
    
    strncpy(*content, content_start, content_length);
    (*content)[content_length] = '\0';
    
    // Remove trailing whitespace
    char *end = *content + strlen(*content) - 1;
    while (end > *content && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    
    return 0;
}

// Update a specific evolution region with new content
void update_evolution_region(code_evolution_t *evolution, const char *region_desc, const char *new_content) {
    if (!evolution || !region_desc || !new_content) return;
    
    // Find the region by description
    for (int i = 0; i < evolution->region_count; i++) {
        if (strcmp(evolution->regions[i].description, region_desc) == 0) {
            // Free old content
            if (evolution->regions[i].content) {
                free(evolution->regions[i].content);
            }
            
            // Set new content
            evolution->regions[i].content = strdup(new_content);
            evolution->regions[i].generation++;
            
            return;
        }
    }
    
    // If region not found, add it if there's space
    if (evolution->region_count < MAX_EVOLUTION_REGIONS) {
        int idx = evolution->region_count;
        evolution->regions[idx].content = strdup(new_content);
        strncpy(evolution->regions[idx].description, region_desc, MAX_EVOLUTION_DESCRIPTION - 1);
        evolution->regions[idx].generation = 1;
        evolution->regions[idx].fitness_score = 0.0;
        evolution->region_count++;
    }
}

// Evaluate fitness of evolved code
double evaluate_evolution_fitness(const char *file_path, config_t *config) {
    if (!file_path || !config) return 0.0;
    
    double fitness = 0.0;
    
    // Use custom test command if specified
    if (strlen(config->test_command) > 0) {
        test_result_t result = run_custom_test(config->test_command, file_path, config);
        
        // Base fitness components
        if (result.syntax_ok) fitness += 0.3;
        if (result.compilation_ok) fitness += 0.3;
        if (result.execution_ok) fitness += 0.4;
        
        cleanup_test_result(&result);
    } else {
        // Fallback to basic file existence check
        FILE *file = fopen(file_path, "r");
        if (file) {
            fitness = 0.1; // Basic file exists
            fclose(file);
        }
    }
    
    // Additional fitness factors could include:
    // - Performance metrics (timing tests)
    // - Code complexity analysis
    // - Memory usage profiling
    // - Test coverage analysis
    
    return fitness;
}

// Generate evolution-specific prompt for AI agents
char* generate_evolution_prompt(const conversation_t *conv, const code_evolution_t *evolution, agent_type_t agent) {
    if (!conv || !conv->config || !evolution) return NULL;
    
    if (!evolution->evolution_enabled) {
        // Fall back to regular prompt generation
        return generate_agent_prompt(conv, agent);
    }
    
    dstring_t* prompt = dstring_create(conv->config->max_prompt_size * 2);
    if (!prompt) return NULL;
    
    // Add evolution-specific instructions
    dstring_append(prompt, 
        "You are working with Beta Evolve's code evolution system, similar to Google DeepMind's Alpha Evolve.\n"
        "The code contains evolution regions marked with special comments:\n\n"
        "// BETA EVOLVE START: <description>\n"
        "// ... evolvable code here ...\n"
        "// BETA EVOLVE END\n\n"
        "Your task is to evolve the code within these marked regions to improve performance, "
        "correctness, and efficiency. You are also responsible for ensuring the code has all "
        "necessary #include headers to compile properly.\n\n");
    
    // Add current evolution status
    if (evolution->region_count > 0) {
        dstring_append_format(prompt, "CURRENT EVOLUTION REGIONS (%d):\n", evolution->region_count);
        for (int i = 0; i < evolution->region_count; i++) {
            dstring_append_format(prompt, "- %s (generation %d, fitness: %.2f)\n",
                                 evolution->regions[i].description,
                                 evolution->regions[i].generation,
                                 evolution->regions[i].fitness_score);
        }
        dstring_append(prompt, "\n");
    }
    
    // Add agent-specific role
    if (agent == AGENT_FAST) {
        dstring_append(prompt,
            "As the FAST EVOLUTION AGENT:\n"
            "1. Focus on rapid exploration of new algorithmic approaches\n"
            "2. Try creative variations within evolution regions\n"
            "3. Add necessary #include headers for any new functions you use\n"
            "4. Maintain compatibility with non-evolvable code sections\n"
            "5. Ensure your changes compile and run\n\n");
    } else {
        dstring_append(prompt,
            "As the REASONING EVOLUTION AGENT:\n"
            "1. Analyze the current evolution regions for optimization opportunities\n"
            "2. Improve algorithmic efficiency and correctness\n"
            "3. Add error handling and edge case coverage\n"
            "4. Include any missing #include headers needed for your implementations\n"
            "5. Evaluate and refine the evolutionary changes\n\n");
    }
    
    // Add current problem and code context
    const char *problem_desc = conv->problem_description ? conv->problem_description : "No problem description";
    const char *current_code = (conv->current_solution && strlen(conv->current_solution) > 0) ? 
                               conv->current_solution : "None";
    const char *errors = (conv->last_test_result.error_message && strlen(conv->last_test_result.error_message) > 0) ? 
                         conv->last_test_result.error_message : "None";
    
    dstring_append_format(prompt, "PROBLEM: %s\n\nCURRENT CODE: %s\n\nERRORS TO FIX: %s\n\n", 
                         problem_desc, current_code, errors);
    
    // Add evolution-specific instructions
    dstring_append(prompt,
        "EVOLUTION INSTRUCTIONS:\n"
        "1. Only modify code within BETA EVOLVE START/END markers, unless when adding headers\n"
        "2. Preserve the marker comments and descriptions exactly\n"
        "3. IMPORTANT: Add any necessary #include headers at the top of the file when needed\n"
        "   - Always include <stdio.h> for printf functions\n"
        "   - Add <stdlib.h> for malloc, free, and other standard library functions\n"
        "   - Add <string.h> for string manipulation functions\n"
        "   - Add <time.h> for time-related functions\n"
        "   - Add any other headers your evolved code requires\n"
        "4. Focus on algorithmic improvements while ensuring code compiles\n"
        "5. Test your changes thoroughly\n"
        "6. Provide analysis of what you evolved and why\n\n"
        
        "RESPONSE FORMAT:\n"
        "Evolution Analysis: [Brief description of what you evolved and the expected improvements]\n\n"
        "```c\n"
        "// Your complete evolved code with all necessary headers and regions\n"
        "```\n\n"
        
        "Your response:");
    
    char* result = strdup(dstring_get(prompt));
    dstring_destroy(prompt);
    
    return result;
}

// Main evolution cycle function
void evolve_code_regions(conversation_t *conv, code_evolution_t *evolution) {
    if (!conv || !evolution || !evolution->evolution_enabled) return;
    if (!conv->config->enable_evolution || strlen(conv->config->evolution_file_path) == 0) return;
    
    // Parse current solution for evolution regions
    if (conv->current_solution && strlen(conv->current_solution) > 0) {
        parse_evolution_regions(evolution, conv->current_solution);
    }
    
    // Use custom test command if specified
    if (strlen(conv->config->test_command) > 0) {
        log_message(conv->config, VERBOSITY_VERBOSE, "%s🧬 Running evolution test with custom command...%s\n", C_INFO, C_RESET);
        
        // Write current solution to evolution file for testing
        if (write_evolution_file(conv->config->evolution_file_path, conv->current_solution) == 0) {
            // Create evolved file path for testing
            char evolved_file_path[1024];
            snprintf(evolved_file_path, sizeof(evolved_file_path), "%s.evolved", conv->config->evolution_file_path);
            
            // Run custom test command with evolved file
            test_result_t test_result = run_custom_test(conv->config->test_command, 
                                                       evolved_file_path, 
                                                       conv->config);
            
            // Update conversation test result for consistency
            cleanup_test_result(&conv->last_test_result);
            conv->last_test_result = test_result;
            
            // Evaluate fitness based on test results
            double current_fitness = 0.0;
            if (test_result.syntax_ok) current_fitness += 0.3;
            if (test_result.compilation_ok) current_fitness += 0.3;
            if (test_result.execution_ok) current_fitness += 0.4;
            
            // Update fitness scores for all regions
            for (int i = 0; i < evolution->region_count; i++) {
                evolution->regions[i].fitness_score = current_fitness;
            }
            
            log_message(conv->config, VERBOSITY_VERBOSE, 
                       "%s🧬 Evolution fitness: %.2f (generation %d)%s\n", 
                       C_INFO, current_fitness, evolution->current_generation, C_RESET);
            
            if (current_fitness >= 1.0) {
                log_message(conv->config, VERBOSITY_NORMAL, "%s🧬 Evolution target achieved! Code passes all tests.%s\n", C_SUCCESS, C_RESET);
            } else if (current_fitness >= 0.6) {
                log_message(conv->config, VERBOSITY_NORMAL, "%s🧬 Evolution making good progress (fitness: %.2f)%s\n", C_INFO, current_fitness, C_RESET);
            } else {
                log_message(conv->config, VERBOSITY_NORMAL, "%s🧬 Evolution needs improvement (fitness: %.2f)%s\n", C_WARNING, current_fitness, C_RESET);
            }
            
            // Show test output if available and verbosity is high enough
            if (conv->config->verbosity >= VERBOSITY_VERBOSE && test_result.output && strlen(test_result.output) > 0) {
                log_message(conv->config, VERBOSITY_VERBOSE, "%sTest Output:%s\n%s\n", C_INFO, C_RESET, test_result.output);
            }
            
            // Show errors if any
            if (test_result.error_message && strlen(test_result.error_message) > 0) {
                log_message(conv->config, VERBOSITY_NORMAL, "%sEvolution Test Errors:%s %s\n", C_ERROR, C_RESET, test_result.error_message);
            }
        } else {
            log_message(conv->config, VERBOSITY_NORMAL, "%s❌ Failed to write evolved code to file: %s%s\n", 
                       C_ERROR, conv->config->evolution_file_path, C_RESET);
        }
    } else {
        // Fallback to basic fitness evaluation without custom testing
        double current_fitness = evaluate_evolution_fitness(conv->config->evolution_file_path, conv->config);
        
        // Update fitness scores for all regions
        for (int i = 0; i < evolution->region_count; i++) {
            evolution->regions[i].fitness_score = current_fitness;
        }
        
        log_message(conv->config, VERBOSITY_VERBOSE, 
                   "%s🧬 Evolution fitness (basic): %.2f (generation %d)%s\n", 
                   C_INFO, current_fitness, evolution->current_generation, C_RESET);
    }
    
    // Comprehensive evaluation if enabled
    if (conv->config->enable_comprehensive_evaluation && conv->current_solution && 
        strlen(conv->current_solution) > 0) {
        
        log_message(conv->config, VERBOSITY_NORMAL, "%s📊 Running comprehensive evaluation...%s\n", C_INFO, C_RESET);
        
        char evolved_file_path[1024];
        snprintf(evolved_file_path, sizeof(evolved_file_path), "%s.evolved", conv->config->evolution_file_path);
        
        evaluation_result_t eval_result = evaluate_code_comprehensive(
            evolved_file_path, conv->current_solution, &conv->config->eval_criteria, conv->config);
        
        // Update fitness score based on comprehensive evaluation
        for (int i = 0; i < evolution->region_count; i++) {
            evolution->regions[i].fitness_score = eval_result.overall_score / 100.0;
        }
        
        // Log evaluation results
        log_message(conv->config, VERBOSITY_NORMAL, 
                   "%s📊 Comprehensive Score: %.1f/100 (Correctness: %.1f, Performance: %.1f, Quality: %.1f)%s\n",
                   C_INFO, eval_result.overall_score, eval_result.correctness_score, 
                   eval_result.performance_score, eval_result.quality_score, C_RESET);
        
        if (eval_result.passed_criteria) {
            log_message(conv->config, VERBOSITY_NORMAL, "%s✅ All evaluation criteria met!%s\n", C_SUCCESS, C_RESET);
        } else {
            log_message(conv->config, VERBOSITY_NORMAL, "%s⚠️  Some criteria not met%s\n", C_WARNING, C_RESET);
        }
        
        // Show detailed metrics in verbose mode
        if (conv->config->verbosity >= VERBOSITY_VERBOSE) {
            log_message(conv->config, VERBOSITY_VERBOSE, 
                       "%sPerformance: %.2fms execution, %ldKB memory, %.1f ops/sec%s\n",
                       C_INFO, eval_result.performance.execution_time_ms, 
                       eval_result.performance.memory_usage_kb, eval_result.performance.throughput, C_RESET);
            
            log_message(conv->config, VERBOSITY_VERBOSE, 
                       "%sCode Quality: %d complexity, %.1f%% coverage, %.1f maintainability%s\n",
                       C_INFO, eval_result.quality.cyclomatic_complexity, 
                       eval_result.quality.test_coverage_percent, 
                       eval_result.quality.maintainability_index, C_RESET);
        }
        
        // Save evaluation to history
        if (conv->config->save_evaluation_history) {
            save_evaluation_history(evolution, &eval_result);
        }
        
        // Save detailed report to file
        if (eval_result.detailed_report && strlen(conv->config->evaluation_output_file) > 0) {
            FILE *report_file = fopen(conv->config->evaluation_output_file, "w");
            if (report_file) {
                fprintf(report_file, "%s", eval_result.detailed_report);
                fclose(report_file);
                log_message(conv->config, VERBOSITY_NORMAL, 
                           "%s📄 Evaluation report saved to: %s%s\n", 
                           C_INFO, conv->config->evaluation_output_file, C_RESET);
            }
        }
        
        // Show recommendations in debug mode
        if (conv->config->verbosity >= VERBOSITY_DEBUG && eval_result.recommendations) {
            log_message(conv->config, VERBOSITY_DEBUG, 
                       "%sRecommendations:\n%s%s\n", C_INFO, eval_result.recommendations, C_RESET);
        }
        
        // Compare with previous evaluation if available
        if (evolution->evaluation_count > 1) {
            evaluation_result_t *prev_eval = &evolution->evaluation_history[evolution->evaluation_count - 2];
            char *comparison_report = NULL;
            compare_evaluations(&eval_result, prev_eval, &comparison_report);
            
            if (comparison_report && conv->config->verbosity >= VERBOSITY_VERBOSE) {
                log_message(conv->config, VERBOSITY_VERBOSE, 
                           "%sEvolution Progress:\n%s%s\n", C_INFO, comparison_report, C_RESET);
                free(comparison_report);
            }
        }
        
        cleanup_evaluation_result(&eval_result);
    }
}

// File I/O functions for evolution system

// Read evolution file content
char* read_evolution_file(const char *file_path) {
    if (!file_path || strlen(file_path) == 0) {
        return NULL;
    }
    
    FILE *file = fopen(file_path, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open evolution file: %s\n", file_path);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        return NULL;
    }
    
    // Allocate buffer and read content
    char *content = malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(content, 1, file_size, file);
    content[bytes_read] = '\0';
    fclose(file);
    
    return content;
}

// Write evolved code to file
int write_evolution_file(const char *file_path, const char *content) {
    if (!file_path || !content) {
        return -1;
    }
    
    // Create evolved file path
    char evolved_file_path[1024];
    snprintf(evolved_file_path, sizeof(evolved_file_path), "%s.evolved", file_path);
    
    FILE *file = fopen(evolved_file_path, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot create evolved file: %s\n", evolved_file_path);
        return -1;
    }
    
    if (fprintf(file, "%s", content) < 0) {
        fclose(file);
        return -1;
    }
    
    fclose(file);
    return 0;
}

// Run custom test command on a file
test_result_t run_custom_test(const char *test_command, const char *file_path, config_t *config) {
    test_result_t result = {0};
    
    // Allocate memory for result strings
    result.error_message = malloc(config->max_response_size);
    result.output = malloc(config->max_response_size);
    
    if (!result.error_message || !result.output) {
        if (result.error_message) free(result.error_message);
        if (result.output) free(result.output);
        result.error_message = NULL;
        result.output = NULL;
        return result;
    }
    
    // Initialize strings
    memset(result.error_message, 0, config->max_response_size);
    memset(result.output, 0, config->max_response_size);
    
    if (!test_command || !file_path || strlen(test_command) == 0) {
        snprintf(result.error_message, config->max_response_size, 
                "Invalid test command or file path");
        return result;
    }
    
    // Replace {file} placeholder with actual file path
    char expanded_command[2048];
    char *placeholder = strstr(test_command, "{file}");
    
    if (placeholder) {
        // Calculate lengths for safe string manipulation
        size_t prefix_len = placeholder - test_command;
        size_t suffix_len = strlen(placeholder + 6); // 6 = strlen("{file}")
        
        // Check if the expanded command will fit
        if (prefix_len + strlen(file_path) + suffix_len >= sizeof(expanded_command)) {
            snprintf(result.error_message, config->max_response_size, 
                    "Test command too long after expansion");
            return result;
        }
        
        // Build expanded command
        strncpy(expanded_command, test_command, prefix_len);
        expanded_command[prefix_len] = '\0';
        strcat(expanded_command, file_path);
        strcat(expanded_command, placeholder + 6);
    } else {
        // No placeholder, use command as-is with file path appended
        snprintf(expanded_command, sizeof(expanded_command), "%s %s", test_command, file_path);
    }
    
    // Execute the test command and capture output
    FILE *pipe = popen(expanded_command, "r");
    if (!pipe) {
        snprintf(result.error_message, config->max_response_size, 
                "Failed to execute test command: %s", expanded_command);
        return result;
    }
    
    // Read command output
    size_t total_read = 0;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) && total_read < (size_t)config->max_response_size - 1) {
        size_t len = strlen(buffer);
        if (total_read + len < (size_t)config->max_response_size - 1) {
            strcpy(result.output + total_read, buffer);
            total_read += len;
        } else {
            break;
        }
    }
    result.output[total_read] = '\0';
    
    // Get exit code
    int exit_code = pclose(pipe);
    int actual_exit_code = WEXITSTATUS(exit_code);
    
    // Determine test results based on exit code
    if (actual_exit_code == 0) {
        result.syntax_ok = 1;
        result.compilation_ok = 1;
        result.execution_ok = 1;
    } else {
        // Parse output to determine what failed
        const char *output_lower = result.output;
        
        // Check for common compilation errors
        if (strstr(output_lower, "error:") || strstr(output_lower, "undefined reference") ||
            strstr(output_lower, "fatal error") || strstr(output_lower, "cannot find")) {
            result.syntax_ok = 0;
            result.compilation_ok = 0;
            result.execution_ok = 0;
            snprintf(result.error_message, config->max_response_size, 
                    "Compilation failed: %s", result.output);
        } else if (strstr(output_lower, "segmentation fault") || strstr(output_lower, "abort") ||
                   strstr(output_lower, "core dumped")) {
            result.syntax_ok = 1;
            result.compilation_ok = 1;
            result.execution_ok = 0;
            snprintf(result.error_message, config->max_response_size, 
                    "Runtime error: %s", result.output);
        } else {
            // Assume compilation succeeded but test failed
            result.syntax_ok = 1;
            result.compilation_ok = 1;
            result.execution_ok = 0;
            snprintf(result.error_message, config->max_response_size, 
                    "Test failed (exit code %d): %s", actual_exit_code, result.output);
        }
    }
    
    return result;
}
