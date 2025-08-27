#include "projectutil.h"
#include "../util/util.h"

#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

#include <cjson/cJSON.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

lua_State *LPCFG = NULL;

cJSON *luaTable2cJSON(lua_State *L, int index) {
	if (index < 0)
		index = lua_gettop(L) + index + 1;

	// If it is not a table
	if (!lua_istable(L, index)) {
		fprintf(
			stderr,
			"Cannot convert Lua table to JSON because it is not a table.\n");
		return NULL;
	}

	cJSON *result = cJSON_CreateObject();
	if (!result)
		return NULL;

	lua_pushnil(L);
	while (lua_next(L, index) != 0) {
		const char *key = NULL;

		// Set the key and value
		if (lua_type(L, -2) == LUA_TSTRING) {
			key = lua_tostring(L, -2);
		} else if (lua_type(L, -2) == LUA_TNUMBER) {
			static char buf[32];
			snprintf(buf, sizeof(buf), "%lld", (long long)lua_tointeger(L, -2));
			key = buf;
		} else {
			lua_pop(L, 1);
			continue;
		}

		// Add the key and value to the result
		switch (lua_type(L, -1)) {
			case LUA_TSTRING:
				cJSON_AddStringToObject(result, key, lua_tostring(L, -1));
				break;
			case LUA_TNUMBER:
				cJSON_AddNumberToObject(result, key, lua_tonumber(L, -1));
				break;
			case LUA_TBOOLEAN:
				cJSON_AddBoolToObject(result, key, lua_toboolean(L, -1));
				break;
			case LUA_TTABLE: {
				cJSON *sub = luaTable2cJSON(L, lua_gettop(L));
				if (sub) {
					cJSON_AddItemToObject(result, key, sub);
				}
				break;
			}
			default:
				break;
		}
		lua_pop(L, 1);
	}

	// Return the result
	return result;
}

void projectutil_init(cJSON **PCFGo) {
	if (chkfexist("mdrxproject.lua")) {
		fprintf(stderr, "Error: mdrxproject.lua does not exist.\nCannot "
						"initiate project.\n");
		return;
	}

	printf("Initiating project.\n");

	// Initiate PCFG
	cJSON *PCFG = cJSON_CreateObject();

	// Close old Lua state
	if (LPCFG) {
		lua_close(LPCFG);
		LPCFG = NULL;
	}
	LPCFG = luaL_newstate();
	if (!LPCFG) {
		fprintf(stderr, "Failed to initialize Lua.\n");
		return;
	}
	luaL_openlibs(LPCFG);

	// Check mdrxproject.lua file existence
	if (luaL_dofile(LPCFG, "mdrxproject.lua") != LUA_OK) {
		fprintf(stderr, "%s\n", lua_tostring(LPCFG, -1));
		lua_pop(LPCFG, 1);
		lua_close(LPCFG);
		LPCFG = NULL;
		return;
	}

	// Get the project variable
	lua_getglobal(LPCFG, "project");

	if (PCFG) {
		cJSON_Delete(PCFG);
		PCFG = NULL;
	}

	// Convert Lua to JSON
	PCFG = luaTable2cJSON(LPCFG, -1);
	if (!PCFG) {
		fprintf(stderr, "Failed to convert Lua table to JSON.\n");
		lua_close(LPCFG);
		LPCFG = NULL;
		return;
	}

	*PCFGo = PCFG;
	printf("Project successfully initiated.\n");
}

void projectutil_build(const cJSON *PCFG) {
	// Check if the project is initialized
	if (!PCFG) {
		fprintf(stderr, "Project is not initialized.\n");
		return;
	}
	// Get the build_config
	cJSON *bcfg = cJSON_GetObjectItemCaseSensitive(PCFG, "build_config");
	if (!bcfg) {
		fprintf(
			stderr,
			"build_config object in project configuration does not exist.\n");
		return;
	}

	// Print project name and version
	char *pname = cJSON_GetObjectItemCaseSensitive(PCFG, "name")->valuestring;
	char *pversion =
		cJSON_GetObjectItemCaseSensitive(PCFG, "version")->valuestring;
	printf("Building %s %s.\n", pname, pversion);

	// Get some configuration from build_config
	cJSON *langcfg = cJSON_GetObjectItemCaseSensitive(bcfg, "languages");
	cJSON *target  = cJSON_GetObjectItemCaseSensitive(bcfg, "target");
	if (!target) {
		printf("No build target found.\n");
		return;
	}

	// Iterate the targets
	int size = cJSON_GetArraySize(target);
	for (int i = 0; i < size; i++) {
		cJSON *item = cJSON_GetArrayItem(target, i);
		if (!item)
			continue;

		char  *source = cJSON_GetObjectItem(item, "source")->valuestring;
		char  *lang	  = cJSON_GetObjectItem(item, "language")->valuestring;
		cJSON *args	  = cJSON_GetObjectItem(item, "args");
		char  *argsv  = args->valuestring ? args->valuestring : "";

		cJSON *slang	= cJSON_GetObjectItem(langcfg, lang);
		char  *executor = cJSON_GetObjectItem(slang, "executor")->valuestring;
		cJSON *flags	= cJSON_GetObjectItem(slang, "flags");
		char  *flagsv	= flags->valuestring ? flags->valuestring : "";

		if (!source || !lang) {
			fprintf(stderr,
					"The target index %d does not have source or a language.\n",
					i);
			continue;
		}

		// Declare fcmd variable for the command to execute later
		int size = strlen(executor) + strlen(argsv) + strlen(flagsv) +
				   strlen(source) + 4;
		char *fcmd = malloc(size);
		if (!fcmd) {
			perror("malloc");
			continue;
		}

		// Construct the fcmd
		snprintf(fcmd, size, "%s %s %s %s", executor, argsv, flagsv, source);

		// fcmd_s for the wordexp_t form of fcmd
		wordexp_t fcmd_s;
		ssplit(fcmd, &fcmd_s);
		free(fcmd);
		if (!fcmd_s.we_wordv) {
			perror("wordexp");
			wordfree(&fcmd_s);
		}

		// Execute
		execcmd(fcmd_s.we_wordv);
		wordfree(&fcmd_s);
	}
	printf("Build completed.\n");
}

void projectutil_custom_rule(const char *rname, int rargc, char **rargv) {
	if (!LPCFG) {
		fprintf(stderr, "Configuration is empty.\n");
		return;
	}

	// get function
	lua_getglobal(LPCFG, rname);
	// check if is a function and exist
	if (!lua_isfunction(LPCFG, -1)) {
		fprintf(stderr, "Custom rule not found or not a function.\n");
		lua_pop(LPCFG, 1);
		return;
	}

	// push argc
	lua_pushinteger(LPCFG, rargc);

	// push argv
	lua_newtable(LPCFG);

	// push argv content
	for (int i = 0; i < rargc; i++) {
		lua_pushinteger(LPCFG, i + 1);
		lua_pushstring(LPCFG, rargv[i]);
		lua_settable(LPCFG, -3);
	}

	// execute
	if (lua_pcall(LPCFG, 2, 0, 0) != LUA_OK) {
		fprintf(stderr, "Error encountered when calling %s: %s\n", rname,
				lua_tostring(LPCFG, -1));
		lua_pop(LPCFG, 1);
		return;
	}
}
