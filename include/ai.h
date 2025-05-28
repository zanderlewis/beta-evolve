#ifndef AI_H
#define AI_H

#include "beta_evolve.h"

// AI client functions for calling models
char* call_ai_model(const char* prompt, agent_type_t agent, config_t *config);
char* validate_and_clean_response(const char* response);

#endif // AI_H
