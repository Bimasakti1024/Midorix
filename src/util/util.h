// src/util/util.c
#ifndef UTIL_H
#define UTIL_H

#define _GNU_SOURCE
#include "../ext/cJSON/cJSON.h"
#include <stddef.h>
#include <wordexp.h>

// Macros
//  Default Configuration
#define DEFAULT_PROMPT "[Midorix]> "
#define DEFAULT_PREFIX "."
#define DEFAULT_WELCOME_MSG "Welcome to Midorix!\n"

int readjson(const char *jsonf,
			 cJSON	   **jsonout); // Parse JSON configuration file

int chkfexist(const char *filename); // Check file existence

// String Manipulations
void feval(const char *input, char *output, size_t maxlen);
void sfeval(const char *input, char *output, size_t maxlen);
int	 ssplit(const char *input, wordexp_t *dest);

void flush_stdin(); // Flush stdin

void execcmd(char *argv[]); // Execute command in 'The Unix Way'

#endif
