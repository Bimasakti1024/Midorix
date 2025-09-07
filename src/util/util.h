// src/util/util.c
#ifndef UTIL_H
#define UTIL_H

#include <cjson/cJSON.h>
#include <stddef.h>
#include <wordexp.h>

// Macro
//  Default Configuration
#define DEFAULT_PROMPT "[Midorix]> "
#define DEFAULT_PREFIX "."
#define DEFAULT_WELCOME_MSG "Welcome to Midorix!\n"

//  Colors
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"

//  Log Tags
#define WARN_TAG "[" YELLOW "WARNING" RESET "] "
#define ERR_TAG "[" RED "ERROR" RESET "] "
#define INFO_TAG "[" GREEN "INFO" RESET "] "

int readjson(const char *jsonf,
			 cJSON	   **jsonout); // Parse JSON configuration file

int chkfexist(const char *filename); // Check file existence
int chkbin(const char *prog);		 // Check binary file existence in PATH
int dir_exist(const char *path);	 // Check if a direcory exist

// String Manipulations
char *concat(const char *first, ...);
int	  ssplit(const char *input, wordexp_t *dest);

void execcmd(char *argv[]); // Execute command in 'The Unix Way'

#endif
