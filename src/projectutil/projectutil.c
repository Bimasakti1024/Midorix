#include "projectutil.h"
#include "../util/util.h"

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

	if (!lua_istable(L, index)) {
		fprintf(stderr, "Cannot convert Lua table to JSON: not a table.\n");
		return NULL;
	}

	cJSON *result = cJSON_CreateObject();

	if (!result)
		return NULL;

	lua_pushnil(L); // first key
	while (lua_next(L, index) != 0) {
		const char *key = NULL;

		// Convert key to string
		if (lua_type(L, -2) == LUA_TSTRING) {
			key = lua_tostring(L, -2);
		} else if (lua_type(L, -2) == LUA_TNUMBER) {
			static char buf[32];

			snprintf(buf, sizeof(buf), "%lld", (long long)lua_tointeger(L, -2));
			key = buf;
		} else {
			lua_pop(L, 1); // skip value
			continue;
		}

		// Handle value types
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

				if (sub)
					cJSON_AddItemToObject(result, key, sub);
				break;
			}
			default:
				// Skip unsupported types
				break;
		}

		lua_pop(L, 1); // pop value, keep key for next iteration
	}

	return result;
}

void projectutil_init(cJSON **PCFGo) {
	if (!chkfexist("mdrxproject.lua")) {
		fprintf(stderr, "Error: mdrxproject.lua does not exist.\nCannot "
						"initiate project.\n");
		return;
	}

	if (*PCFGo) {
		fprintf(stderr,
				"Project already initiated, please deinitiate it first.\n");
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

void projectutil_build(const cJSON *PCFG, const char *btarget,
					   const char *mode) {
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
	//  Get language configuration
	cJSON *langcfg = cJSON_GetObjectItemCaseSensitive(bcfg, "languages");

	//  Get build targets
	cJSON *target = cJSON_GetObjectItemCaseSensitive(bcfg, "target");

	if (!target) {
		fprintf(stderr, "No build target index found.\n");
		return;
	}

	//  Get mode
	cJSON *modeobj = cJSON_GetObjectItemCaseSensitive(PCFG, "mode");

	if (!modeobj) {
		fprintf(stderr, "No mode configuration found.\n");
		return;
	}
	cJSON *smode = cJSON_GetObjectItemCaseSensitive(modeobj, mode);

	if (!smode) {
		fprintf(stderr, "Selected mode %s does not exist.\n", mode);
		return;
	}

	// Set up the target configuration
	cJSON *targetv = NULL;

	if (btarget) {
		targetv = cJSON_GetObjectItemCaseSensitive(target, btarget);
		if (!targetv) {
			fprintf(stderr, "Build target %s does not exist.\n", btarget);
			return;
		}
	} else {
		int isize = cJSON_GetArraySize(target);

		targetv = cJSON_CreateArray();

		// Iterate each target
		for (int i = 0; i < isize; i++) {
			cJSON *iitem = cJSON_GetArrayItem(target, i);
			int	   jsize = cJSON_GetArraySize(iitem);

			// Add all target targets to the targetv array
			for (int j = 0; j < jsize; j++) {
				cJSON *jitem  = cJSON_GetArrayItem(iitem, j);
				cJSON *jitemc = cJSON_Duplicate(jitem, 1);

				cJSON_AddItemToArray(targetv, jitemc);
			}
		}
	}

	// Iterate the targets and build it
	int tsize = cJSON_GetArraySize(targetv);

	for (int i = 0; i < tsize; i++) {
		cJSON *item = cJSON_GetArrayItem(targetv, i);

		if (!item)
			continue;

		// Get source
		char *source =
			cJSON_GetObjectItemCaseSensitive(item, "source")->valuestring;

		if (!source) {
			fprintf(stderr,
					"Build target at index %d does not have a source file.\n",
					i);
			break;
		}

		// Get language
		char *lang =
			cJSON_GetObjectItemCaseSensitive(item, "language")->valuestring;

		if (!lang) {
			fprintf(stderr,
					"Build target %s does not have language configured.\n",
					source);
		}

		// Get action
		cJSON *taction = cJSON_GetObjectItemCaseSensitive(item, "action");

		if (!taction) {
			fprintf(stderr,
					"Build target %s does not have any action configured.\n",
					source);
			break;
		}
		char *tactionv = taction->valuestring;

		// Set arguments
		cJSON *args	 = cJSON_GetObjectItemCaseSensitive(item, "args");
		char  *argsv = args->valuestring ? args->valuestring : "";

		// Set mode flags
		cJSON *slmode = cJSON_GetObjectItemCaseSensitive(smode, lang);

		if (!slmode) {
			fprintf(stderr, "Language %s is not available for %s build mode.\n",
					lang, mode);
			break;
		}

		cJSON *mflags  = cJSON_GetObjectItemCaseSensitive(slmode, "flags");
		char  *mflagsv = mflags->valuestring ? mflags->valuestring : "";

		cJSON *slang  = cJSON_GetObjectItemCaseSensitive(langcfg, lang);
		cJSON *action = cJSON_GetObjectItemCaseSensitive(slang, "action");
		cJSON *sact	  = cJSON_GetObjectItemCaseSensitive(action, tactionv);

		if (!sact) {
			free(argsv);
			fprintf(stderr, "Build target %s action %s does not exist.\n",
					source, tactionv);
			break;
		}

		char *executor =
			cJSON_GetObjectItemCaseSensitive(slang, "executor")->valuestring;
		cJSON *flags  = cJSON_GetObjectItemCaseSensitive(sact, "flags");
		char  *flagsv = flags->valuestring ? flags->valuestring : "";

		// Declare fcmd variable for the command to execute later
		int cmd_size = strlen(executor) + strlen(argsv) + strlen(mflagsv) +
					   strlen(flagsv) + strlen(source) + 5;
		char *fcmd = malloc(cmd_size);

		if (!fcmd) {
			perror("malloc");
			continue;
		}

		// Construct the fcmd
		snprintf(fcmd, cmd_size, "%s %s %s %s %s", executor, argsv, mflagsv,
				 flagsv, source);

		// fcmd_s for the wordexp_t form of fcmd
		wordexp_t fcmd_s;

		ssplit(fcmd, &fcmd_s);
		free(fcmd);
		if (!fcmd_s.we_wordv) {
			perror("wordexp");
			wordfree(&fcmd_s);
		}

		printf("Building %s\n", source);

		// Execute
		execcmd(fcmd_s.we_wordv);
		wordfree(&fcmd_s);
	}
	if (!btarget)
		cJSON_Delete(targetv);
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
