// src/chandl/cmd_handle.c
#include "cmd_handle.h"
#include "../core/core.h"
#include "../projectutil/projectutil.h"
#include "../util/util.h"
#include "chandl.h"

#include <cjson/cJSON.h>
#include <dirent.h>
#include <lauxlib.h>
#include <linux/limits.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

static cJSON *lconfig = NULL;
static cJSON *PCFG	  = NULL;

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
	if (!chkfexist(configfn)) {
		fprintf(stderr, "Configuration file %s does not exist.\n", configfn);
		exit(1);
	}

	printf("[Midorix] Loading configuration file: %s\n", configfn);
	printf("[Midorix] Parsing configuration.\n");

	if (readjson(configfn, &lconfig)) {
		fprintf(stderr, "Failed to parse configuration.\n");
		exit(1);
	}

	*cfgout = lconfig;

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

	char *argk = append(key, "_arg");
	if (!argk) {
		free(command);
		return;
	}

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

// Command implementations
void cmd_helloWorld(int argc, char **argv) {
	printf("Hello, World!\n");
}

void cmd_help(int argc, char **argv) {
	printf("List of built-in commands:\n"
		   "  NAME          ALIAS     DESCRIPTION\n");
	for (int i = 0; command_table[i].cmd != NULL; i++) {
		printf("  %-10s    %-6s    %s\n", command_table[i].cmd,
			   command_table[i].alias, command_table[i].desc);
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

// Project manager

// subcommands index forward declaration
extern Command proman_subcommands[];

// Subfunction
static void psub_init(int argc, char **argv) {
	projectutil_init(&PCFG);
}
static void psub_deinit(int argc, char **argv) {
	// Safety
	if (!PCFG) {
		fprintf(stderr, "No project is currently initialized.\n");
		return;
	}
	// Delete
	cJSON_Delete(PCFG);
	PCFG = NULL;
	printf("Project deinitialized.\n");
}
static void psub_build(int argc, char **argv) {
	if (argc < 2) {
		printf("Usage: .proman build MODE TARGET\n");
		return;
	}

	projectutil_build(PCFG, argv[2], argv[1]);
}
static void psub_show(int argc, char **argv) {
	char *cconfig = cJSON_Print(PCFG);
	if (cconfig) {
		puts(cconfig);
		free(cconfig);
	} else {
		fprintf(stderr, "Configuration is empty.\n");
	}
}

static void psub_help(int argc, char **arg) {
	printf("Available project subcommands:\n"
		   "  NAME          ALIAS          DESCRIPTION\n");
	for (int i = 0; proman_subcommands[i].cmd != NULL; i++) {
		printf("  %-10s    %-10s    %s\n", proman_subcommands[i].cmd,
			   proman_subcommands[i].alias, proman_subcommands[i].desc);
	}
}

static void psub_custom_rule(int argc, char **argv) {
	if (argc < 1) {
		printf("No rule were provided.\n");
		return;
	}

	projectutil_custom_rule(argv[0], argc, argv);
}

// subcommands index
Command proman_subcommands[] = {
	{"init", "i", psub_init, "Initialize project."},
	{"deinit", "di", psub_deinit, "Deinitialize project."},
	{"build", "b", psub_build, "Build initiated project."},
	{"custom", "c", psub_custom_rule, "Execute a custom rule."},
	{"show", "s", psub_show, "Show configuration."},
	{"help", "h", psub_help, "Show help."},
	{NULL, NULL, NULL, NULL}};

//  Main function
void cmd_project(int argc, char **argv) {
	if (argc < 2) {
		fprintf(
			stderr,
			"No subcommand were provided. Use \"help\" subcommands for list of "
			"subcommands.\n");
		return;
	}

	char **rargv = &argv[1];

	for (int i = 0; proman_subcommands[i].cmd != NULL; i++) {
		if ((strcmp(proman_subcommands[i].cmd, argv[1]) == 0) ||
			(strcmp(proman_subcommands[i].alias, argv[1]) == 0)) {
			proman_subcommands[i].handler(argc - 1, rargv);
			return;
		}
	}

	fprintf(stderr, "Subcommand not recognized.\n");
}

// Doctor
void cmd_doctor(int argc, char **argv) {
	int problemc = 0;
	int warningc = 0;

	printf("Checking Midorix..\n");

	// Check configuration
	printf("Checking configuration %s: ", configfn);
	cJSON *dummy;
	if (readjson(configfn, &dummy)) {
		printf("PROBLEM.\n");
		problemc++;
	} else {
		printf("OK.\n");
	}
	cJSON_Delete(dummy);

	// Check custom command directory
	printf("Checking %s existence: ", ccmd_path);
	if (dir_exist(ccmd_path)) {
		printf("OK.\n");

		DIR *ccmdr = opendir(ccmd_path);
		if (ccmdr) {
			struct dirent *entry;

			while ((entry = readdir(ccmdr)) != NULL) {
				if (entry->d_name[0] == '.' &&
					(entry->d_name[1] == '\0' ||
					 (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
					continue;

				lua_State *eL = luaL_newstate();
				luaL_openlibs(eL);

				char *fccmd_path = append(ccmd_path, "/");
				char *fullpath	 = append(fccmd_path, entry->d_name);
				free(fccmd_path);
				printf("Checking %s: ", fullpath);

				if (luaL_dofile(eL, fullpath)) {
					printf("PROBLEM\n"
						   "=======================================\n");
					fprintf(stderr, "%s\n", lua_tostring(eL, -1));
					printf("=======================================\n");
					problemc++;
					lua_pop(eL, 1);
				} else {
					printf("OK\n");
				}
				free(fullpath);
				lua_close(eL);
			}
			closedir(ccmdr);
		} else {
			perror("opendir");
			problemc++;
		}
	} else {
		printf("WARNING.\n");
		warningc++;
	}

	printf("Problem: %d.\n", problemc);
	printf("Warning: %d\n", warningc);
}
