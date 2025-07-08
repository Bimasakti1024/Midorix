#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <string.h>
#include <limits.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "util/util.h"
#include "chandl/chandl.h"
#include "chandl/cmd_handle.h"

#define DEFAULT(value, fallback) ((value) ? (value) : (fallback))

// Global
char prefix[MAX_VALUE], prompt[MAX_VALUE];
size_t prefixlen;

// Signal handler
void sighandler(int sig) {
	fprintf(stdout, "\n[Midorix] Received signal: %d\n", sig);
}

// Execute function
void execute(char* command);

// Main
int main(int argc, char* argv[]) {
	// Signal handler
	signal(SIGINT, sighandler);

	// Config path
	const char* home = getenv("HOME");
	if (!home) {
		fprintf(stderr, "[Midorix] Error: HOME environment variable not set.\n");
		return 1;
	}

	printf("[Midorix] Found home path: %s\n", home);

	char configfn[PATH_MAX];
	snprintf(configfn, sizeof(configfn), "%s/.midorixc", home);

	// Init dictionary
	Dictionary config = {0};
	cmdh_init_config(&config, configfn);

	// Welcome
	char welcome_msg[MAX_VALUE];
	sfeval(DEFAULT(dict_get(&config, "welcome_msg"), DEFAULT_WELCOME_MSG), welcome_msg, sizeof(welcome_msg));
	printf("%s", welcome_msg);

	// Prompt
	sfeval(DEFAULT(dict_get(&config, "prompt"), DEFAULT_PROMPT), prompt, sizeof(prompt));

	// Prefix
	sfeval(DEFAULT(dict_get(&config, "prefix"), DEFAULT_PREFIX), prefix, sizeof(prefix));
	prefixlen = strlen(prefix);

	// Main loop
	char command[4096];
	while (1) {
		char* input = readline(prompt);
		if (!input) {
			fprintf(stderr, "[Midorix] Error: Input error.\n");
			break;
		}

		if (strlen(input) > 0) {
			add_history(input);
			strncpy(command, input, sizeof(command) - 1);
			command[sizeof(command) - 1] = '\0';
			execute(command);
		}
		free(input);
	}

	return 0;
}

// Execute command
void execute(char* command) {
	if (command == NULL || strlen(command) == 0) return;

	if (strlen(command) > prefixlen && strncmp(command, prefix, prefixlen) == 0) {
		// Midorix command
		memmove(command, command + prefixlen, strlen(command) - prefixlen + 1);

		wordexp_t arg;
		ssplit(command, &arg);

		int found = 0;
		for (int i = 0; command_table[i].cmd != NULL; i++) {
			if (strcmp(command_table[i].cmd, arg.we_wordv[0]) == 0 ||
				strcmp(command_table[i].shortcut, arg.we_wordv[0]) == 0) {
				found = 1;
				command_table[i].handler(arg.we_wordc, arg.we_wordv);
				break;
			}
		}

		if (!found)
			printf("[Midorix] Unknown command: %s\n", arg.we_wordv[0]);

	} else {
		// System fallback
		wordexp_t arg;
		ssplit(command, &arg);

		if (!arg.we_wordv) {
			fprintf(stderr, "[Midorix] wordexp failed.\n");
			return;
		}

		printf("[Midorix] Executing system command: %s\n", command);
		execcmd(arg.we_wordv);
	}
}

