// src/main.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cjson/cJSON.h>
#include <lauxlib.h>
#include <limits.h>
#include <linenoise.h>
#include <lua.h>
#include <lualib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>

#include "chandl/chandl.h"
#include "chandl/cmd_handle.h"
#include "util/util.h"

// Global
char		 *prefix, *prompt, *welcome_msg, *command;
size_t		  prefixlen;
static cJSON *config = NULL;
char		 *config_path, *configfn, *ccmd_path;
lua_State	 *L = NULL; // For user-defined commands made using LuA

// Cleanup
void cleanup() {
	if (config)
		cJSON_Delete(config);
	if (prefix)
		free(prefix);
	if (prompt)
		free(prompt);
	if (welcome_msg)
		free(welcome_msg);
	if (config_path)
		free(config_path);
	if (ccmd_path)
		free(ccmd_path);
	if (configfn)
		free(configfn);
	if (command)
		free(command);
	if (L)
		lua_close(L);
}

// Signal handler
void sighandler(int sig) {
	printf("[Midorix] Received signal: %d\n", sig);
	char *ans = linenoise("Exit(y/n) ");
	int	  b	  = 0;
	if (ans && (ans[0] == 'y' || ans[0] == 'Y')) {
		b = 1;
	}
	if (ans)
		free(ans);
	if (b) {
		cleanup();
		exit(0);
	};
}

// Execute function
void execute(const char *command);

int dir_exist(const char *path) {
	struct stat st;
	return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

// Main
int main(int argc, char *argv[]) {
	atexit(cleanup);
	setvbuf(stdout, NULL, _IONBF, 0);

	// Signal handler
	signal(SIGINT, sighandler);

	// Get home path
	const char *home = getenv("HOME");
	if (!home) {
		fprintf(stderr,
				"[Midorix] Error: HOME environment variable not set.\n");
		return 1;
	}

	// Get configuration directory path
	config_path = append(home, "/.config/midorix");
	if (!config_path) {
		perror("malloc");
		return 1;
	}
	if (!dir_exist(config_path)) {
		fprintf(stderr,
				"[Midorix] Error: Configuration directory not found.\n");
	}

	configfn = append(config_path, "/config.json");
	if (!configfn) {
		return 1;
	}

	ccmd_path = append(config_path, "/custom_command");
	if (!ccmd_path) {
		perror("malloc");
		return 1;
	}
	if (!dir_exist(ccmd_path))
		fprintf(stderr, "[Midorix] Warning: custom command does not exist.\n");

	// Init configuration
	cmdh_init_config(&config, configfn);

	// Welcome
	welcome_msg =
		strdup(cJSON_GetObjectItem(config, "welcome_msg")->valuestring);
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

	// Set max history
	linenoiseHistorySetMaxLen(
		cJSON_GetObjectItem(config, "max_history")->valueint);

	// Auto-start command
	cJSON *autostart = cJSON_GetObjectItem(config, "autostart");
	if (cJSON_IsArray(autostart)) {
		int size = cJSON_GetArraySize(autostart);
		for (int i = 0; i < size; i++) {
			cJSON *value = cJSON_GetArrayItem(autostart, i);
			printf("%s\n", value->valuestring);
			execute(value->valuestring);
		}
	}

	// Main loop
	while (1) {
		char *input = linenoise(prompt);
		if (!input) {
			fprintf(stderr, "[Midorix] Error: Input error.\n");
			break;
		}

		int inputl = strlen(input);
		if (inputl > 0) {
			linenoiseHistoryAdd(input);
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
void execute(const char *command) {
	if (command == NULL || strlen(command) == 0)
		return;

	int		  found = 0;
	wordexp_t arg;
	if (ssplit(command, &arg) != 0) {
		perror("wordexp");
	}

	char *cmdname = strdup(arg.we_wordv[0]);
	if (!cmdname) {
		perror("strdup");
		goto execute_clean;
	}

	if (strncmp(cmdname, prefix, prefixlen) == 0) {
		// Midorix command
		memmove(cmdname, cmdname + prefixlen, strlen(cmdname + prefixlen) + 1);
		for (int i = 0; command_table[i].cmd != NULL; i++) {
			if (strcmp(command_table[i].cmd, cmdname) == 0 ||
				strcmp(command_table[i].alias, cmdname) == 0) {
				found = 1;
				command_table[i].handler(arg.we_wordc, arg.we_wordv);
				goto execute_clean;
			}
		}
	}
	char *ccmd = append(ccmd_path, cmdname);
	if (!ccmd) {
		goto execute_clean;
	}

	if (chkfexist(ccmd)) {
		free(ccmd);
	} else {
		found = 1;
		if (L)
			lua_close(L);
		L = luaL_newstate();
		luaL_openlibs(L);

		if (luaL_dofile(L, ccmd) != LUA_OK) {
			fprintf(stderr,
					"Error encountered when loading custom command: %s\n",
					lua_tostring(L, -1));
			lua_pop(L, 1);
			free(ccmd);
			goto execute_clean;
		} else {
			found = 1;
		}
		free(ccmd);

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
	if (!found) {
		// System fallback
		execcmd(arg.we_wordv);
	}
execute_clean:
	wordfree(&arg);
	if (cmdname)
		free(cmdname);
	return;
}
