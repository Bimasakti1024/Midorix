#ifndef PROJECTUTIL_H
#define PROJECTUTIL_H

#include <cjson/cJSON.h>
#include <lua.h>
#include <wordexp.h>

extern lua_State *LPCFG;

void projectutil_init(cJSON **PCFGo);
void projectutil_build(const cJSON *PCFG, const char *mode);
void projectutil_custom_rule(const char *rname, int rargc, char **rargv);

#endif
