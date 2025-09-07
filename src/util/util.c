// src/util/util.c
#include "util.h"
#include <cjson/cJSON.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

// Read a file
char *readf(const char *path) {
	// Open the file
	FILE *f = fopen(path, "rb");

	// If failed to open will return a NULL
	if (!f)
		return NULL;

	// Move the file pointer to the end of the file
	fseek(f, 0, SEEK_END);

	// Get the current file pointer position, which is the
	// size of the file in bytes
	long size = ftell(f);

	// Moves the current file pointer positon to the
	// beginning, so it can read from the start
	rewind(f);

	// Allocate a buffer to hold the file content
	// and a null-terminator.
	// the cast (char *) is used to convert void*
	// returned by malloc to a char*
	char *buffer = (char *)malloc(size + 1);

	// Read the file and save it to the buffer
	// with each element size is 1 byte
	fread(buffer, 1, size, f);

	// Null terminate the buffer
	buffer[size] = '\0';

	// Close the file and return the buffer
	fclose(f);
	return buffer;
}

// Read a JSON file and save the parsed JSON to jsonout
int readjson(const char *jsonf, cJSON **jsonout) {
	// Read the JSON file
	char *buffer = readf(jsonf);

	// Parse the JSON
	cJSON *parsed = cJSON_Parse(buffer);

	// Free the buffer
	free(buffer);

	// Check if it successfully parsed the JSON
	if (!parsed) {
		return 1;
	}

	// Duplicate the cJSON Object
	cJSON *dup = cJSON_Duplicate(parsed, 1);

	// Delete the old cJSON Object
	cJSON_Delete(parsed);

	// Check if the duplication process is success
	if (!dup)
		return 1;

	// Change the jsonout to the parsed JSON and return
	*jsonout = dup;
	return 0;
}

// Check if a file exist
int chkfexist(const char *filename) {
	return !access(filename, F_OK);
}

// Check if a binary file exist and executable in $PATH
int chkbin(const char *prog) {
	// Get $PATH and save it to path_env
	char *path_env = getenv("PATH");

	// Safety
	if (!path_env) {
		perror("getenv");
		return -1;
	}

	// Duplicate the path_env
	char *pathv = strdup(path_env);
	if (!pathv) {
		perror("strdup");
		return -1;
	}

	// Get individual path
	char *pathd = strtok(pathv, ":");

	// Iterate each path
	while (pathd) {
		// Construct the full path
		char *fullpath = concat(pathd, "/", prog, NULL);

		// Check if the file is executable
		if (access(fullpath, X_OK) == 0) {
			free(fullpath);
			free(pathv);
			return 1;
		}

		// If not executable will free fullpath
		// and to the next iteration
		free(fullpath);
		pathd = strtok(NULL, ":");
	}

	// Free the path_env duplicate
	free(pathv);
	return 0;
}

// Concat a string
char *concat(const char *first, ...) {
	// Get all the arguments
	va_list args;
	va_start(args, first);

	// Get the length
	size_t		total_len = strlen(first);
	const char *s;

	// Count total length
	// Copy the arguments
	va_list args_copy;
	va_copy(args_copy, args);

	// Iterate each arguments
	while ((s = va_arg(args_copy, const char *)) != NULL) {
		// Add the length of each arguments
		// and add it to the total_len
		total_len += strlen(s);
	}

	// End arg_copy
	va_end(args_copy);

	// Allocate buffer
	char *result = malloc(total_len + 1);
	if (!result) {
		va_end(args);
		return NULL;
	}

	// Set the result to first
	strcpy(result, first);

	// Iterate each argument
	while ((s = va_arg(args, const char *)) != NULL) {
		// Concatenate each argument
		strcat(result, s);
	}

	va_end(args);
	return result;
}

// Split the input using wordexp and set the dest
// as the output
int ssplit(const char *input, wordexp_t *dest) {
	// Split the input and save it to dest
	int ret = wordexp(input, dest, 0);

	// Check if the splitting process failed
	if (ret != 0) {
		// Set wordc to 0 and wordv to NULL
		dest->we_wordc = 0;
		dest->we_wordv = NULL;

		return ret;
	}
	return 0;
}

// Check if a directory exist
int dir_exist(const char *path) {
	struct stat st;

	return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

// Execute a command
void execcmd(char *argv[]) {
	// Print the command
	for (int i = 0; argv[i] != NULL; i++) {
		printf("%s", argv[i]);
		if (argv[i + 1] != NULL)
			printf(" ");
	}

	puts("");

	// Fork
	pid_t pid = fork();

	// Check the pid
	if (pid == 0) {
		// Child Process
		execvp(argv[0], argv);
		perror("execvp");
		exit(EXIT_FAILURE);
	} else if (pid > 0) {
		// Parent Process
		int status;

		// Wait until the execution ends
		waitpid(pid, &status, 0);

		// Print the exit code or terminate signal
		if (WIFEXITED(status)) {
			printf("Command exited with code: %d\n", WEXITSTATUS(status));
		} else if (WTERMSIG(status)) {
			printf("Command terminated with signal: %d\n", WTERMSIG(status));
		}
	} else {
		perror("fork");
	}
}
