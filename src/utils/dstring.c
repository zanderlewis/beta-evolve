#include "beta_evolve.h"
#include <stdarg.h>

// Create a new dynamic string with initial capacity
dstring_t* dstring_create(size_t initial_capacity) {
    dstring_t* ds = malloc(sizeof(dstring_t));
    if (!ds) return NULL;
    
    if (initial_capacity < 64) initial_capacity = 64; // Minimum capacity
    
    ds->data = malloc(initial_capacity);
    if (!ds->data) {
        free(ds);
        return NULL;
    }
    
    ds->data[0] = '\0';
    ds->length = 0;
    ds->capacity = initial_capacity;
    
    return ds;
}

// Destroy a dynamic string and free all memory
void dstring_destroy(dstring_t* ds) {
    if (!ds) return;
    
    if (ds->data) {
        free(ds->data);
    }
    free(ds);
}

// Expand the capacity of a dynamic string if needed
static int dstring_expand(dstring_t* ds, size_t needed_capacity) {
    if (ds->capacity >= needed_capacity) return 0;
    
    // Double capacity or use needed_capacity, whichever is larger
    size_t new_capacity = ds->capacity * 2;
    if (new_capacity < needed_capacity) {
        new_capacity = needed_capacity;
    }
    
    char* new_data = realloc(ds->data, new_capacity);
    if (!new_data) return -1;
    
    ds->data = new_data;
    ds->capacity = new_capacity;
    
    return 0;
}

// Append a string to the dynamic string
int dstring_append(dstring_t* ds, const char* str) {
    if (!ds || !str) return -1;
    
    size_t str_len = strlen(str);
    size_t needed_capacity = ds->length + str_len + 1;
    
    if (dstring_expand(ds, needed_capacity) != 0) {
        return -1;
    }
    
    strcpy(ds->data + ds->length, str);
    ds->length += str_len;
    
    return 0;
}

// Append formatted string to the dynamic string
int dstring_append_format(dstring_t* ds, const char* format, ...) {
    if (!ds || !format) return -1;
    
    va_list args, args_copy;
    va_start(args, format);
    va_copy(args_copy, args);
    
    // First, determine how much space we need
    int needed_len = vsnprintf(NULL, 0, format, args);
    va_end(args);
    
    if (needed_len < 0) {
        va_end(args_copy);
        return -1;
    }
    
    size_t needed_capacity = ds->length + needed_len + 1;
    
    if (dstring_expand(ds, needed_capacity) != 0) {
        va_end(args_copy);
        return -1;
    }
    
    vsnprintf(ds->data + ds->length, needed_len + 1, format, args_copy);
    va_end(args_copy);
    
    ds->length += needed_len;
    
    return 0;
}

// Clear the dynamic string content but keep the allocated memory
void dstring_clear(dstring_t* ds) {
    if (!ds || !ds->data) return;
    
    ds->data[0] = '\0';
    ds->length = 0;
}

// Get the string content (read-only)
char* dstring_get(const dstring_t* ds) {
    if (!ds || !ds->data) return NULL;
    return ds->data;
}
