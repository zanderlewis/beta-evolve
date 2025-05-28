#ifndef CONFIG_H
#define CONFIG_H

#include "beta_evolve.h"

// Configuration management functions
int load_config(config_t *config, const char *config_file);
int load_problem_prompt_file(config_t *config, const char *prompt_file_path);
void free_config(config_t *config);
int is_local_server(const char *endpoint);

#endif // CONFIG_H
