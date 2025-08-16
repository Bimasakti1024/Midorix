// src/chandl/cmd_handle.h
#ifndef CMD_HANDLE_H
#define CMD_HANDLE_H

#include "../util/util.h"
#include <cjson/cJSON.h>

void cmdh_init_config(cJSON **cfgout, const char *configfn);

void cmd_helloWorld(int argc, char **argv);

void cmd_help(int argc, char **argv);
void cmd_scfg(int argc, char **argv);
void cmd_quit(int argc, char **argv);

void cmd_edit(int argc, char **argv);
void cmd_exec(int argc, char **argv);
void cmd_debug(int argc, char **argv);
void cmd_mema(int argc, char **argv);

void cmd_init_project(int argc, char **argv);
void cmd_build_project(int argc, char **argv);

#endif
