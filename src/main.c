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
#include "ext/cJSON/cJSON.h"
#include "chandl/chandl.h"
#include "chandl/cmd_handle.h"

#define DEFAULT(value, fallback) ((value) ? (value) : (fallback))

// Global
char *prefix, *prompt, *welcome_msg, *command;
size_t prefixlen;
static cJSON* config = NULL;

// Signal handler
void sighandler(int sig) {
	fprintf(stdout, "\n[Midorix] Received signal: %d\n", sig);
}

// Execute function
void execute(char* command);

void cleanup() {
	if (config) cJSON_Delete(config);
	if (prefix) free(prefix);
	if (prompt) free(prompt);
	if (welcome_msg) free(welcome_msg);
	if (command) free(command);
	clear_history();
}

// Main
int main(int argc, char* argv[]) {
	atexit(cleanup);
	setvbuf(stdout, NULL, _IONBF, 0);

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
	snprintf(configfn, sizeof(configfn), "%s/.midorixc.json", home);

	// Init configuration
	cmdh_init_config(&config, configfn);

	// Welcome
	welcome_msg = strdup(cJSON_GetObjectItem(config, "welcome_msg")->valuestring);
	printf("%s", welcome_msg);

	// Prompt
	prompt = strdup(cJSON_GetObjectItem(config, "prompt")->valuestring);
	if (!prompt) {
		perror("strdup");
		return 1;
	}

	// Prefix
	prefix = strdup(cJSON_GetObjectItem(config, "prefix")->valuestring);
	if (!prefix) {
		perror("strdup");
		return 1;
	}
	prefixlen = strlen(prefix);

	// Set stifle_history(max history)
	stifle_history(cJSON_GetObjectItem(config, "max_history")->valueint);

	// Main loop
	while (1) {
		char* input = readline(prompt);
		if (!input) {
			fprintf(stderr, "[Midorix] Error: Input error.\n");
			break;
		}

		int inputl = strlen(input);
		if (inputl > 0) {
			add_history(input);
			command = strdup(input);
			if (!command) {
				perror("strdup");
				return 1;
			}
			execute(command);
			free(command);
		}
		free(input);
	}

	return 0;
}

// Execute command
void execute(char* command) {
	if (command == NULL || strlen(command) == 0) return;

	if (strlen(command) > prefixlen && strncmp(command, prefix, prefixlen) == 0
		&& command[prefixlen] != '\0') {
		// Midorix command
		char* pure_command = strdup(command + prefixlen);
		if (!pure_command) {
			perror("strdup");
			return;
		}

		wordexp_t arg;
		if (ssplit(pure_command, &arg) != 0) {
			free(pure_command);
			perror("wordexp");
			return;
		}
		free(pure_command);

		int found = 0;
		for (int i = 0; command_table[i].cmd != NULL; i++) {
			if (strcmp(command_table[i].cmd, arg.we_wordv[0]) == 0 ||
				strcmp(command_table[i].shortcut, arg.we_wordv[0]) == 0) {
				found = 1;
				printf("[Midorix] Executing Midorix command: %s\n", command_table[i].cmd);
				command_table[i].handler(arg.we_wordc, arg.we_wordv);
				break;
			}
		}

		if (!found) printf("[Midorix] Unknown command: %s\n", arg.we_wordv[0]);
		wordfree(&arg);
	} else {
		// System fallback
		wordexp_t arg;
		if (ssplit(command, &arg) != 0) {
			perror("wordexp");
			return;
		}

		if (!arg.we_wordv) {
			fprintf(stderr, "[Midorix] wordexp failed.\n");
			return;
		}

		printf("[Midorix] Executing system command: %s\n", command);
		execcmd(arg.we_wordv);
		wordfree(&arg);
	}
}

