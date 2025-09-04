// src/main.c
#include "core/core.h"
#include "engine/engine.h"

#include <stdio.h>
#include <stdlib.h>

int main() {
	atexit(midorix_cleanup);
	if (midorix_init() != 0) {
		fprintf(stderr, "Failed to init Midorix!\n");
		return 1;
	}

	midorix_cli();
	return 0;
}
