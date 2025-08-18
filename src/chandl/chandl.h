// src/chandl/chandl.h
#ifndef CHANDL_H
#define CHANDL_H

typedef struct {
	const char *cmd;
	const char *shortcut;
	void (*handler)(void);
	const char *desc;
} noargCommand;

typedef struct {
	const char *cmd;
	const char *shortcut;
	void (*handler)(int, char **);
	const char *desc;
} Command;

extern Command command_table[];

#endif
