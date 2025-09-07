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

// Execute Function
int execute(const char *command) {
	if (command == NULL || strlen(command) == 0)
		return 0;
	int		  exitc = 0; // Exit Code
	int		  found = 0; // Found Flag
	wordexp_t arg;		 // Argument

	// Split the command into arguments
	if (ssplit(command, &arg) != 0) {
		perror("wordexp");
		return -1;
	}

	// Get the command name
	char *cmdname = strdup(arg.we_wordv[0]);

	// Check for prefix
	if (strncmp(cmdname, prefix, prefixlen) == 0) {
		// Execute Midorix command

		// Remove the prefix
		memmove(cmdname, cmdname + prefixlen, strlen(cmdname + prefixlen) + 1);

		// Find the command
		for (int i = 0; command_table[i].cmd != NULL; i++) {
			// Check if the command name or alias match
			// the command name in the input
			if (strcmp(command_table[i].cmd, cmdname) == 0 ||
				strcmp(command_table[i].alias, cmdname) == 0) {
				found = 1;

				// Execute the command
				command_table[i].handler(arg.we_wordc, arg.we_wordv);

				// Exit
				goto execute_clean;
			}
		}
	}

	// Custom command fallback
	// Construct path for the custom command
	char *ccmd = concat(ccmd_path, "/", cmdname, NULL);

	// Check the concat result
	if (!ccmd) {
		fprintf(stderr,
				ERR_TAG "Failed to construct path for custom command.\n");
		exitc = 1;
		goto execute_clean;
	}

	// Check if the custom command does not exist
	if (!chkfexist(ccmd)) {
		// Free if does not exist
		free(ccmd);
	} else {
		// If exist, will execute
		found = 1;

		// Reset Lua state
		if (L)
			lua_close(L);
		L = luaL_newstate();
		luaL_openlibs(L);

		// Run the custom command (file)
		if (luaL_dofile(L, ccmd) != LUA_OK) {
			// Debug print
			fprintf(stderr,
					ERR_TAG
					"Error encountered when loading custom command: %s\n",
					lua_tostring(L, -1));
			lua_pop(L, 1); // Pop the error
			free(ccmd);
			exitc = 1;
			goto execute_clean;
		}

		// Free the custom command path because it will not be used again
		free(ccmd);

		// Get the main function
		lua_getglobal(L, "main");

		// Check if the main function is actually a function
		if (!lua_isfunction(L, -1)) {
			fprintf(stderr, ERR_TAG "%s cannot be executed by Midorix!\n",
					cmdname);
			lua_pop(L, 1);
			goto execute_clean;
		}

		// Push argc
		lua_pushinteger(L, arg.we_wordc);

		// Push argv
		// Set a new table for argv
		lua_newtable(L);
		for (int i = 0; i < arg.we_wordc; i++) {
			lua_pushinteger(L, i + 1); // Set the index number where the
									   // argument should take place
			lua_pushstring(L, arg.we_wordv[i]); // Set the argument
			lua_rawset(L, -3);					// Set the argument to the table
		}

		// Execute the function
		if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
			// Debug print
			fprintf(stderr, ERR_TAG "Runtime Error: %s\n", lua_tostring(L, -1));
			lua_pop(L, 1);
			exitc = 1;
		}
	}

	// If still not found, will do a system call
	// as a fallback
	if (!found) {
		execcmd(arg.we_wordv);
	}

	// Cleanup
execute_clean:
	wordfree(&arg);
	if (cmdname)
		free(cmdname);
	return exitc;
}

// Midorix CLI function
int midorix_cli(void) {
	// Exit Code
	int exitc = 0;

	// Autoinit_project
	cJSON *autoinit_project =
		cJSON_GetObjectItemCaseSensitive(config, "autoinit_project");

	if (cJSON_IsBool(autoinit_project) && cJSON_IsTrue(autoinit_project) &&
		chkfexist("mdrxproject.lua")) {
		execute(".proman init");
	}

	// Set the maximum history for Midorix
	linenoiseHistorySetMaxLen(
		cJSON_GetObjectItem(config, "max_history")->valueint);

	// Auto-start command
	cJSON *autostart = cJSON_GetObjectItem(config, "autostart");

	// Check if the autostart object is an array
	if (cJSON_IsArray(autostart)) {
		// Iterate each item
		int size = cJSON_GetArraySize(autostart);

		for (int i = 0; i < size; i++) {
			// Get the value
			cJSON *value = cJSON_GetArrayItem(autostart, i);

			// Safety, check if the value is a string
			if (cJSON_IsString(value)) {
				// Execute the command
				execute(value->valuestring);
			} else {
				// Debug print
				fprintf(stderr,
						ERR_TAG
						"Item in autostart on index %d is not a string!\n",
						i);
			}
		}
	} else {
		// Debug print
		fprintf(stderr, ERR_TAG "Object autostart is not an array.\n");
	}

	// Print a line
	puts("====================");

	// Welcome message
	cJSON *welcome_msgj = cJSON_GetObjectItem(config, "welcome_msg");

	// Check if the welcome message is a string
	if (cJSON_IsString(welcome_msgj)) {
		// Get the string value and print it
		welcome_msg =
			strdup(cJSON_GetObjectItem(config, "welcome_msg")->valuestring);
		printf("%s", welcome_msg);
	}

	// Main loop
	while (1) {
		// User Input
		char *input = linenoise(prompt);

		// If the input is NULL
		if (!input) {
			fprintf(stderr, ERR_TAG "Input error.\n");
			exitc = 1;
			break;
		}

		if (*input) {
			// Add to history
			linenoiseHistoryAdd(input);

			// Take the command
			char *command = strdup(input);

			// Check if taking the command is success
			if (!command) {
				perror("strdup");
				continue;
			}

			// Execute the command
			execute(command);

			// Free the command
			free(command);
		}

		// Free the input
		free(input);
	}

	// Return
	return exitc;
}
