// src/chandl/cmd_handle.c
#include "cmd_handle.h"
#include "../projectutil/projectutil.h"
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

static cJSON *lconfig = NULL;
static cJSON *PCFG	  = NULL;

void cmdh_cleanup() {
	if (PCFG) {
		cJSON_Delete(PCFG);
		PCFG = NULL;
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

// Command implementations
void cmd_helloWorld(int argc, char **argv) {
	printf("Hello, World!\n");
}

void cmd_help(int argc, char **argv) {
	printf("List of built-in commands:\n"
		"  NAME          ALIAS     DESCRIPTION\n");
	for (int i = 0; command_table[i].cmd != NULL; i++) {
		printf("  %-10s    %-6s    %s\n",
		 command_table[i].cmd,
		 command_table[i].alias,
		 command_table[i].desc);
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

// Subfunction
static void psub_init(void)  { projectutil_init(&PCFG); }
static void psub_deinit(void) {
	// Safety
	if (!PCFG) {
		printf("No project is currently initialized.\n");
		return;
	}
	cJSON_Delete(PCFG);
	PCFG = NULL;
	printf("Project deinitialized.\n");
}
static void psub_build(void) { projectutil_build(PCFG); }
static void psub_show(void) {
	char *cconfig = cJSON_Print(PCFG);
	if (cconfig) {
		puts(cconfig);
		free(cconfig);
	} else {
		fprintf(stderr, "Configuration is empty.\n");
	}
}

extern noargCommand subcommands[];

static void psub_help(void) {
    printf("Available project subcommands:\n"
		   "  NAME          ALIAS     DESCRIPTION\n");
    for (int i = 0; subcommands[i].cmd != NULL; i++) {
        printf("  %-10s    %-6s    %s\n",
            subcommands[i].cmd,
            subcommands[i].alias,
            subcommands[i].desc);
    }
}

// subcommands index
noargCommand subcommands[] = {
    {"init", "i", psub_init, "Initialize project."},
    {"deinit", "di", psub_deinit, "Deinitialize project."},
    {"build", "b", psub_build, "Build initiated project."},
    {"show", "s", psub_show, "Show configuration."},
    {"help", "h", psub_help, "Show help."},
    {NULL, NULL, NULL, NULL}
};

//  Main function
void cmd_project(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "No action were provided. Use help action for list of subcommands.\n");
        return;
    }

    for (int i = 0; subcommands[i].cmd != NULL; i++) {
        if ((strcmp(subcommands[i].cmd, argv[1]) == 0) || (strcmp(subcommands[i].alias, argv[1]) == 0)) {
            subcommands[i].handler();
            return;
        }
    }

    fprintf(stderr, "Action not recognized.\n");
}

