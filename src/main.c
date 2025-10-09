// src/main.c
#include "core/core.h"
#include "engine/engine.h"
#include "util/util.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	bool  ignore_init_error;
	bool  no_init;
	char *config_path;
	bool  cli;
	char *execute;
} LaunchParams;

extern LaunchParams parse_mdrxparam(int argc, char *argv[]);
extern void			free_cstring(char *s);

LaunchParams LaunchParameter;

void LaunchParam_cleanup() {
	if (LaunchParameter.execute)
		free_cstring(LaunchParameter.execute);
	if (LaunchParameter.config_path)
		free_cstring(LaunchParameter.config_path);
}

void cleanup() {
	LaunchParam_cleanup();
	midorix_cleanup();
}

int main(int argc, char *argv[]) {
	printf(INFO_TAG "Start.\n");

	// Setup Cleanup
	atexit(cleanup);

	LaunchParameter = parse_mdrxparam(argc, argv);

	// Initiate Midorix
	if (!LaunchParameter.no_init) {
		printf(INFO_TAG "Initiating Midorix.\n");
		int midorix_status = midorix_init(LaunchParameter.ignore_init_error,
										  LaunchParameter.config_path);
		if (midorix_status != 0) {
			fprintf(stderr,
					RED "FATAL" RESET
						": Failed to init Midorix.\nExit Code: %d.\n",
					midorix_status);
			return -1;
		}
		printf(SUCC_TAG "Midorix successfully initiated.\n");
	}

	// Execute a command
	if (LaunchParameter.execute) {
		execute(LaunchParameter.execute);
	}

	// Start the CLI
	if (LaunchParameter.cli)
		midorix_cli();
	printf(INFO_TAG "End.\n");
	return 0;
}
