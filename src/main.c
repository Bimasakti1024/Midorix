#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <signal.h>
#include "util/util.h"
#include "chandl/chandl.h"
#include "chandl/cmd_handle.h"

#define DEFAULT(value, fallback) ((value) ? (value) : (fallback))

// Signal Handler
void sighandler(int sig) {
	fprintf(stdout, "Received signal: %d\n", sig);
}

// Main function
int main(int argc, char* argv[]) {
	// Setup signal handler
	signal(SIGINT, sighandler);

	// Load Configuration
	char* home = getenv("HOME");
	if (home == NULL) {
		fprintf(stderr, "Error: HOME environment variable not set.\n");
		return 1;
	}

	printf("[Midorix] Found home path: %s\n", home);

	char configfn[PATH_MAX];
	snprintf(configfn, sizeof(configfn), "%s/.midorixc", home);

	// Initialize dictionary
	Dictionary config = {0};

	// Check configuration file existence
	if (chkfexist(configfn) == -1) {
		fprintf(stderr, "Warning: configuration file does not exist, using default configuration.\n");
	} else {
		printf("[Midorix] Found configuration file: %s\n", configfn);
		printf("[Midorix] Loading Configuration...\n");

		// Read the configuration file
		if (readini(configfn, &config) == 1) {
			fprintf(stderr, "Error: Failed to read configuration file.\n");
			return 1;
		}

		// feval all value
		for (int i = 0; i < config.count; i++) {
			char* value = config.value[i];
			sfeval(value, value, sizeof(value));
		}

		cmdh_init_config(&config);	// Load the configuration to the command handler

		printf("[Midorix] Configuration loaded.\n");
	}

	// Set welcome message
	char welcome_msg[MAX_VALUE];
	feval(DEFAULT(dict_get(&config, "welcome_msg"), DEFAULT_WELCOME_MSG), welcome_msg, sizeof(welcome_msg));
	printf("%s", welcome_msg);

	// Set prompt
	char prompt[MAX_VALUE];
	feval(DEFAULT(dict_get(&config, "prompt"), DEFAULT_PROMPT), prompt, sizeof(prompt));

	// Set prefix
	char prefix[MAX_VALUE];
	feval(DEFAULT(dict_get(&config, "prefix"), DEFAULT_PREFIX), prefix, sizeof(prefix));

	// Main loop
	while (1) {
		char command[4096 + 1];
		printf("%s", prompt);
		if (fgets(command, sizeof(command), stdin) == NULL) {
			fprintf(stderr, "Error: Input error.\n");
			return -1;
		}

		if (strchr(command, '\n') == NULL) { flush_stdin(); }	 	// Handle Buffer Overflow
		command[strcspn(command, "\n")] = '\0';		 	 	// Null Terminate

		if (strlen(command) == 0) {				 	// Handle Empty
			continue;
		} else if (strncmp(command, prefix, strlen(prefix)) == 0) {	// Check for prefix
			// Execute Midorix Command
			memmove(command, command + 1, strlen(command)); 	// Move to the left(remove the dot)

			wordexp_t arg;						// Setup Argument
			ssplit(command, &arg);					// Split

			int cmdFound = 0;					// Flag
			// Find the command
			for (int i = 0; command_table[i].cmd != NULL; i++) {
				if ((strcmp(command_table[i].cmd, arg.we_wordv[0]) == 0) || (strcmp(command_table[i].shortcut, arg.we_wordv[0]) == 0)) {
					cmdFound = 1;
					command_table[i].handler(arg.we_wordc, arg.we_wordv);
					break;
				}
			}
			if (cmdFound) {
				continue;
			} else {
				printf("Unknown Midorix Command: %s\n", arg.we_wordv[0]);
			}
		} else {
			printf("Fallback to system call.\n");
			wordexp_t arg;
			ssplit(command, &arg);

			if (arg.we_wordv == NULL) {			  	  // Error Handler
				fprintf(stderr, "wordexp failed.\n");
			}
			execcmd(arg.we_wordv);					  // Execute
		}
	}

	return 0;
}

