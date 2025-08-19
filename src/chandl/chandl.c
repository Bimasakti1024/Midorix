// src/chandl/chandl.c
#include "chandl.h"
#define CMD(name, alias, func, desc) void func(int, char **);
#include "commands.def"
#undef CMD

#include <stddef.h>

#define CMD(name, alias, func, desc) {name, alias, func, desc},
Command command_table[] = {
#include "commands.def"
	{NULL, NULL, NULL, NULL}
};

#undef CMD
