#include "colors.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Global variable to control color output
int colors_enabled = 1;

// Semantic color helper functions
const char* get_success_color(void) {
    return should_use_colors() ? COLOR_BRIGHT_GREEN : "";
}

const char* get_error_color(void) {
    return should_use_colors() ? COLOR_BRIGHT_RED : "";
}

const char* get_warning_color(void) {
    return should_use_colors() ? COLOR_BRIGHT_YELLOW : "";
}

const char* get_info_color(void) {
    return should_use_colors() ? COLOR_BRIGHT_BLUE : "";
}

const char* get_debug_color(void) {
    return should_use_colors() ? COLOR_BRIGHT_BLACK : "";
}

const char* get_header_color(void) {
    static char header_color[50];
    if (should_use_colors()) {
        snprintf(header_color, sizeof(header_color), "%s%s", COLOR_BOLD, COLOR_CYAN);
        return header_color;
    }
    return "";
}

const char* get_emphasis_color(void) {
    return should_use_colors() ? COLOR_BOLD : "";
}

const char* get_subtle_color(void) {
    return should_use_colors() ? COLOR_DIM : "";
}

// Initialize colors based on environment
void colors_init(void) {
    const char* term = getenv("TERM");
    const char* no_color = getenv("NO_COLOR");
    const char* force_color = getenv("FORCE_COLOR");
    
    // Disable colors if NO_COLOR is set (respects standard)
    if (no_color && strlen(no_color) > 0) {
        colors_enabled = 0;
        return;
    }
    
    // Force colors if FORCE_COLOR is set
    if (force_color && strlen(force_color) > 0) {
        colors_enabled = 1;
        return;
    }
    
    // Check if terminal supports colors
    if (!term || strcmp(term, "dumb") == 0) {
        colors_enabled = 0;
        return;
    }
    
    // Enable colors by default for most terminals
    colors_enabled = 1;
}

// Explicitly enable colors
void colors_enable(void) {
    colors_enabled = 1;
}

// Explicitly disable colors
void colors_disable(void) {
    colors_enabled = 0;
}

// Auto-detect color support
void colors_auto(void) {
    colors_init();
}

// Print a styled header
void print_header(const char* title) {
    if (!title) return;
    
    int len = strlen(title);
    int total_width = len + 4; // Account for padding
    
    printf("\n%s", C_HEADER);
    for (int i = 0; i < total_width; i++) {
        printf("═");
    }
    printf("\n║ %s%s%s ║\n", C_BOLD, title, C_HEADER);
    for (int i = 0; i < total_width; i++) {
        printf("═");
    }
    printf("%s\n\n", C_RESET);
}

// Print a separator line
void print_separator(void) {
    printf("%s", C_SUBTLE);
    for (int i = 0; i < 60; i++) {
        printf("─");
    }
    printf("%s\n", C_RESET);
}

// Print a progress bar
void print_progress_bar(int current, int total, const char* label) {
    if (total <= 0) return;
    
    int width = 30;
    int filled = (current * width) / total;
    int percent = (current * 100) / total;
    
    printf("\r%s%s%s [", C_INFO, label ? label : "Progress", C_RESET);
    
    // Print filled portion
    printf("%s", C_SUCCESS);
    for (int i = 0; i < filled; i++) {
        printf("█");
    }
    
    // Print empty portion
    printf("%s", C_SUBTLE);
    for (int i = filled; i < width; i++) {
        printf("░");
    }
    
    printf("%s] %s%3d%%%s (%d/%d)", 
           C_RESET, C_EMPHASIS, percent, C_RESET, current, total);
    
    if (current >= total) {
        printf("\n");
    }
    
    fflush(stdout);
}

// Print a status message with appropriate coloring
void print_status(const char* status, const char* message) {
    if (!status || !message) return;
    
    const char* color = C_RESET;
    const char* symbol = "•";
    
    if (strstr(status, "success") || strstr(status, "ok") || strstr(status, "pass")) {
        color = C_SUCCESS;
        symbol = "✓";
    } else if (strstr(status, "error") || strstr(status, "fail") || strstr(status, "crash")) {
        color = C_ERROR;
        symbol = "✗";
    } else if (strstr(status, "warning") || strstr(status, "warn")) {
        color = C_WARNING;
        symbol = "⚠";
    } else if (strstr(status, "info")) {
        color = C_INFO;
        symbol = "ℹ";
    } else if (strstr(status, "debug")) {
        color = C_DEBUG;
        symbol = "🐛";
    }
    
    printf("%s%s%s %s%s%s: %s\n", 
           color, symbol, C_RESET,
           C_BOLD, status, C_RESET,
           message);
}

// Print a code block with syntax highlighting hints
void print_code_block(const char* code, const char* language) {
    if (!code) return;
    
    printf("%s╭─", C_SUBTLE);
    if (language) {
        printf(" %s%s%s ", C_EMPHASIS, language, C_SUBTLE);
    }
    for (int i = 0; i < 50; i++) {
        printf("─");
    }
    printf("╮%s\n", C_RESET);
    
    // Split code into lines and print with line numbers
    char* code_copy = strdup(code);
    char* line = strtok(code_copy, "\n");
    int line_num = 1;
    
    while (line) {
        printf("%s│%s %s%3d%s │ %s\n", 
               C_SUBTLE, C_RESET,
               C_DIM, line_num, C_RESET,
               line);
        line = strtok(NULL, "\n");
        line_num++;
    }
    
    printf("%s╰", C_SUBTLE);
    for (int i = 0; i < 55; i++) {
        printf("─");
    }
    printf("╯%s\n", C_RESET);
    
    free(code_copy);
}
