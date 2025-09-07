// src/main.c
#include "core/core.h"
#include "engine/engine.h"
#include "util/util.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	// Debug print
	printf(INFO_TAG "Initiating Midorix.\n");

	// Setup Cleanup
	atexit(midorix_cleanup);

	// Initiate Midorix
	int midorix_status = midorix_init(0);
	if (midorix_status != 0) {
		fprintf(stderr,
				RED "FATAL" RESET ": Failed to init Midorix. Exit Code: %d.\n",
				midorix_status);
		return -1;
	}
	printf(INFO_TAG "Midorix successfully initiated.\n");

	// Start the CLI
	midorix_cli();
	printf(INFO_TAG "End.\n");
	return 0;
}
