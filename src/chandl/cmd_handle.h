#ifndef CMD_HANDLER_H
#define CMD_HANDLER_H

#include "../util/util.h"

void cmdh_init_config(Dictionary* cfgin);

void cmd_helloWorld(int argc, char** argv);

void cmd_help(int argc, char** argv);
void cmd_exit(int argc, char** argv);

#endif

