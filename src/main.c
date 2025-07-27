#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <string.h>
#include <limits.h>
#include <signal.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "util/util.h"
#include "ext/cJSON/cJSON.h"
#include "chandl/chandl.h"
#include "chandl/cmd_handle.h"

// Global
char *prefix, *prompt, *welcome_msg, *command;
size_t prefixlen;
static cJSON* config = NULL;
char config_path[PATH_MAX], ccmd_path[PATH_MAX];
lua_State *L = NULL;		// For user-defined commands made using LuA

// Signal handler
void sighandler(int sig) {
	fprintf(stdout, "\n[Midorix] Received signal: %d\n", sig);
}

// Execute function
void execute(char* command);

int dir_exist(const char* path) {
	struct stat st;
	return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

void cleanup() {
	if (config) cJSON_Delete(config);
	if (prefix) free(prefix);
	if (prompt) free(prompt);
	if (welcome_msg) free(welcome_msg);
	if (command) free(command);
	if (L) lua_close(L);
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
	snprintf(config_path, sizeof(config_path), "%s/.config/midorix", home);
	if (!dir_exist(config_path)) {
		fprintf(stderr, "[Midorix] Error: Configuration directory not found.\n");
	}

	char configfn[PATH_MAX];
	snprintf(configfn, sizeof(configfn), "%s/config.json", config_path);

	snprintf(ccmd_path, sizeof(ccmd_path), "%s/custom_command/", config_path);
	if (!dir_exist(ccmd_path)) fprintf(stderr, "[Midorix] Warning: custom command does not exist.\n");

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
		char* cmdname = strdup(arg.we_wordv[0]);

		int found = 0;
		for (int i = 0; command_table[i].cmd != NULL; i++) {
			if (strcmp(command_table[i].cmd, cmdname) == 0 ||
				strcmp(command_table[i].shortcut, cmdname) == 0) {
				found = 1;
				printf("[Midorix] Executing Midorix command: %s\n", command_table[i].cmd);
				command_table[i].handler(arg.we_wordc, arg.we_wordv);
				break;
			}
		}

		if (!found) {
			if (L) lua_close(L);
			L = luaL_newstate();
			luaL_openlibs(L);

			size_t size = strlen(ccmd_path) + strlen(cmdname) + 1;
			char* ccmd = malloc(size);
			if (!ccmd) {
				perror("malloc");
				exit(1);
			}
			snprintf(ccmd, size + 1, "%s/%s", ccmd_path, cmdname);

			if (luaL_dofile(L, ccmd) != LUA_OK) {
				fprintf(stderr, "Error encountered when loading custom command: %s\n", lua_tostring(L, -1));
				lua_pop(L, 1);
				goto execute_clean;
			} else { found = 1; }

			// Push argc
			lua_getglobal(L, "main");

			if (!lua_isfunction(L, -1)) {
			    fprintf(stderr, "%s is not a function!\n", cmdname);
				lua_pop(L, 1);
				goto execute_clean;
			}
			lua_pushinteger(L, arg.we_wordc);

			// Push argv
			lua_newtable(L);
			for (int i = 0; i < arg.we_wordc; i++) {
			    lua_pushinteger(L, i + 1);
				lua_pushstring(L, arg.we_wordv[i]);
				lua_rawset(L, -3);
			}

			// Execute
			if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
				fprintf(stderr, "Runtime Error: %s\n", lua_tostring(L, -1));
				lua_pop(L, 1);
			}
		}
		if (!found) fprintf(stderr, "[Midorix]: Command %s not found.", cmdname);
		execute_clean:
			wordfree(&arg);
			if (cmdname) free(cmdname);
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
