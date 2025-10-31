// src/core/core.c
#include "core.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <cjson/cJSON.h>
#include <lauxlib.h>
#include <limits.h>
#include <linenoise.h>
#include <lua.h>
#include <lualib.h>
#include <signal.h>
#include <string.h>

#include "../util/util.h"
#include "../chandl/cmd_handle.h"
#include "../engine/engine.h"

char *prefix = NULL;
char *prompt = NULL;
char *welcome_msg = NULL;
char *command = NULL;
size_t prefixlen = 0;
cJSON *config = NULL;
char *config_path = NULL;
char *configfn = NULL;
char *shortcutfn = NULL;
char *ccmd_path = NULL;
lua_State *L = NULL; // For user-defined commands made using LuA

extern void rscmd_init_shortcut(const char *shortcutfn);

// Cleanup
void midorix_cleanup() {
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
		midorix_cleanup();
		exit(0);
	};
}

int midorix_init(int iie, const char *cfgpth ) {
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
	if (!cfgpth) {
		config_path = concat(home, "/.config/midorix", NULL);
	} else {
		config_path = strdup(cfgpth);
	}
	if (!config_path) {
		perror("malloc");
		if (!iie)
			return 2;
	}
	if (!dir_exist(config_path)) {
		fprintf(stderr,
				"[Midorix] Error: Configuration directory not found.\n");
		if (!iie)
			return 3;
	}

	configfn = concat(config_path, "/config.json", NULL);
	if (!configfn) {
		fprintf(stderr,
		   		 "[Midorix] Error: Failed to get configuration file path.\n");
		perror("malloc");
		if (!iie)
			return 4;
	}

	ccmd_path = concat(config_path, "/custom_command", NULL);
	if (!ccmd_path) {
		fprintf(stderr,
		  		"[Midorix] Warning: Failed to get custom command directory path.\n");
		perror("malloc");
	}
	if (ccmd_path && !dir_exist(ccmd_path)) {
		fprintf(stderr, "[Midorix] Warning: custom command does not exist.\n");
	}

	// Init configuration
	cmdh_init_config(&config, configfn);

	shortcutfn = concat(config_path, "/shortcut.json", NULL);
	if (!shortcutfn) {
		fprintf(stderr,
		  		"[Midorix] Warning: Failed to get shortcut file path.\n");
		perror("malloc");
		if (!iie)
			return 5;
	}
	rscmd_init_shortcut(shortcutfn);

	// Prompt
	prompt = strdup(cJSON_GetObjectItem(config, "prompt")->valuestring);
	if (!prompt) {
		perror("strdup");
		if (!iie)
			return 5;
	}

	// Prefix
	prefix = strdup(cJSON_GetObjectItem(config, "prefix")->valuestring);
	if (!prefix) {
		perror("strdup");
		if (!iie)
			return 6;
	}
	prefixlen = strlen(prefix);

	// Autoinit
	cJSON *autoinit_project =
		cJSON_GetObjectItemCaseSensitive(config, "autoinit_project");
	if (cJSON_IsBool(autoinit_project) && cJSON_IsTrue(autoinit_project) &&
		chkfexist("mdrxproject.lua")) {
		execute(".proman init");
	}

	// Welcome
	welcome_msg =
		strdup(cJSON_GetObjectItem(config, "welcome_msg")->valuestring);

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
	return 0;
}

