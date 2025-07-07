#include "cmd_handle.h"
#include "chandl.h"
#include "../util/util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static Dictionary config;

void cmdh_init_config(Dictionary* cfgin) {
	memcpy(&config, cfgin, sizeof(Dictionary));
}

void cmdh_run(int argc, char** argv, const char* key) {
    if (argc < 2) {
        printf("Expected at least 2 arguments.\n");
        return;
    }

    const char* command = dict_get(&config, key);
    printf("%s\n", command);
    if (!command) {
        fprintf(stderr, "%s not configured.\n", key);
        return;
    }

    char** fcommand = malloc(sizeof(char*) * (argc + 1));
    if (!command) {
        perror("malloc");
        return;
    }

    fcommand[0] = (char*) command;
    memmove(&fcommand[1], &argv[1], sizeof(char*) * (argc - 1));
    fcommand[argc] = NULL;

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
