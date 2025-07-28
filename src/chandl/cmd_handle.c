#include "cmd_handle.h"
#include "chandl.h"
#include "../util/util.h"
#include "../ext/cJSON/cJSON.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static cJSON* lconfig = NULL;

void cmdh_init_config(cJSON** cfgout, const char* configfn) {
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
	printf("[Midorix] Loaded configuration.\n");
}

void cmdh_run(int argc, char** argv, const char* key) {
	cJSON* kval = cJSON_GetObjectItem(lconfig, key);
	if (!kval || !kval->valuestring) {
		fprintf(stderr, "%s not configured.\n", key);
		return;
	}
	char* val = kval->valuestring;

	char* command = strdup(val); // allocate copy
	if (!command) {
		perror("strdup");
		return;
	}

	int len = snprintf(NULL, 0, "%s_arg", key);
	char* argk = malloc(len + 1);
	if (!argk) {
		perror("malloc");
		return;
	}
	snprintf(argk, len + 1, "%s_arg", key);

	cJSON* aval = cJSON_GetObjectItem(lconfig, argk);
	char* argstr = NULL;
	if (aval) {
		argstr = strdup(aval->valuestring);
		if (!argstr) {
			perror("strdup");
			goto cleanup;
			return;
		}
	}

	wordexp_t w = {0};
	if (argstr) {
		if (ssplit(argstr, &w) != 0) {
			perror("wordexp");
			goto cleanup;
			return;
		}
	}

	// Build argument list
	int total = 1 + w.we_wordc + (argc - 1);
	char** fcommand = malloc(sizeof(char*) * (total + 1));
	if (!fcommand) {
		perror("malloc");
		goto cleanup;
		return;
	}

	int i = 0;
	fcommand[i++] = command;
	for (int j = 0; j < w.we_wordc; j++) fcommand[i++] = w.we_wordv[j];
	for (int j = 1; j < argc; j++) fcommand[i++] = argv[j];
	fcommand[i] = NULL;

	execcmd(fcommand); // will use fcommand[0] = command
	cleanup:
		if (fcommand) free(fcommand);
		free(command);
		free(argk);
		if (argstr) free(argstr);
		wordfree(&w);
}


void cmd_helloWorld(int argc, char** argv) {
	printf("Hello, World!\n");
}


void cmd_help(int argc, char** argv) {
	for (int i = 0; command_table[i].cmd != NULL; i++) {
		printf("Name: %s\n", command_table[i].cmd);
		printf("Shortcut: %s\n", command_table[i].shortcut);
		printf("Description: %s\n\n", command_table[i].desc);
	}
}

void cmd_scfg(int argc, char** argv) {
	printf("%s\n", cJSON_Print(lconfig));
}

void cmd_quit(int argc, char** argv) {
	printf("Goodbye.\n");

	if (argv[1] == NULL) {
		exit(0);
	}

	char *end;
	long val = strtol(argv[1], &end, 10);

	exit(val);
}

void cmd_edit(int argc, char** argv) {
	cmdh_run(argc, argv, "editor");
}

void cmd_exec(int argc, char** argv) {
	cmdh_run(argc, argv, "executor");
}

void cmd_debug(int argc, char** argv) {
	cmdh_run(argc, argv, "debugger");
}

void cmd_mema(int argc, char** argv) {
	cmdh_run(argc, argv, "memanalyzer");
}

