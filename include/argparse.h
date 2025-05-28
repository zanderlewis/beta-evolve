#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Maximum number of arguments and options
#define MAX_ARGS 32
#define MAX_OPTIONS 16
#define MAX_ARG_NAME_LEN 64
#define MAX_ARG_HELP_LEN 256
#define MAX_ARG_VALUE_LEN 512

// Argument types
typedef enum {
    ARG_TYPE_STRING,
    ARG_TYPE_INT,
    ARG_TYPE_FLOAT,
    ARG_TYPE_BOOL,
    ARG_TYPE_FLAG
} arg_type_t;

// Argument definition
typedef struct {
    char name[MAX_ARG_NAME_LEN];
    char short_name;
    char help[MAX_ARG_HELP_LEN];
    arg_type_t type;
    bool required;
    bool found;
    union {
        char str_value[MAX_ARG_VALUE_LEN];
        int int_value;
        float float_value;
        bool bool_value;
    } value;
    union {
        char str_default[MAX_ARG_VALUE_LEN];
        int int_default;
        float float_default;
        bool bool_default;
    } default_value;
} arg_t;

// Argument parser structure
typedef struct {
    char program_name[MAX_ARG_NAME_LEN];
    char description[MAX_ARG_HELP_LEN];
    arg_t args[MAX_ARGS];
    char *positional_args[MAX_ARGS];
    int num_args;
    int num_positional;
    bool help_requested;
} argparser_t;

// Function prototypes
argparser_t* argparse_create(const char *program_name, const char *description);
void argparse_destroy(argparser_t *parser);

// Add argument functions
void argparse_add_string(argparser_t *parser, const char *name, char short_name, 
                        const char *help, bool required, const char *default_value);
void argparse_add_int(argparser_t *parser, const char *name, char short_name,
                     const char *help, bool required, int default_value);
void argparse_add_float(argparser_t *parser, const char *name, char short_name,
                       const char *help, bool required, float default_value);
void argparse_add_bool(argparser_t *parser, const char *name, char short_name,
                      const char *help, bool default_value);
void argparse_add_flag(argparser_t *parser, const char *name, char short_name,
                      const char *help);

// Parse arguments
bool argparse_parse(argparser_t *parser, int argc, char *argv[]);

// Get argument values
const char* argparse_get_string(argparser_t *parser, const char *name);
int argparse_get_int(argparser_t *parser, const char *name);
float argparse_get_float(argparser_t *parser, const char *name);
bool argparse_get_bool(argparser_t *parser, const char *name);
bool argparse_is_set(argparser_t *parser, const char *name);

// Get positional arguments
int argparse_get_positional_count(argparser_t *parser);
const char* argparse_get_positional(argparser_t *parser, int index);

// Help and usage
void argparse_print_help(argparser_t *parser);
void argparse_print_usage(argparser_t *parser);

#endif // ARGPARSE_H
