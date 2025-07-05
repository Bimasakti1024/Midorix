#ifndef UTIL_H
#define UTIL_H

#define _GNU_SOURCE
#include <stddef.h>
#include <wordexp.h>

// Macros
//  Dictionary
#define MAX_ENTRY 100	// Maximum number of key-value pairs
#define MAX_KEY 128	// Maximum length for keys
#define MAX_VALUE 256	// Maximum length for values
//  Default Configuration
#define DEFAULT_PROMPT		"[Midorix]> "
#define DEFAULT_PREFIX		"."
#define DEFAULT_EDITOR		"vim"
#define DEFAULT_EXECUTOR	"gcc"
#define DEFAULT_DEBUGGER	"gdb"
#define DEFAULT_WELCOME_MSG	"Welcome to Midorix!\n"

typedef struct {
	char key[MAX_ENTRY][MAX_KEY];
	char value[MAX_ENTRY][MAX_VALUE];
	int count;
} Dictionary;

// Dictionary functions
void dict_set(Dictionary* dict, const char* key, const char* value);
const char* dict_get(Dictionary* dict, const char* key);

int readini(const char* inifn, Dictionary* dict); // Parse ini configuration file

int chkfexist(const char* filename); // Check file existence

// String Manipulations
void rmquote(const char* src, char* dest, size_t maxlen);
void unescape(const char* input, char* output, size_t maxlen);
void feval(const char* input, char* output, size_t maxlen);
void ssplit(const char* input, wordexp_t* dest);

void flush_stdin(); // Flush stdin

void execcmd(char* argv[]); // Execute command in 'The Unix Way'

#endif

