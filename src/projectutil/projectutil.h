#ifndef PROJECTUTIL_H
#define PROJECTUTIL_H

#define _GNU_SOURCE
#include <wordexp.h>
#include <cjson/cJSON.h>
#include <lua.h>

extern lua_State *LPCFG;

void projectutil_init(cJSON **PCFGo);
void projectutil_build(const cJSON *PCFG);
void projectutil_custom_rule(const char* rname, int rargc, char **rargv);

#endif
