#ifndef CMD_HANDLER_H
#define CMD_HANDLER_H

#include "../util/util.h"

void cmdh_init_config(Dictionary* cfgout, const char* configfn);

void cmd_helloWorld(int argc, char** argv);

void cmd_help(int argc, char** argv);
void cmd_scfg(int argc, char** argv);
void cmd_quit(int argc, char** argv);

void cmd_edit(int argc, char** argv);
void cmd_exec(int argc, char** argv);
void cmd_debug(int argc, char** argv);
void cmd_mema(int argc, char** argv);

#endif

