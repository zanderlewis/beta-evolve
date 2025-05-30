#include "beta_evolve.h"

// Forward declarations for evolution functions
extern int write_evolution_file(const char *file_path, const char *content);
extern test_result_t run_custom_test(const char *test_command, const char *file_path, config_t *config);

// Execute a shell command and capture output
int execute_command(const char* command, char* output, size_t output_size) {
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        snprintf(output, output_size, "Failed to execute command: %s", command);
        return -1;
    }
    
    size_t total_read = 0;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) && total_read < output_size - 1) {
        size_t len = strlen(buffer);
        if (total_read + len < output_size - 1) {
            strcpy(output + total_read, buffer);
            total_read += len;
        } else {
            break;
        }
    }
    output[total_read] = '\0';
    
    int exit_code = pclose(pipe);
    return WEXITSTATUS(exit_code);
}

// Test generated C code for compilation and basic syntax
test_result_t test_generated_code(const char* code_content, const char* problem_description, config_t *config) {
    test_result_t result = {0};
    
    // Allocate dynamic memory for error message and output
    result.error_message = malloc(config->max_response_size);
    result.output = malloc(config->max_response_size);
    
    if (!result.error_message || !result.output) {
        if (result.error_message) free(result.error_message);
        if (result.output) free(result.output);
        result.error_message = NULL;
        result.output = NULL;
        return result;
    }
    
    // Initialize the strings
    memset(result.error_message, 0, config->max_response_size);
    memset(result.output, 0, config->max_response_size);
    
    // Get system temp directory
    const char* temp_dir = getenv("TMPDIR");
    if (!temp_dir) {
        temp_dir = "/tmp";  // fallback for Unix systems
    }
    
    // Create a temporary file with the generated code
    char temp_filename[512];
    snprintf(temp_filename, sizeof(temp_filename), "%s/test.c", temp_dir);
    
    FILE* temp_file = fopen(temp_filename, "w");
    if (!temp_file) {
        snprintf(result.error_message, config->max_response_size, 
                "Failed to create temporary file for problem: %s", problem_description);
        return result;
    }
    
    fprintf(temp_file, "%s\n", code_content);
    fclose(temp_file);
    
    // Test 1: Syntax check
    char syntax_command[512];
    snprintf(syntax_command, sizeof(syntax_command), 
             "gcc -Wall -Wextra -Wpedantic -std=c99 -fsyntax-only %s 2>&1", temp_filename);
    
    char *syntax_output = malloc(config->max_response_size);
    if (!syntax_output) {
        snprintf(result.error_message, config->max_response_size, "Memory allocation failed for syntax output");
        unlink(temp_filename);
        return result;
    }
    
    int syntax_result = execute_command(syntax_command, syntax_output, config->max_response_size);
    
    if (syntax_result == 0) {
        result.syntax_ok = 1;
        
        // Test 2: Simple compilation check
        char compile_command[1024];
        char binary_name[512];
        snprintf(binary_name, sizeof(binary_name), "%s/test", temp_dir);
        
        // Include additional args from config if specified
        if (strlen(config->args) > 0) {
            snprintf(compile_command, sizeof(compile_command), 
                     "gcc -Wall -Wextra -std=c99 -o %s %s %s 2>&1", 
                     binary_name, temp_filename, config->args);
        } else {
            snprintf(compile_command, sizeof(compile_command), 
                     "gcc -Wall -Wextra -std=c99 -o %s %s 2>&1", 
                     binary_name, temp_filename);
        }
        
        char *compile_output = malloc(config->max_response_size);
        if (!compile_output) {
            snprintf(result.error_message, config->max_response_size, "Memory allocation failed for compile output");
            free(syntax_output);
            unlink(temp_filename);
            return result;
        }
        
        int compile_result = execute_command(compile_command, compile_output, config->max_response_size);
        
        if (compile_result == 0) {
            result.compilation_ok = 1;
            
            // Test 3: Simple execution check - capture both stdout and stderr
            if (strstr(code_content, "int main") || strstr(code_content, "void main")) {
                char exec_command[1024];
                // Capture both stdout and stderr for better error reporting
                snprintf(exec_command, sizeof(exec_command), "%s 2>&1", binary_name);
                
                char *exec_output = malloc(config->max_response_size);
                if (!exec_output) {
                    snprintf(result.error_message, config->max_response_size, "Memory allocation failed for exec output");
                    free(compile_output);
                    free(syntax_output);
                    unlink(temp_filename);
                    return result;
                }
                
                int exec_result = execute_command(exec_command, exec_output, config->max_response_size);
                
                // Consider any exit code 0 as success
                if (exec_result == 0) {
                    result.execution_ok = 1;
                    strncpy(result.output, exec_output, config->max_response_size - 1);
                    result.output[config->max_response_size - 1] = '\0';
                } else {
                    // Include program output in error message for better debugging
                    if (strlen(exec_output) > 0) {
                        snprintf(result.error_message, config->max_response_size,
                                "Program exited with non-zero exit code: %d\nOutput:\n%s", exec_result, exec_output);
                    } else {
                        snprintf(result.error_message, config->max_response_size,
                                "Program exited with non-zero exit code: %d", exec_result);
                    }
                }
                
                free(exec_output);
            } else {
                result.execution_ok = 1; // No main function, so execution test is not applicable
                strcpy(result.output, "No main function found - library code compiled successfully");
            }
            
            // Clean up binary
            unlink(binary_name);
        } else {
            snprintf(result.error_message, config->max_response_size,
                    "Compilation failed:\n%s", compile_output);
        }
        
        free(compile_output);
    } else {
        snprintf(result.error_message, config->max_response_size,
                "Syntax check failed:\n%s", syntax_output);
    }
    
    // Clean up temporary file and syntax output
    free(syntax_output);
    unlink(temp_filename);
    
    return result;
}

// Generate a simple test report for the AI agents
char* generate_test_report(const test_result_t* test_result, const char* problem_description) {
    (void)problem_description; // Suppress unused parameter warning
    
    // Use a reasonable default size if we can't determine the actual config size
    int report_size = 4096; // Fallback size
    
    char* report = malloc(report_size);
    if (!report) return NULL;
    
    // Just return the program output if it ran successfully
    if (test_result->execution_ok && test_result->output && strlen(test_result->output) > 0) {
        snprintf(report, report_size, "%s", test_result->output);
    } else if (!test_result->syntax_ok || !test_result->compilation_ok) {
        // Only show compilation errors if they exist
        snprintf(report, report_size, "Compilation failed:\n%s", 
                test_result->error_message ? test_result->error_message : "Unknown error");
    } else if (!test_result->execution_ok) {
        // Show execution error
        snprintf(report, report_size, "Execution failed: %s", 
                test_result->error_message ? test_result->error_message : "Unknown error");
    } else {
        snprintf(report, report_size, "Code compiled and ran successfully");
    }
    
    return report;
}

// Enhanced solution update that includes testing
void update_solution_with_testing(conversation_t *conv, const char *reasoning_response) {
    if (!conv || !conv->config || !conv->current_solution) return;
    
    // Extract code from the reasoning response
    const char *code_start = strstr(reasoning_response, "```c");
    if (!code_start) {
        code_start = strstr(reasoning_response, "```");
    }
    
    if (code_start) {
        // Skip the opening ```c or ```
        code_start = strchr(code_start, '\n');
        if (code_start) code_start++;
        
        const char *code_end = strstr(code_start, "```");
        if (code_end) {
            // Extract the code
            size_t code_length = code_end - code_start;
            if (code_length < (size_t)(conv->config->max_code_size - 1)) {
                strncpy(conv->current_solution, code_start, code_length);
                conv->current_solution[code_length] = '\0';
                
                // Remove trailing whitespace
                char *end = conv->current_solution + strlen(conv->current_solution) - 1;
                while (end > conv->current_solution && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
                    *end = '\0';
                    end--;
                }
                
                // Test the generated code - use custom test command if specified, otherwise built-in testing
                test_result_t test_result = {0};
                
                if (strlen(conv->config->test_command) > 0) {
                    // Custom test command mode
                    log_message(conv->config, VERBOSITY_DEBUG, "%s🧪 Testing code with custom command...%s\n", C_INFO, C_RESET);
                    
                    if (conv->config->enable_evolution && strlen(conv->config->evolution_file_path) > 0) {
                        // Evolution mode: Write to evolved file and test
                        if (write_evolution_file(conv->config->evolution_file_path, conv->current_solution) == 0) {
                            // Create evolved file path for testing
                            char evolved_file_path[1024];
                            snprintf(evolved_file_path, sizeof(evolved_file_path), "%s.evolved", conv->config->evolution_file_path);
                            
                            test_result = run_custom_test(conv->config->test_command, evolved_file_path, conv->config);
                        } else {
                            // Failed to write file, create error result
                            test_result.error_message = malloc(conv->config->max_response_size);
                            test_result.output = malloc(conv->config->max_response_size);
                            if (test_result.error_message && test_result.output) {
                                snprintf(test_result.error_message, conv->config->max_response_size, 
                                        "Failed to write evolved code to file: %s.evolved", conv->config->evolution_file_path);
                                memset(test_result.output, 0, conv->config->max_response_size);
                                test_result.syntax_ok = 0;
                                test_result.compilation_ok = 0;
                                test_result.execution_ok = 0;
                            }
                        }
                    } else {
                        // Standard mode: Write to temporary file and test
                        const char* temp_dir = getenv("TMPDIR");
                        if (!temp_dir) {
                            temp_dir = "/tmp";
                        }
                        
                        char temp_file_path[1024];
                        snprintf(temp_file_path, sizeof(temp_file_path), "%s/test.c", temp_dir);
                        
                        FILE* temp_file = fopen(temp_file_path, "w");
                        if (temp_file) {
                            fprintf(temp_file, "%s\n", conv->current_solution);
                            fclose(temp_file);
                            
                            test_result = run_custom_test(conv->config->test_command, temp_file_path, conv->config);
                            
                            // Clean up temporary file
                            unlink(temp_file_path);
                        } else {
                            // Failed to write temp file, create error result
                            test_result.error_message = malloc(conv->config->max_response_size);
                            test_result.output = malloc(conv->config->max_response_size);
                            if (test_result.error_message && test_result.output) {
                                snprintf(test_result.error_message, conv->config->max_response_size, 
                                        "Failed to create temporary file for custom testing");
                                memset(test_result.output, 0, conv->config->max_response_size);
                                test_result.syntax_ok = 0;
                                test_result.compilation_ok = 0;
                                test_result.execution_ok = 0;
                            }
                        }
                    }
                } else {
                    // Standard mode: Use built-in testing
                    test_result = test_generated_code(conv->current_solution, conv->problem_description, conv->config);
                }
                
                // Clean up previous test result
                cleanup_test_result(&conv->last_test_result);
                
                // Store test result in conversation
                conv->last_test_result = test_result;
                
                char* test_report = generate_test_report(&test_result, conv->problem_description);
                
                if (test_report) {
                    // Show the output based on verbosity level
                    if (conv->config->verbosity >= VERBOSITY_VERBOSE) {
                        log_message(conv->config, VERBOSITY_VERBOSE, "%sProgram Output:%s %s\n", C_INFO, C_RESET, test_report);
                    }
                    
                    // Log test results (minimal)
                    FILE *log_file = fopen("beta-evolve.log", "a");
                    if (log_file) {
                        fprintf(log_file, "Output: %s\n", test_report);
                        fclose(log_file);
                    }
                    
                    free(test_report);
                }
            }
        }
    }
}

// Check if there are any code errors in the test result
int has_code_errors(const test_result_t* test_result) {
    return !test_result->syntax_ok || !test_result->compilation_ok || !test_result->execution_ok;
}

// Get the last test result from conversation
test_result_t get_last_test_result(const conversation_t *conv) {
    return conv->last_test_result;
}
