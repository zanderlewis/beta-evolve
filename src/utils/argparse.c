#include "argparse.h"

// Create a new argument parser
argparser_t* argparse_create(const char *program_name, const char *description) {
    argparser_t *parser = malloc(sizeof(argparser_t));
    if (!parser) return NULL;
    
    strncpy(parser->program_name, program_name, MAX_ARG_NAME_LEN - 1);
    parser->program_name[MAX_ARG_NAME_LEN - 1] = '\0';
    
    if (description) {
        strncpy(parser->description, description, MAX_ARG_HELP_LEN - 1);
        parser->description[MAX_ARG_HELP_LEN - 1] = '\0';
    } else {
        parser->description[0] = '\0';
    }
    
    parser->num_args = 0;
    parser->num_positional = 0;
    parser->help_requested = false;
    
    return parser;
}

// Destroy argument parser
void argparse_destroy(argparser_t *parser) {
    if (parser) {
        for (int i = 0; i < parser->num_positional; i++) {
            if (parser->positional_args[i]) {
                free(parser->positional_args[i]);
            }
        }
        free(parser);
    }
}

// Helper function to find argument by name
static arg_t* find_arg(argparser_t *parser, const char *name) {
    for (int i = 0; i < parser->num_args; i++) {
        if (strcmp(parser->args[i].name, name) == 0) {
            return &parser->args[i];
        }
    }
    return NULL;
}

// Helper function to find argument by short name
static arg_t* find_arg_short(argparser_t *parser, char short_name) {
    for (int i = 0; i < parser->num_args; i++) {
        if (parser->args[i].short_name == short_name) {
            return &parser->args[i];
        }
    }
    return NULL;
}

// Add string argument
void argparse_add_string(argparser_t *parser, const char *name, char short_name, 
                        const char *help, bool required, const char *default_value) {
    if (parser->num_args >= MAX_ARGS) return;
    
    arg_t *arg = &parser->args[parser->num_args++];
    strncpy(arg->name, name, MAX_ARG_NAME_LEN - 1);
    arg->name[MAX_ARG_NAME_LEN - 1] = '\0';
    arg->short_name = short_name;
    strncpy(arg->help, help ? help : "", MAX_ARG_HELP_LEN - 1);
    arg->help[MAX_ARG_HELP_LEN - 1] = '\0';
    arg->type = ARG_TYPE_STRING;
    arg->required = required;
    arg->found = false;
    
    if (default_value) {
        strncpy(arg->default_value.str_default, default_value, MAX_ARG_VALUE_LEN - 1);
        arg->default_value.str_default[MAX_ARG_VALUE_LEN - 1] = '\0';
        strncpy(arg->value.str_value, default_value, MAX_ARG_VALUE_LEN - 1);
        arg->value.str_value[MAX_ARG_VALUE_LEN - 1] = '\0';
    } else {
        arg->default_value.str_default[0] = '\0';
        arg->value.str_value[0] = '\0';
    }
}

// Add integer argument
void argparse_add_int(argparser_t *parser, const char *name, char short_name,
                     const char *help, bool required, int default_value) {
    if (parser->num_args >= MAX_ARGS) return;
    
    arg_t *arg = &parser->args[parser->num_args++];
    strncpy(arg->name, name, MAX_ARG_NAME_LEN - 1);
    arg->name[MAX_ARG_NAME_LEN - 1] = '\0';
    arg->short_name = short_name;
    strncpy(arg->help, help ? help : "", MAX_ARG_HELP_LEN - 1);
    arg->help[MAX_ARG_HELP_LEN - 1] = '\0';
    arg->type = ARG_TYPE_INT;
    arg->required = required;
    arg->found = false;
    arg->default_value.int_default = default_value;
    arg->value.int_value = default_value;
}

// Add float argument
void argparse_add_float(argparser_t *parser, const char *name, char short_name,
                       const char *help, bool required, float default_value) {
    if (parser->num_args >= MAX_ARGS) return;
    
    arg_t *arg = &parser->args[parser->num_args++];
    strncpy(arg->name, name, MAX_ARG_NAME_LEN - 1);
    arg->name[MAX_ARG_NAME_LEN - 1] = '\0';
    arg->short_name = short_name;
    strncpy(arg->help, help ? help : "", MAX_ARG_HELP_LEN - 1);
    arg->help[MAX_ARG_HELP_LEN - 1] = '\0';
    arg->type = ARG_TYPE_FLOAT;
    arg->required = required;
    arg->found = false;
    arg->default_value.float_default = default_value;
    arg->value.float_value = default_value;
}

// Add boolean argument
void argparse_add_bool(argparser_t *parser, const char *name, char short_name,
                      const char *help, bool default_value) {
    if (parser->num_args >= MAX_ARGS) return;
    
    arg_t *arg = &parser->args[parser->num_args++];
    strncpy(arg->name, name, MAX_ARG_NAME_LEN - 1);
    arg->name[MAX_ARG_NAME_LEN - 1] = '\0';
    arg->short_name = short_name;
    strncpy(arg->help, help ? help : "", MAX_ARG_HELP_LEN - 1);
    arg->help[MAX_ARG_HELP_LEN - 1] = '\0';
    arg->type = ARG_TYPE_BOOL;
    arg->required = false; // Bool args are never required
    arg->found = false;
    arg->default_value.bool_default = default_value;
    arg->value.bool_value = default_value;
}

// Add flag argument (boolean that defaults to false)
void argparse_add_flag(argparser_t *parser, const char *name, char short_name,
                      const char *help) {
    argparse_add_bool(parser, name, short_name, help, false);
    parser->args[parser->num_args - 1].type = ARG_TYPE_FLAG;
}

// Parse command line arguments
bool argparse_parse(argparser_t *parser, int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        
        // Check for help
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            parser->help_requested = true;
            return true;
        }
        
        // Long option (--name)
        if (strncmp(arg, "--", 2) == 0) {
            char *name = arg + 2;
            char *value = strchr(name, '=');
            
            if (value) {
                *value = '\0';
                value++;
            }
            
            arg_t *found_arg = find_arg(parser, name);
            if (!found_arg) {
                fprintf(stderr, "Unknown option: --%s\n", name);
                return false;
            }
            
            found_arg->found = true;
            
            if (found_arg->type == ARG_TYPE_FLAG) {
                found_arg->value.bool_value = true;
            } else if (found_arg->type == ARG_TYPE_BOOL) {
                if (value) {
                    found_arg->value.bool_value = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
                } else {
                    found_arg->value.bool_value = true;
                }
            } else {
                if (!value) {
                    if (i + 1 >= argc) {
                        fprintf(stderr, "Option --%s requires a value\n", name);
                        return false;
                    }
                    value = argv[++i];
                }
                
                switch (found_arg->type) {
                    case ARG_TYPE_STRING:
                        strncpy(found_arg->value.str_value, value, MAX_ARG_VALUE_LEN - 1);
                        found_arg->value.str_value[MAX_ARG_VALUE_LEN - 1] = '\0';
                        break;
                    case ARG_TYPE_INT:
                        found_arg->value.int_value = atoi(value);
                        break;
                    case ARG_TYPE_FLOAT:
                        found_arg->value.float_value = atof(value);
                        break;
                    default:
                        break;
                }
            }
        }
        // Short option (-x)
        else if (arg[0] == '-' && arg[1] != '\0') {
            for (int j = 1; arg[j] != '\0'; j++) {
                char short_name = arg[j];
                arg_t *found_arg = find_arg_short(parser, short_name);
                
                if (!found_arg) {
                    fprintf(stderr, "Unknown option: -%c\n", short_name);
                    return false;
                }
                
                found_arg->found = true;
                
                if (found_arg->type == ARG_TYPE_FLAG) {
                    found_arg->value.bool_value = true;
                } else if (found_arg->type == ARG_TYPE_BOOL) {
                    found_arg->value.bool_value = true;
                } else {
                    // Need a value
                    char *value = NULL;
                    if (arg[j + 1] != '\0') {
                        // Value attached to flag (-xvalue)
                        value = &arg[j + 1];
                        j = strlen(arg); // Skip rest of this arg
                    } else if (i + 1 < argc) {
                        // Value is next argument
                        value = argv[++i];
                    } else {
                        fprintf(stderr, "Option -%c requires a value\n", short_name);
                        return false;
                    }
                    
                    switch (found_arg->type) {
                        case ARG_TYPE_STRING:
                            strncpy(found_arg->value.str_value, value, MAX_ARG_VALUE_LEN - 1);
                            found_arg->value.str_value[MAX_ARG_VALUE_LEN - 1] = '\0';
                            break;
                        case ARG_TYPE_INT:
                            found_arg->value.int_value = atoi(value);
                            break;
                        case ARG_TYPE_FLOAT:
                            found_arg->value.float_value = atof(value);
                            break;
                        default:
                            break;
                    }
                    break; // Only one value per short option group
                }
            }
        }
        // Positional argument
        else {
            if (parser->num_positional < MAX_ARGS) {
                parser->positional_args[parser->num_positional] = malloc(strlen(arg) + 1);
                strcpy(parser->positional_args[parser->num_positional], arg);
                parser->num_positional++;
            }
        }
    }
    
    // Check required arguments
    for (int i = 0; i < parser->num_args; i++) {
        if (parser->args[i].required && !parser->args[i].found) {
            fprintf(stderr, "Required argument --%s is missing\n", parser->args[i].name);
            return false;
        }
    }
    
    return true;
}

// Get string value
const char* argparse_get_string(argparser_t *parser, const char *name) {
    arg_t *arg = find_arg(parser, name);
    if (arg && arg->type == ARG_TYPE_STRING) {
        return arg->value.str_value;
    }
    return NULL;
}

// Get integer value
int argparse_get_int(argparser_t *parser, const char *name) {
    arg_t *arg = find_arg(parser, name);
    if (arg && arg->type == ARG_TYPE_INT) {
        return arg->value.int_value;
    }
    return 0;
}

// Get float value
float argparse_get_float(argparser_t *parser, const char *name) {
    arg_t *arg = find_arg(parser, name);
    if (arg && arg->type == ARG_TYPE_FLOAT) {
        return arg->value.float_value;
    }
    return 0.0f;
}

// Get boolean value
bool argparse_get_bool(argparser_t *parser, const char *name) {
    arg_t *arg = find_arg(parser, name);
    if (arg && (arg->type == ARG_TYPE_BOOL || arg->type == ARG_TYPE_FLAG)) {
        return arg->value.bool_value;
    }
    return false;
}

// Check if argument was set
bool argparse_is_set(argparser_t *parser, const char *name) {
    arg_t *arg = find_arg(parser, name);
    return arg ? arg->found : false;
}

// Get positional argument count
int argparse_get_positional_count(argparser_t *parser) {
    return parser->num_positional;
}

// Get positional argument
const char* argparse_get_positional(argparser_t *parser, int index) {
    if (index >= 0 && index < parser->num_positional) {
        return parser->positional_args[index];
    }
    return NULL;
}

// Print help
void argparse_print_help(argparser_t *parser) {
    printf("Usage: %s [OPTIONS] [ARGS...]\n", parser->program_name);
    
    if (strlen(parser->description) > 0) {
        printf("\n%s\n", parser->description);
    }
    
    printf("\nOptions:\n");
    printf("  -h, --help          Show this help message and exit\n");
    
    for (int i = 0; i < parser->num_args; i++) {
        arg_t *arg = &parser->args[i];
        
        printf("  ");
        if (arg->short_name) {
            printf("-%c, ", arg->short_name);
        } else {
            printf("    ");
        }
        
        printf("--%-15s ", arg->name);
        
        if (strlen(arg->help) > 0) {
            printf("%s", arg->help);
        }
        
        if (arg->required) {
            printf(" (required)");
        } else {
            switch (arg->type) {
                case ARG_TYPE_STRING:
                    if (strlen(arg->default_value.str_default) > 0) {
                        printf(" (default: %s)", arg->default_value.str_default);
                    }
                    break;
                case ARG_TYPE_INT:
                    printf(" (default: %d)", arg->default_value.int_default);
                    break;
                case ARG_TYPE_FLOAT:
                    printf(" (default: %.2f)", arg->default_value.float_default);
                    break;
                case ARG_TYPE_BOOL:
                    printf(" (default: %s)", arg->default_value.bool_default ? "true" : "false");
                    break;
                case ARG_TYPE_FLAG:
                    printf(" (flag)");
                    break;
            }
        }
        
        printf("\n");
    }
}

// Print usage
void argparse_print_usage(argparser_t *parser) {
    printf("Usage: %s [OPTIONS] [ARGS...]\n", parser->program_name);
}
