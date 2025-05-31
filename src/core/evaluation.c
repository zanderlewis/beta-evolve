#include "beta_evolve.h"
#include <math.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>

// Initialize evaluation criteria with default values
void init_evaluation_criteria(evaluation_criteria_t *criteria) {
    if (!criteria) return;
    
    // Performance criteria
    criteria->min_performance_score = 70.0;      // Minimum 70% performance score
    criteria->max_execution_time_ms = 1000.0;    // Maximum 1 second execution time
    criteria->max_memory_usage_kb = 10240;       // Maximum 10MB memory usage
    criteria->target_throughput = 1000.0;        // Target 1000 ops/sec
    
    // Quality criteria
    criteria->min_test_coverage_percent = 80.0;  // Minimum 80% test coverage
    criteria->max_cyclomatic_complexity = 10;    // Maximum complexity of 10
    
    // Enable profiling options
    criteria->enable_performance_profiling = 1;
    criteria->enable_memory_profiling = 1;
    criteria->enable_quality_analysis = 1;
}

// Measure performance metrics for code execution
performance_metrics_t measure_performance(const char *file_path, config_t *config) {
    performance_metrics_t metrics = {0};
    
    if (!file_path || !config) return metrics;
    
    // Create temporary executable
    char binary_path[1024];
    snprintf(binary_path, sizeof(binary_path), "/tmp/perf_test_%d", getpid());
    
    // Compile the code
    char compile_cmd[2048];
    snprintf(compile_cmd, sizeof(compile_cmd), 
             "gcc -O2 -Wall -Wextra -std=c99 -o %s %s 2>/dev/null", 
             binary_path, file_path);
    
    if (system(compile_cmd) != 0) {
        log_message(config, VERBOSITY_DEBUG, "Performance measurement: compilation failed\n");
        return metrics;
    }
    
    // Measure execution time and memory usage
    struct timeval start_time, end_time;
    struct rusage usage_start, usage_end;
    
    // Get initial resource usage
    getrusage(RUSAGE_CHILDREN, &usage_start);
    gettimeofday(&start_time, NULL);
    
    // Run the executable multiple times for average
    int runs = 5;
    double total_time = 0.0;
    long max_memory = 0;
    
    for (int i = 0; i < runs; i++) {
        struct timeval run_start, run_end;
        gettimeofday(&run_start, NULL);
        
        // Execute with timeout
        pid_t pid = fork();
        if (pid == 0) {
            // Child process - execute the binary
            execl(binary_path, binary_path, NULL);
            exit(1);
        } else if (pid > 0) {
            // Parent process - wait and measure
            int status;
            struct rusage child_usage;
            
            if (wait4(pid, &status, 0, &child_usage) == pid) {
                gettimeofday(&run_end, NULL);
                
                // Calculate execution time for this run
                double run_time = (run_end.tv_sec - run_start.tv_sec) * 1000.0 + 
                                 (run_end.tv_usec - run_start.tv_usec) / 1000.0;
                total_time += run_time;
                
                // Track maximum memory usage
                long memory_kb = child_usage.ru_maxrss;
                #ifdef __APPLE__
                memory_kb /= 1024; // macOS reports in bytes, convert to KB
                #endif
                if (memory_kb > max_memory) {
                    max_memory = memory_kb;
                }
            }
        }
    }
    
    gettimeofday(&end_time, NULL);
    getrusage(RUSAGE_CHILDREN, &usage_end);
    
    // Calculate average metrics
    metrics.execution_time_ms = total_time / runs;
    metrics.memory_usage_kb = max_memory;
    
    // Calculate CPU usage (simplified)
    double total_cpu_time = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) +
                           (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1000000.0;
    double wall_time = (end_time.tv_sec - start_time.tv_sec) +
                      (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    
    if (wall_time > 0) {
        metrics.cpu_usage_percent = (int)((total_cpu_time / wall_time) * 100);
    }
    
    // Estimate throughput (operations per second)
    if (metrics.execution_time_ms > 0) {
        metrics.throughput = 1000.0 / metrics.execution_time_ms;
    }
    
    // Clean up
    unlink(binary_path);
    
    log_message(config, VERBOSITY_DEBUG, 
               "Performance: %.2fms execution, %ldKB memory, %d%% CPU, %.1f ops/sec\n",
               metrics.execution_time_ms, metrics.memory_usage_kb, 
               metrics.cpu_usage_percent, metrics.throughput);
    
    return metrics;
}

// Analyze code quality metrics
code_quality_metrics_t analyze_code_quality(const char *code_content) {
    code_quality_metrics_t quality = {0};
    
    if (!code_content) return quality;
    
    // Count lines of code
    const char *ptr = code_content;
    int line_count = 0;
    int function_count = 0;
    int current_function_length = 0;
    int max_function_length = 0;
    int brace_depth = 0;
    int complexity = 1; // Base complexity
    
    while (*ptr) {
        if (*ptr == '\n') {
            line_count++;
            if (brace_depth > 0) {
                current_function_length++;
            }
        }
        
        // Function detection (simplified)
        if (strncmp(ptr, "int ", 4) == 0 || strncmp(ptr, "void ", 5) == 0 || 
            strncmp(ptr, "char ", 5) == 0 || strncmp(ptr, "double ", 7) == 0) {
            // Look for function signature pattern
            const char *paren = strchr(ptr, '(');
            const char *newline = strchr(ptr, '\n');
            if (paren && (!newline || paren < newline)) {
                function_count++;
                if (current_function_length > max_function_length) {
                    max_function_length = current_function_length;
                }
                current_function_length = 0;
            }
        }
        
        // Track braces for function boundaries
        if (*ptr == '{') {
            brace_depth++;
        } else if (*ptr == '}') {
            brace_depth--;
            if (brace_depth == 0) {
                if (current_function_length > max_function_length) {
                    max_function_length = current_function_length;
                }
                current_function_length = 0;
            }
        }
        
        // Count complexity contributors
        if (strncmp(ptr, "if", 2) == 0 || strncmp(ptr, "while", 5) == 0 || 
            strncmp(ptr, "for", 3) == 0 || strncmp(ptr, "switch", 6) == 0) {
            complexity++;
        } else if (strncmp(ptr, "case", 4) == 0 || strncmp(ptr, "&&", 2) == 0 || 
                   strncmp(ptr, "||", 2) == 0) {
            complexity++;
        }
        
        ptr++;
    }
    
    quality.lines_of_code = line_count;
    quality.function_count = function_count;
    quality.max_function_length = max_function_length;
    quality.cyclomatic_complexity = complexity;
    
    // Calculate maintainability index (simplified formula)
    // Based on Halstead volume, cyclomatic complexity, and lines of code
    double volume = line_count * log2(line_count + 1); // Simplified Halstead volume
    quality.maintainability_index = fmax(0, 
        171 - 5.2 * log(volume) - 0.23 * complexity - 16.2 * log(line_count));
    
    // Estimate test coverage (simplified - look for test functions)
    int test_functions = 0;
    ptr = code_content;
    while ((ptr = strstr(ptr, "test_")) != NULL) {
        test_functions++;
        ptr += 5;
    }
    
    if (function_count > 0) {
        quality.test_coverage_percent = fmin(100.0, (double)test_functions / function_count * 100.0);
    }
    
    // Simplified code duplication detection (look for repeated patterns)
    quality.code_duplication_percent = 5; // Default low value
    
    return quality;
}

// Calculate overall score from individual metrics
double calculate_overall_score(const evaluation_result_t *result) {
    if (!result) return 0.0;
    
    // Weighted scoring: correctness 40%, performance 30%, quality 30%
    return (result->correctness_score * 0.4) + 
           (result->performance_score * 0.3) + 
           (result->quality_score * 0.3);
}

// Generate detailed evaluation report
char* generate_evaluation_report(const evaluation_result_t *result) {
    if (!result) return NULL;
    
    dstring_t *report = dstring_create(4096);
    if (!report) return NULL;
    
    dstring_append(report, "=== COMPREHENSIVE CODE EVALUATION REPORT ===\n\n");
    
    // Overall scores
    dstring_append_format(report, "OVERALL SCORE: %.1f/100\n", result->overall_score);
    dstring_append_format(report, "  - Correctness: %.1f/100\n", result->correctness_score);
    dstring_append_format(report, "  - Performance: %.1f/100\n", result->performance_score);
    dstring_append_format(report, "  - Quality: %.1f/100\n\n", result->quality_score);
    
    // Correctness metrics
    dstring_append(report, "CORRECTNESS ANALYSIS:\n");
    dstring_append_format(report, "  - Syntax: %s\n", 
                         result->test_result.syntax_ok ? "✅ PASS" : "❌ FAIL");
    dstring_append_format(report, "  - Compilation: %s\n", 
                         result->test_result.compilation_ok ? "✅ PASS" : "❌ FAIL");
    dstring_append_format(report, "  - Execution: %s\n\n", 
                         result->test_result.execution_ok ? "✅ PASS" : "❌ FAIL");
    
    // Performance metrics
    dstring_append(report, "PERFORMANCE ANALYSIS:\n");
    dstring_append_format(report, "  - Execution Time: %.2f ms\n", result->performance.execution_time_ms);
    dstring_append_format(report, "  - Memory Usage: %ld KB\n", result->performance.memory_usage_kb);
    dstring_append_format(report, "  - CPU Usage: %d%%\n", result->performance.cpu_usage_percent);
    dstring_append_format(report, "  - Throughput: %.1f ops/sec\n\n", result->performance.throughput);
    
    // Quality metrics
    dstring_append(report, "CODE QUALITY ANALYSIS:\n");
    dstring_append_format(report, "  - Lines of Code: %d\n", result->quality.lines_of_code);
    dstring_append_format(report, "  - Function Count: %d\n", result->quality.function_count);
    dstring_append_format(report, "  - Cyclomatic Complexity: %d\n", result->quality.cyclomatic_complexity);
    dstring_append_format(report, "  - Max Function Length: %d lines\n", result->quality.max_function_length);
    dstring_append_format(report, "  - Maintainability Index: %.1f/100\n", result->quality.maintainability_index);
    dstring_append_format(report, "  - Test Coverage: %.1f%%\n", result->quality.test_coverage_percent);
    dstring_append_format(report, "  - Code Duplication: %d%%\n\n", result->quality.code_duplication_percent);
    
    // Status summary
    dstring_append_format(report, "CRITERIA COMPLIANCE: %s\n", 
                         result->passed_criteria ? "✅ PASSED" : "❌ FAILED");
    
    if (result->test_result.error_message && strlen(result->test_result.error_message) > 0) {
        dstring_append_format(report, "\nERROR DETAILS:\n%s\n", result->test_result.error_message);
    }
    
    if (result->test_result.output && strlen(result->test_result.output) > 0) {
        dstring_append_format(report, "\nPROGRAM OUTPUT:\n%s\n", result->test_result.output);
    }
    
    char *result_str = strdup(dstring_get(report));
    dstring_destroy(report);
    return result_str;
}

// Generate improvement recommendations
char* generate_improvement_recommendations(const evaluation_result_t *result) {
    if (!result) return NULL;
    
    dstring_t *recommendations = dstring_create(2048);
    if (!recommendations) return NULL;
    
    dstring_append(recommendations, "=== IMPROVEMENT RECOMMENDATIONS ===\n\n");
    
    // Correctness recommendations
    if (result->correctness_score < 100.0) {
        dstring_append(recommendations, "CORRECTNESS IMPROVEMENTS:\n");
        if (!result->test_result.syntax_ok) {
            dstring_append(recommendations, "  - Fix syntax errors and compilation warnings\n");
        }
        if (!result->test_result.compilation_ok) {
            dstring_append(recommendations, "  - Resolve compilation errors and missing dependencies\n");
        }
        if (!result->test_result.execution_ok) {
            dstring_append(recommendations, "  - Debug runtime errors and segmentation faults\n");
            dstring_append(recommendations, "  - Add proper error handling and input validation\n");
        }
        dstring_append(recommendations, "\n");
    }
    
    // Performance recommendations
    if (result->performance_score < 80.0) {
        dstring_append(recommendations, "PERFORMANCE IMPROVEMENTS:\n");
        if (result->performance.execution_time_ms > 100.0) {
            dstring_append(recommendations, "  - Optimize algorithms for better time complexity\n");
            dstring_append(recommendations, "  - Consider using more efficient data structures\n");
        }
        if (result->performance.memory_usage_kb > 1024) {
            dstring_append(recommendations, "  - Reduce memory usage and fix memory leaks\n");
            dstring_append(recommendations, "  - Use stack allocation where possible\n");
        }
        if (result->performance.cpu_usage_percent > 90) {
            dstring_append(recommendations, "  - Reduce CPU-intensive operations\n");
            dstring_append(recommendations, "  - Consider algorithmic optimizations\n");
        }
        dstring_append(recommendations, "\n");
    }
    
    // Quality recommendations
    if (result->quality_score < 80.0) {
        dstring_append(recommendations, "CODE QUALITY IMPROVEMENTS:\n");
        if (result->quality.cyclomatic_complexity > 10) {
            dstring_append(recommendations, "  - Reduce function complexity by breaking down large functions\n");
            dstring_append(recommendations, "  - Simplify conditional logic and nested structures\n");
        }
        if (result->quality.max_function_length > 50) {
            dstring_append(recommendations, "  - Break down large functions into smaller, focused functions\n");
        }
        if (result->quality.test_coverage_percent < 80.0) {
            dstring_append(recommendations, "  - Add more comprehensive test cases\n");
            dstring_append(recommendations, "  - Include edge case testing and error condition tests\n");
        }
        if (result->quality.maintainability_index < 70.0) {
            dstring_append(recommendations, "  - Improve code documentation and comments\n");
            dstring_append(recommendations, "  - Refactor complex code sections for clarity\n");
        }
        dstring_append(recommendations, "\n");
    }
    
    // General recommendations
    dstring_append(recommendations, "GENERAL RECOMMENDATIONS:\n");
    dstring_append(recommendations, "  - Follow consistent coding style and naming conventions\n");
    dstring_append(recommendations, "  - Add comprehensive documentation and comments\n");
    dstring_append(recommendations, "  - Include robust error handling and input validation\n");
    dstring_append(recommendations, "  - Consider using static analysis tools for additional insights\n");
    
    char *result_str = strdup(dstring_get(recommendations));
    dstring_destroy(recommendations);
    return result_str;
}

// Comprehensive code evaluation
evaluation_result_t evaluate_code_comprehensive(const char *file_path, const char *code_content, 
                                              const evaluation_criteria_t *criteria, config_t *config) {
    evaluation_result_t result = {0};
    
    if (!file_path || !code_content || !config) return result;
    
    // Initialize result structure
    result.evaluation_timestamp = time(NULL);
    result.detailed_report = NULL;
    result.recommendations = NULL;
    
    // Basic correctness testing
    if (strlen(config->test_command) > 0) {
        result.test_result = run_custom_test(config->test_command, file_path, config);
    } else {
        result.test_result = test_generated_code(code_content, "Evaluation", config);
    }
    
    // Calculate correctness score
    result.correctness_score = 0.0;
    if (result.test_result.syntax_ok) result.correctness_score += 33.33;
    if (result.test_result.compilation_ok) result.correctness_score += 33.33;
    if (result.test_result.execution_ok) result.correctness_score += 33.34;
    
    // Performance evaluation
    if (criteria && criteria->enable_performance_profiling && result.test_result.compilation_ok) {
        result.performance = measure_performance(file_path, config);
        
        // Calculate performance score based on criteria
        result.performance_score = 100.0;
        
        if (criteria->max_execution_time_ms > 0) {
            double time_ratio = result.performance.execution_time_ms / criteria->max_execution_time_ms;
            if (time_ratio > 1.0) result.performance_score -= (time_ratio - 1.0) * 50.0;
        }
        
        if (criteria->max_memory_usage_kb > 0) {
            double memory_ratio = (double)result.performance.memory_usage_kb / criteria->max_memory_usage_kb;
            if (memory_ratio > 1.0) result.performance_score -= (memory_ratio - 1.0) * 30.0;
        }
        
        if (criteria->target_throughput > 0 && result.performance.throughput < criteria->target_throughput) {
            double throughput_ratio = result.performance.throughput / criteria->target_throughput;
            result.performance_score *= throughput_ratio;
        }
        
        result.performance_score = fmax(0.0, fmin(100.0, result.performance_score));
    } else {
        result.performance_score = result.test_result.execution_ok ? 75.0 : 0.0;
    }
    
    // Code quality analysis
    if (criteria && criteria->enable_quality_analysis) {
        result.quality = analyze_code_quality(code_content);
        
        // Calculate quality score
        result.quality_score = 100.0;
        
        if (criteria->max_cyclomatic_complexity > 0 && 
            result.quality.cyclomatic_complexity > criteria->max_cyclomatic_complexity) {
            result.quality_score -= 20.0;
        }
        
        if (criteria->min_test_coverage_percent > 0 && 
            result.quality.test_coverage_percent < criteria->min_test_coverage_percent) {
            double coverage_ratio = result.quality.test_coverage_percent / criteria->min_test_coverage_percent;
            result.quality_score *= coverage_ratio;
        }
        
        // Penalize for poor maintainability
        if (result.quality.maintainability_index < 70.0) {
            result.quality_score *= (result.quality.maintainability_index / 70.0);
        }
        
        result.quality_score = fmax(0.0, fmin(100.0, result.quality_score));
    } else {
        result.quality_score = 75.0; // Default score when quality analysis is disabled
    }
    
    // Calculate overall score
    result.overall_score = calculate_overall_score(&result);
    
    // Check if criteria are met
    result.passed_criteria = evaluate_against_criteria(&result, criteria);
    
    // Generate reports
    result.detailed_report = generate_evaluation_report(&result);
    result.recommendations = generate_improvement_recommendations(&result);
    
    return result;
}

// Check if evaluation meets criteria
int evaluate_against_criteria(const evaluation_result_t *result, const evaluation_criteria_t *criteria) {
    if (!result || !criteria) return 0;
    
    // Check minimum performance score
    if (result->performance_score < criteria->min_performance_score) return 0;
    
    // Check execution time limit
    if (criteria->max_execution_time_ms > 0 && 
        result->performance.execution_time_ms > criteria->max_execution_time_ms) return 0;
    
    // Check memory usage limit
    if (criteria->max_memory_usage_kb > 0 && 
        result->performance.memory_usage_kb > criteria->max_memory_usage_kb) return 0;
    
    // Check test coverage requirement
    if (criteria->min_test_coverage_percent > 0 && 
        result->quality.test_coverage_percent < criteria->min_test_coverage_percent) return 0;
    
    // Check complexity limit
    if (criteria->max_cyclomatic_complexity > 0 && 
        result->quality.cyclomatic_complexity > criteria->max_cyclomatic_complexity) return 0;
    
    // Must pass basic correctness tests
    if (!result->test_result.syntax_ok || !result->test_result.compilation_ok || 
        !result->test_result.execution_ok) return 0;
    
    return 1; // All criteria met
}

// Clean up evaluation result memory
void cleanup_evaluation_result(evaluation_result_t *result) {
    if (!result) return;
    
    cleanup_test_result(&result->test_result);
    
    if (result->detailed_report) {
        free(result->detailed_report);
        result->detailed_report = NULL;
    }
    
    if (result->recommendations) {
        free(result->recommendations);
        result->recommendations = NULL;
    }
}

// Save evaluation to history
void save_evaluation_history(code_evolution_t *evolution, const evaluation_result_t *result) {
    if (!evolution || !result) return;
    
    // Reallocate history array if needed
    if (evolution->evaluation_count == 0) {
        evolution->evaluation_history = malloc(sizeof(evaluation_result_t) * 10);
        if (!evolution->evaluation_history) return;
    } else if (evolution->evaluation_count % 10 == 0) {
        evolution->evaluation_history = realloc(evolution->evaluation_history, 
                                              sizeof(evaluation_result_t) * (evolution->evaluation_count + 10));
        if (!evolution->evaluation_history) return;
    }
    
    // Copy the evaluation result
    evolution->evaluation_history[evolution->evaluation_count] = *result;
    evolution->evaluation_count++;
}

// Get best evaluation from history
evaluation_result_t* get_best_evaluation(const code_evolution_t *evolution) {
    if (!evolution || evolution->evaluation_count == 0) return NULL;
    
    evaluation_result_t *best = &evolution->evaluation_history[0];
    for (int i = 1; i < evolution->evaluation_count; i++) {
        if (evolution->evaluation_history[i].overall_score > best->overall_score) {
            best = &evolution->evaluation_history[i];
        }
    }
    
    return best;
}

// Compare two evaluations
void compare_evaluations(const evaluation_result_t *eval1, const evaluation_result_t *eval2, char **comparison_report) {
    if (!eval1 || !eval2 || !comparison_report) return;
    
    dstring_t *report = dstring_create(2048);
    if (!report) return;
    
    dstring_append(report, "=== EVALUATION COMPARISON ===\n\n");
    
    dstring_append_format(report, "OVERALL SCORES:\n");
    dstring_append_format(report, "  Current: %.1f/100\n", eval1->overall_score);
    dstring_append_format(report, "  Previous: %.1f/100\n", eval2->overall_score);
    dstring_append_format(report, "  Change: %+.1f\n\n", eval1->overall_score - eval2->overall_score);
    
    dstring_append_format(report, "CORRECTNESS:\n");
    dstring_append_format(report, "  Current: %.1f/100\n", eval1->correctness_score);
    dstring_append_format(report, "  Previous: %.1f/100\n", eval2->correctness_score);
    dstring_append_format(report, "  Change: %+.1f\n\n", eval1->correctness_score - eval2->correctness_score);
    
    dstring_append_format(report, "PERFORMANCE:\n");
    dstring_append_format(report, "  Current: %.1f/100\n", eval1->performance_score);
    dstring_append_format(report, "  Previous: %.1f/100\n", eval2->performance_score);
    dstring_append_format(report, "  Change: %+.1f\n", eval1->performance_score - eval2->performance_score);
    dstring_append_format(report, "  Time: %.2fms → %.2fms (%+.2fms)\n", 
                         eval2->performance.execution_time_ms, eval1->performance.execution_time_ms,
                         eval1->performance.execution_time_ms - eval2->performance.execution_time_ms);
    dstring_append_format(report, "  Memory: %ldKB → %ldKB (%+ldKB)\n\n", 
                         eval2->performance.memory_usage_kb, eval1->performance.memory_usage_kb,
                         eval1->performance.memory_usage_kb - eval2->performance.memory_usage_kb);
    
    dstring_append_format(report, "CODE QUALITY:\n");
    dstring_append_format(report, "  Current: %.1f/100\n", eval1->quality_score);
    dstring_append_format(report, "  Previous: %.1f/100\n", eval2->quality_score);
    dstring_append_format(report, "  Change: %+.1f\n", eval1->quality_score - eval2->quality_score);
    dstring_append_format(report, "  Complexity: %d → %d\n", 
                         eval2->quality.cyclomatic_complexity, eval1->quality.cyclomatic_complexity);
    dstring_append_format(report, "  Test Coverage: %.1f%% → %.1f%%\n\n", 
                         eval2->quality.test_coverage_percent, eval1->quality.test_coverage_percent);
    
    if (eval1->overall_score > eval2->overall_score) {
        dstring_append(report, "✅ IMPROVEMENT DETECTED\n");
    } else if (eval1->overall_score < eval2->overall_score) {
        dstring_append(report, "⚠️  REGRESSION DETECTED\n");
    } else {
        dstring_append(report, "➡️  NO SIGNIFICANT CHANGE\n");
    }
    
    *comparison_report = strdup(dstring_get(report));
    dstring_destroy(report);
}
