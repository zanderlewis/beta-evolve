#ifndef COLORS_H
#define COLORS_H

#include <stdio.h>
#include <unistd.h>

// Color codes for terminal output
#define COLOR_RESET     "\033[0m"
#define COLOR_BOLD      "\033[1m"
#define COLOR_DIM       "\033[2m"
#define COLOR_UNDERLINE "\033[4m"
#define COLOR_BLINK     "\033[5m"
#define COLOR_REVERSE   "\033[7m"

// Standard colors
#define COLOR_BLACK     "\033[30m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"

// Bright colors
#define COLOR_BRIGHT_BLACK   "\033[90m"
#define COLOR_BRIGHT_RED     "\033[91m"
#define COLOR_BRIGHT_GREEN   "\033[92m"
#define COLOR_BRIGHT_YELLOW  "\033[93m"
#define COLOR_BRIGHT_BLUE    "\033[94m"
#define COLOR_BRIGHT_MAGENTA "\033[95m"
#define COLOR_BRIGHT_CYAN    "\033[96m"
#define COLOR_BRIGHT_WHITE   "\033[97m"

// Background colors
#define COLOR_BG_BLACK   "\033[40m"
#define COLOR_BG_RED     "\033[41m"
#define COLOR_BG_GREEN   "\033[42m"
#define COLOR_BG_YELLOW  "\033[43m"
#define COLOR_BG_BLUE    "\033[44m"
#define COLOR_BG_MAGENTA "\033[45m"
#define COLOR_BG_CYAN    "\033[46m"
#define COLOR_BG_WHITE   "\033[47m"

// 256-color support
#define COLOR_256(n) "\033[38;5;" #n "m"
#define COLOR_BG_256(n) "\033[48;5;" #n "m"

// RGB color support
#define COLOR_RGB(r,g,b) "\033[38;2;" #r ";" #g ";" #b "m"
#define COLOR_BG_RGB(r,g,b) "\033[48;2;" #r ";" #g ";" #b "m"

// Global variable to control color output
extern int colors_enabled;

// Function to check if colors should be enabled
static inline int should_use_colors(void) {
    return colors_enabled && isatty(STDOUT_FILENO);
}

// Macros that conditionally apply colors
#define C_RESET     (should_use_colors() ? COLOR_RESET : "")
#define C_BOLD      (should_use_colors() ? COLOR_BOLD : "")
#define C_DIM       (should_use_colors() ? COLOR_DIM : "")
#define C_UNDERLINE (should_use_colors() ? COLOR_UNDERLINE : "")
#define C_BLINK     (should_use_colors() ? COLOR_BLINK : "")
#define C_REVERSE   (should_use_colors() ? COLOR_REVERSE : "")

#define C_BLACK     (should_use_colors() ? COLOR_BLACK : "")
#define C_RED       (should_use_colors() ? COLOR_RED : "")
#define C_GREEN     (should_use_colors() ? COLOR_GREEN : "")
#define C_YELLOW    (should_use_colors() ? COLOR_YELLOW : "")
#define C_BLUE      (should_use_colors() ? COLOR_BLUE : "")
#define C_MAGENTA   (should_use_colors() ? COLOR_MAGENTA : "")
#define C_CYAN      (should_use_colors() ? COLOR_CYAN : "")
#define C_WHITE     (should_use_colors() ? COLOR_WHITE : "")

#define C_BRIGHT_BLACK   (should_use_colors() ? COLOR_BRIGHT_BLACK : "")
#define C_BRIGHT_RED     (should_use_colors() ? COLOR_BRIGHT_RED : "")
#define C_BRIGHT_GREEN   (should_use_colors() ? COLOR_BRIGHT_GREEN : "")
#define C_BRIGHT_YELLOW  (should_use_colors() ? COLOR_BRIGHT_YELLOW : "")
#define C_BRIGHT_BLUE    (should_use_colors() ? COLOR_BRIGHT_BLUE : "")
#define C_BRIGHT_MAGENTA (should_use_colors() ? COLOR_BRIGHT_MAGENTA : "")
#define C_BRIGHT_CYAN    (should_use_colors() ? COLOR_BRIGHT_CYAN : "")
#define C_BRIGHT_WHITE   (should_use_colors() ? COLOR_BRIGHT_WHITE : "")

#define C_BG_BLACK   (should_use_colors() ? COLOR_BG_BLACK : "")
#define C_BG_RED     (should_use_colors() ? COLOR_BG_RED : "")
#define C_BG_GREEN   (should_use_colors() ? COLOR_BG_GREEN : "")
#define C_BG_YELLOW  (should_use_colors() ? COLOR_BG_YELLOW : "")
#define C_BG_BLUE    (should_use_colors() ? COLOR_BG_BLUE : "")
#define C_BG_MAGENTA (should_use_colors() ? COLOR_BG_MAGENTA : "")
#define C_BG_CYAN    (should_use_colors() ? COLOR_BG_CYAN : "")
#define C_BG_WHITE   (should_use_colors() ? COLOR_BG_WHITE : "")

// Semantic color helper functions
const char* get_success_color(void);
const char* get_error_color(void);
const char* get_warning_color(void);
const char* get_info_color(void);
const char* get_debug_color(void);
const char* get_header_color(void);
const char* get_emphasis_color(void);
const char* get_subtle_color(void);

// Semantic color macros for different types of output
#define C_SUCCESS    get_success_color()
#define C_ERROR      get_error_color()
#define C_WARNING    get_warning_color()
#define C_INFO       get_info_color()
#define C_DEBUG      get_debug_color()
#define C_HEADER     get_header_color()
#define C_EMPHASIS   get_emphasis_color()
#define C_SUBTLE     get_subtle_color()

// Status symbols with colors
#define STATUS_SUCCESS    C_SUCCESS "✅" C_RESET
#define STATUS_ERROR      C_ERROR "❌" C_RESET
#define STATUS_WARNING    C_WARNING "⚠️" C_RESET
#define STATUS_INFO       C_INFO "ℹ️" C_RESET
#define STATUS_WORKING    C_YELLOW "⚙️" C_RESET
#define STATUS_THINKING   C_MAGENTA "🤔" C_RESET

// Functions for color management
void colors_init(void);
void colors_enable(void);
void colors_disable(void);
void colors_auto(void);

// Pretty printing functions
void print_header(const char* title);
void print_separator(void);
void print_progress_bar(int current, int total, const char* label);
void print_status(const char* status, const char* message);
void print_code_block(const char* code, const char* language);

#endif // COLORS_H
