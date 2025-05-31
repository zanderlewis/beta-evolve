#ifndef BETA_EVOLVE_H
#define BETA_EVOLVE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "toml.h"
#include "colors.h"
#include "json.h"
#include <time.h>
#include <unistd.h>
#include <stdarg.h>

// Default values for flexible configuration
#define DEFAULT_MAX_RESPONSE_SIZE 10240
#define DEFAULT_MAX_PROMPT_SIZE 4096
#define DEFAULT_MAX_CONVERSATION_TURNS 10
#define DEFAULT_MAX_CODE_SIZE 8192

// Verbosity levels (integer based)
#define VERBOSITY_QUIET   -1    // Minimal output only
#define VERBOSITY_NORMAL   0    // Default: show iteration progress and error status  
#define VERBOSITY_VERBOSE  1    // Show detailed information and AI responses
#define VERBOSITY_DEBUG    2    // Show everything including API calls and debugging info

// Dynamic string structure for flexible text handling
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} dstring_t;

// Agent types
typedef enum {
    AGENT_FAST,
    AGENT_REASONING
} agent_type_t;

// Testing result structure (must be defined early for evaluation_result_t)
typedef struct {
    int syntax_ok;
    int compilation_ok;
    int execution_ok;
    char *error_message;    // Dynamic allocation
    char *output;           // Dynamic allocation
} test_result_t;

// Code evolution structures and constants
#define EVOLUTION_MARKER_START "// BETA EVOLVE START"
#define EVOLUTION_MARKER_END "// BETA EVOLVE END"
#define MAX_EVOLUTION_REGIONS 50
#define MAX_EVOLUTION_DESCRIPTION 256

// Evolution region structure
typedef struct {
    char *content;                               // Current content in this region
    char description[MAX_EVOLUTION_DESCRIPTION]; // Description/identifier for this region
    int start_line;                              // Line number where region starts
    int end_line;                                // Line number where region ends
    int generation;                              // Evolution generation number
    double fitness_score;                        // Performance/correctness score
} evolution_region_t;

// Performance metrics structure
typedef struct {
    double execution_time_ms;                    // Execution time in milliseconds
    long memory_usage_kb;                        // Memory usage in kilobytes
    int cpu_usage_percent;                       // CPU usage percentage
    double throughput;                           // Operations per second (if applicable)
    int cache_misses;                            // Cache misses (if profiled)
} performance_metrics_t;

// Code quality metrics structure
typedef struct {
    int lines_of_code;                           // Total lines of code
    int cyclomatic_complexity;                   // Code complexity measure
    double test_coverage_percent;                // Test coverage percentage
    int function_count;                          // Number of functions
    int max_function_length;                     // Longest function in lines
    double maintainability_index;               // Maintainability score (0-100)
    int code_duplication_percent;                // Percentage of duplicated code
} code_quality_metrics_t;

// Evaluation criteria structure
typedef struct {
    double min_performance_score;                // Minimum acceptable performance score
    double max_execution_time_ms;                // Maximum allowed execution time
    long max_memory_usage_kb;                    // Maximum allowed memory usage
    double min_test_coverage_percent;            // Minimum test coverage required
    int max_cyclomatic_complexity;               // Maximum complexity allowed
    double target_throughput;                    // Target operations per second
    int enable_performance_profiling;            // Enable detailed performance analysis
    int enable_memory_profiling;                 // Enable memory usage analysis
    int enable_quality_analysis;                 // Enable code quality analysis
} evaluation_criteria_t;

// Comprehensive evaluation result structure
typedef struct {
    double overall_score;                        // Overall evaluation score (0-100)
    double correctness_score;                    // Correctness score (0-100)
    double performance_score;                    // Performance score (0-100)
    double quality_score;                        // Code quality score (0-100)
    performance_metrics_t performance;           // Performance metrics
    code_quality_metrics_t quality;              // Code quality metrics
    test_result_t test_result;                   // Basic test results
    char *detailed_report;                       // Detailed evaluation report
    char *recommendations;                       // Improvement recommendations
    time_t evaluation_timestamp;                 // When evaluation was performed
    int passed_criteria;                         // Whether code meets criteria
} evaluation_result_t;

// Code evolution context
typedef struct {
    evolution_region_t regions[MAX_EVOLUTION_REGIONS];
    int region_count;
    char *base_code;                             // Non-evolvable skeleton code
    int total_generations;                       // Total evolution cycles
    int current_generation;                      // Current evolution generation
    int evolution_enabled;                       // Whether evolution is active
    evaluation_result_t *evaluation_history;     // History of evaluations
    int evaluation_count;                        // Number of evaluations performed
    evaluation_criteria_t criteria;              // Evaluation criteria
} code_evolution_t;

// Configuration (must be defined before conversation_t)
typedef struct {
    char fast_model_api_key[256];
    char reasoning_model_api_key[256];
    char fast_model_endpoint[512];
    char reasoning_model_endpoint[512];
    char fast_model_name[128];
    char reasoning_model_name[128];
    int iterations;
    // Flexible configuration parameters
    int max_response_size;
    int max_prompt_size;
    int max_conversation_turns;
    int max_code_size;
    // Problem prompt file configuration
    char problem_prompt_file[512];
    char *loaded_problem_prompt;  // Dynamically loaded problem description
    // Additional arguments for compilation/execution
    char args[1024];
    // Evolution configuration
    char evolution_file_path[512];       // Path to the code file to evolve
    char test_command[1024];             // Custom command to test the evolved code
    int enable_evolution;                // Enable/disable evolution mode
    // Evaluation configuration
    evaluation_criteria_t eval_criteria; // Evaluation criteria and thresholds
    int enable_comprehensive_evaluation; // Enable detailed evaluation
    int save_evaluation_history;         // Save evaluation results to history
    char evaluation_output_file[512];    // File to save evaluation reports
    // Output control
    int verbosity;
    int use_colors;
} config_t;

// Message structure for conversation
typedef struct {
    agent_type_t sender;
    char *content;          // Dynamic allocation
    time_t timestamp;
} message_t;

// Conversation context
typedef struct {
    message_t *messages;                    // Dynamic array of messages
    int message_count;                      // Number of messages in the conversation
    int max_messages;                       // Based on max_conversation_turns * 2
    char *problem_description;              // Dynamic allocation for problem description
    char *current_solution;                 // Dynamic allocation for current solution
    int iterations;                         // Current iteration number
    test_result_t last_test_result;         // Last test result for the current solution
    config_t *config;                       // Reference to config for limits
    code_evolution_t evolution;             // Code evolution context
} conversation_t;

// Include AI and config headers after types are defined
#include "ai.h"
#include "config.h"

// Function declarations

// Dynamic string functions
dstring_t* dstring_create(size_t initial_capacity);
void dstring_destroy(dstring_t* ds);
int dstring_append(dstring_t* ds, const char* str);
int dstring_append_format(dstring_t* ds, const char* format, ...);
void dstring_clear(dstring_t* ds);
char* dstring_get(const dstring_t* ds);

// Core functions
char* dstring_get(const dstring_t* ds);

// Core functions (moved to separate modules)
// Config functions are now in config_manager.h
// AI client functions are now in ai_client.h

void init_conversation(conversation_t *conv, const char *problem, config_t *config);
void add_message(conversation_t *conv, agent_type_t sender, const char *content);
void print_conversation(const conversation_t *conv);
char* generate_agent_prompt(const conversation_t *conv, agent_type_t agent);
void update_solution(conversation_t *conv, const char *reasoning_response);
int run_collaboration(const char *problem, config_t *config);
void cleanup_conversation(conversation_t *conv);

// Testing functions
int execute_command(const char* command, char* output, size_t output_size);
test_result_t test_generated_code(const char* code_content, const char* problem_description, config_t *config);
char* generate_test_report(const test_result_t* test_result, const char* problem_description);
void update_solution_with_testing(conversation_t *conv, const char *reasoning_response);
int has_code_errors(const test_result_t* test_result);
test_result_t get_last_test_result(const conversation_t *conv);
void cleanup_test_result(test_result_t *test_result);

// Logging and output functions
void log_message(config_t *config, int level, const char* format, ...);
void log_iteration_start(config_t *config, int iteration, int total_iterations);
void log_code_status(config_t *config, const test_result_t* test_result);
void log_ai_interaction(config_t *config, agent_type_t agent, const char* prompt, const char* response);
void log_error_details(config_t *config, const char* error_message);

// Code evolution functions
void init_code_evolution(code_evolution_t *evolution);
void cleanup_code_evolution(code_evolution_t *evolution);
int parse_evolution_regions(code_evolution_t *evolution, const char *code);
char* assemble_evolved_code(const code_evolution_t *evolution, const char *original_code);
int extract_evolution_region_content(const char *code, const char *region_desc, char **content);
void update_evolution_region(code_evolution_t *evolution, const char *region_desc, const char *new_content);
double evaluate_evolution_fitness(const char *file_path, config_t *config);
char* generate_evolution_prompt(const conversation_t *conv, const code_evolution_t *evolution, agent_type_t agent);
void evolve_code_regions(conversation_t *conv, code_evolution_t *evolution);
// File-based evolution functions
char* read_evolution_file(const char *file_path);
int write_evolution_file(const char *file_path, const char *content);
test_result_t run_custom_test(const char *test_command, const char *file_path, config_t *config);

// Enhanced evaluation functions
evaluation_result_t evaluate_code_comprehensive(const char *file_path, const char *code_content, 
                                              const evaluation_criteria_t *criteria, config_t *config);
performance_metrics_t measure_performance(const char *file_path, config_t *config);
code_quality_metrics_t analyze_code_quality(const char *code_content);
double calculate_overall_score(const evaluation_result_t *result);
char* generate_evaluation_report(const evaluation_result_t *result);
char* generate_improvement_recommendations(const evaluation_result_t *result);
void init_evaluation_criteria(evaluation_criteria_t *criteria);
void cleanup_evaluation_result(evaluation_result_t *result);
int evaluate_against_criteria(const evaluation_result_t *result, const evaluation_criteria_t *criteria);
void save_evaluation_history(code_evolution_t *evolution, const evaluation_result_t *result);
evaluation_result_t* get_best_evaluation(const code_evolution_t *evolution);
void compare_evaluations(const evaluation_result_t *eval1, const evaluation_result_t *eval2, char **comparison_report);

#endif // BETA_EVOLVE_H
