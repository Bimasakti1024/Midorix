#include "cmd_handle.h"
#include "chandl.h"
#include "../util/util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static Dictionary config;

void cmdh_init_config(Dictionary* cfgout, const char* configfn) {
	if (chkfexist(configfn) != 0) {
		fprintf(stderr, "Configuration does not exist.\n");
		exit(1);
	}

	printf("[Midorix] Loading configuration file: %s\n", configfn);
	printf("[Midorix] Parsing configuration.\n");

	if (readini(configfn, &config) == 1) {
		exit(1);
	}

	for (int i = 0; i < config.count; i++) {
		char* value = config.value[i];
		sfeval(value, value, MAX_VALUE);
	}

	memcpy(cfgout, &config, sizeof(Dictionary));
	printf("[Midorix] Loaded configuration.\n");
}

void cmdh_run(int argc, char** argv, const char* key) {
	const char* command = dict_get(&config, key);
	if (!command) {
		fprintf(stderr, "%s not configured.\n", key);
		return;
	}

	char argk[MAX_KEY];
	snprintf(argk, sizeof(argk), "%s_arg", key);

	const char* argstr = dict_get(&config, argk);

	wordexp_t w = {0};
	if (argstr) ssplit(argstr, &w);

	// Count arguments:
	// command + arg_from_config + arg_from_user + NULL
	int total = 1 + w.we_wordc + (argc - 1);
	char** fcommand = malloc(sizeof(char*) * (total + 1));
	if (!fcommand) {
		perror("malloc");
		wordfree(&w);
		return;
	}

	int i = 0;
	fcommand[i++] = (char*)command;

	for (int j = 0; j < w.we_wordc; j++) fcommand[i++] = w.we_wordv[j];

	for (int j = 1; j < argc; j++) fcommand[i++] = argv[j];

	fcommand[i] = NULL; // NULL-terminate

	execcmd(fcommand);
	free(fcommand);
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
	for (int i = 0; i < config.count; i++) {
		printf("%s: %s\n", config.key[i], config.value[i]);
	}
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

