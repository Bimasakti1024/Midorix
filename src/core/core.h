// src/core/core.h
#ifndef CORE_H
#define CORE_H

#include <cjson/cJSON.h>
#include <lua.h>

// Global
extern char *prefix;
extern char *prompt;
extern char *welcome_msg;
extern char *command;
extern size_t prefixlen;
extern cJSON *config;
extern char *config_path;
extern char *configfn;
extern char *shortcutfn;
extern char *ccmd_path;
extern lua_State *L; // For user-defined commands made using LuA

void midorix_cleanup();
int midorix_init(int iie, const char *cfgpth);

#endif

