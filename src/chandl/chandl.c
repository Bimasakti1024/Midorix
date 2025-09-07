// src/chandl/chandl.c
#include "chandl.h"
#define MDRX_REGISTER_CMD(name, alias, func, desc) void func(int, char **);
#include "commands.def"
#undef MDRX_REGISTER_CMD

#include <stddef.h>

#define MDRX_REGISTER_CMD(name, alias, func, desc) {name, alias, func, desc},
Command command_table[] = {
#include "commands.def"
	{NULL, NULL, NULL, NULL}};

#undef CMD
