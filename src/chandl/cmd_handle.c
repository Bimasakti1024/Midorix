#include "cmd_handle.h"
#include "chandl.h"
#include "../util/util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

Dictionary config;

void cmdh_init_config(Dictionary* cfgin) {
	memcpy(&cfgin, &config, sizeof(config));
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

void cmd_exit(int argc, char** argv) {
	printf("Goodbye.\n");

	if (argv[1] == NULL) {
		exit(0);
	}

	char *end;
	long val = strtol(argv[1], &end, 10);

	exit(val);
}

