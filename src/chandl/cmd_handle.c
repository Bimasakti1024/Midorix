// src/chandl/cmd_handle.c
#include "cmd_handle.h"
#include "../util/util.h"
#include "chandl.h"
#include <cjson/cJSON.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

static cJSON	 *lconfig = NULL;
static cJSON	 *PCFG	  = NULL;
static lua_State *LPCFG	  = NULL;

void cmdh_cleanup() {
	if (PCFG) {
		cJSON_Delete(PCFG);
		PCFG = NULL;
	}
	if (LPCFG) {
		lua_close(LPCFG);
		LPCFG = NULL;
	}
}

void cmdh_init_config(cJSON **cfgout, const char *configfn) {
	if (chkfexist(configfn) != 0) {
		fprintf(stderr, "Configuration file %s does not exist.\n", configfn);
		exit(1);
	}

	printf("[Midorix] Loading configuration file: %s\n", configfn);
	printf("[Midorix] Parsing configuration.\n");

	if (readjson(configfn, &lconfig) == 1) {
		exit(1);
	}

	*cfgout = lconfig;

	// Close old Lua state if exists
	if (LPCFG) {
		lua_close(LPCFG);
		LPCFG = NULL;
	}

	LPCFG = luaL_newstate();
	if (!LPCFG) {
		fprintf(stderr, "Failed to initialize Lua state.\n");
		exit(1);
	}
	luaL_openlibs(LPCFG);

	atexit(cmdh_cleanup);
	printf("[Midorix] Loaded configuration.\n");
}

void cmdh_run(int argc, char **argv, const char *key) {
	cJSON *kval = cJSON_GetObjectItem(lconfig, key);
	if (!kval || !kval->valuestring) {
		fprintf(stderr, "%s not configured.\n", key);
		return;
	}

	char *command = strdup(kval->valuestring);
	if (!command) {
		perror("strdup");
		return;
	}

	int	  len  = snprintf(NULL, 0, "%s_arg", key);
	char *argk = malloc(len + 1);
	if (!argk) {
		perror("malloc");
		free(command);
		return;
	}
	snprintf(argk, len + 1, "%s_arg", key);

	cJSON *aval	  = cJSON_GetObjectItem(lconfig, argk);
	char  *argstr = NULL;
	if (aval) {
		argstr = strdup(aval->valuestring);
		if (!argstr) {
			perror("strdup");
			free(command);
			free(argk);
			return;
		}
	}

	wordexp_t w = {0};
	if (argstr) {
		if (ssplit(argstr, &w) != 0) {
			perror("wordexp");
			free(command);
			free(argk);
			free(argstr);
			return;
		}
	}

	// Build argument list
	int	   total	= 1 + w.we_wordc + (argc - 1);
	char **fcommand = malloc(sizeof(char *) * (total + 1));
	if (!fcommand) {
		perror("malloc");
		free(command);
		free(argk);
		if (argstr)
			free(argstr);
		wordfree(&w);
		return;
	}

	int i		  = 0;
	fcommand[i++] = command;
	for (int j = 0; j < w.we_wordc; j++)
		fcommand[i++] = w.we_wordv[j];
	for (int j = 1; j < argc; j++)
		fcommand[i++] = argv[j];
	fcommand[i] = NULL;

	execcmd(fcommand);

	// Cleanup
	free(fcommand);
	free(command);
	free(argk);
	if (argstr)
		free(argstr);
	wordfree(&w);
}

cJSON *luaTable2cJSON(lua_State *L, int index) {
	if (index < 0)
		index = lua_gettop(L) + index + 1;

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

	return result;
}

// Command implementations
void cmd_helloWorld(int argc, char **argv) {
	printf("Hello, World!\n");
}

void cmd_help(int argc, char **argv) {
	for (int i = 0; command_table[i].cmd != NULL; i++) {
		printf("Name: %s\n", command_table[i].cmd);
		printf("Shortcut: %s\n", command_table[i].shortcut);
		printf("Description: %s\n\n", command_table[i].desc);
	}
}

void cmd_scfg(int argc, char **argv) {
	char *config = cJSON_Print(lconfig);
	puts(config);
	free(config);
}

void cmd_quit(int argc, char **argv) {
	printf("Goodbye.\n");
	if (argv[1] == NULL) {
		exit(0);
	}
	long val = strtol(argv[1], NULL, 10);
	exit(val);
}

void cmd_edit(int argc, char **argv) {
	cmdh_run(argc, argv, "editor");
}
void cmd_exec(int argc, char **argv) {
	cmdh_run(argc, argv, "executor");
}
void cmd_debug(int argc, char **argv) {
	cmdh_run(argc, argv, "debugger");
}
void cmd_mema(int argc, char **argv) {
	cmdh_run(argc, argv, "memanalyzer");
}

void cmd_init_project(int argc, char **argv) {
	if (chkfexist("mdrxproject.lua")) {
		fprintf(stderr, "Error: mdrxproject.lua does not exist.\n");
		return;
	}

	printf("Initiating project.\n");

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

	if (luaL_dofile(LPCFG, "mdrxproject.lua") != LUA_OK) {
		fprintf(stderr, "%s\n", lua_tostring(LPCFG, -1));
		lua_pop(LPCFG, 1);
		lua_close(LPCFG);
		LPCFG = NULL;
		return;
	}

	lua_getglobal(LPCFG, "project");

	if (PCFG) {
		cJSON_Delete(PCFG);
		PCFG = NULL;
	}

	PCFG = luaTable2cJSON(LPCFG, -1);
	if (!PCFG) {
		fprintf(stderr, "Failed to convert Lua table to JSON.\n");
		lua_close(LPCFG);
		LPCFG = NULL;
		return;
	}

	char *r = cJSON_Print(PCFG);
	if (r) {
		puts(r);
		free(r);
	}

	printf("Project successfully initiated.\n");
}

void cmd_build_project(int argc, char **argv) {
	cJSON *bcfg = cJSON_GetObjectItemCaseSensitive(PCFG, "build_config");
	if (!bcfg)
		return;

	cJSON *langcfg = cJSON_GetObjectItemCaseSensitive(bcfg, "languages");
	cJSON *target  = cJSON_GetObjectItemCaseSensitive(bcfg, "target");
	if (!target)
		return;

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

		if (!source || !lang || !args)
			continue;

		int size = strlen(executor) + strlen(argsv) + strlen(flagsv) +
				   strlen(source) + 4;
		char *fcmd = malloc(size);
		if (!fcmd) {
			perror("malloc");
			continue;
		}

		snprintf(fcmd, size, "%s %s %s %s", executor, argsv, flagsv, source);

		wordexp_t fcmd_s;
		ssplit(fcmd, &fcmd_s);
		free(fcmd);
		if (!fcmd_s.we_wordv) {
			perror("wordexp");
			wordfree(&fcmd_s);
		}

		execcmd(fcmd_s.we_wordv);
		wordfree(&fcmd_s);
	}
}
