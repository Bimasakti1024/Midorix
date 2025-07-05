#ifndef CHANDL_H
#define CHANDL_H

void cmd_exit(int argc, char **argv);

typedef struct {
	const char* cmd;
	const char* shortcut;
	void (*handler)(int, char **);
	const char* desc;
} Command;

extern Command command_table[];

#endif

