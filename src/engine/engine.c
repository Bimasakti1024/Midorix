#include "engine.h"
#include "../chandl/chandl.h"
#include "../core/core.h"

#include "../util/util.h"
#include <cjson/cJSON.h>
#include <lauxlib.h>
#include <linenoise.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

int execute(const char *command) {
	int exitc = 0;
	if (command == NULL || strlen(command) == 0)
		return 0;

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
	char *s_ccmd = append(ccmd_path, "/");
	char *ccmd	 = append(s_ccmd, cmdname);
	free(s_ccmd);
	if (!ccmd) {
		fprintf(stderr, "Failed to append string when constructing path.\n");
		exitc = 1;
		goto execute_clean;
	}

	if (!chkfexist(ccmd)) {
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
			exitc = 1;
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
			exitc = 1;
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
	return exitc;
}

int midorix_cli() {
	int exitc = 0;
	// Autoinit
	cJSON *autoinit_project =
		cJSON_GetObjectItemCaseSensitive(config, "autoinit_project");
	if (cJSON_IsBool(autoinit_project) && cJSON_IsTrue(autoinit_project) &&
		chkfexist("mdrxproject.lua")) {
		execute(".proman init");
	}

	// Welcome
	cJSON *welcome_msgj = cJSON_GetObjectItem(config, "welcome_msg");
	if (cJSON_IsString(welcome_msgj)) {
		welcome_msg =
			strdup(cJSON_GetObjectItem(config, "welcome_msg")->valuestring);
		printf("%s", welcome_msg);
	}

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
			exitc = 1;
			break;
		}

		int inputl = strlen(input);
		if (inputl > 0) {
			linenoiseHistoryAdd(input);
			char *command = strdup(input);
			if (!command) {
				perror("strdup");
				free(command);
				continue;
			}
			execute(command);
			free(command);
		}
		free(input);
	}
	return exitc;
}
