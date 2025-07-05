#include "chandl.h"
#define CMD(name, shortcut, func, desc) void func(int, char **);
#include "commands.def"
#undef CMD

#include <stddef.h>

#define CMD(name, shortcut, func, desc) {name, shortcut, func, desc},
Command command_table[] = {
	#include "commands.def"
	{NULL, NULL, NULL}
};
#undef CMD

