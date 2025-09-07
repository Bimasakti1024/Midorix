// src/chandl/cmd_handle.c
#include "cmd_handle.h"
#include "../core/core.h"
#include "../projectutil/projectutil.h"
#include "../util/util.h"
#include "chandl.h"

#include <cjson/cJSON.h>
#include <dirent.h>
#include <lauxlib.h>
#include <linux/limits.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wordexp.h>

static cJSON *lconfig;
static cJSON *PCFG;

void cmdh_cleanup(void) {
	if (PCFG) {
		cJSON_Delete(PCFG);
		PCFG = NULL;
	}
	if (LPCFG) {
		lua_close(LPCFG);
		LPCFG = NULL;
	}
}

void cmdh_init_config(cJSON **cfgout, const char *configfn) {
	if (!chkfexist(configfn)) {
		fprintf(stderr, "Configuration file %s does not exist.\n", configfn);
		exit(1);
	}

	printf(INFO_TAG "Loading configuration file: %s\n", configfn);
	printf(INFO_TAG "Parsing configuration.\n");

	if (readjson(configfn, &lconfig)) {
		fprintf(stderr, ERR_TAG "Failed to parse configuration.\n");
		exit(1);
	}

	*cfgout = lconfig;

	atexit(cmdh_cleanup);
	printf(INFO_TAG "Loaded configuration.\n");
}

void cmdh_run(int argc, char **argv, const char *key) {
	cJSON *kval = cJSON_GetObjectItem(lconfig, key);

	if (!kval || !kval->valuestring) {
		fprintf(stderr, ERR_TAG "%s not configured.\n", key);
		return;
	}

	char *command = strdup(kval->valuestring);

	if (!command) {
		perror("strdup");
		return;
	}

	char *argk = concat(key, "_arg", NULL);

	if (!argk) {
		free(command);
		return;
	}

	cJSON *aval	  = cJSON_GetObjectItem(lconfig, argk);
	char  *argstr = NULL;

	if (aval) {
		argstr = strdup(aval->valuestring);
		if (!argstr) {
			perror("strdup");
			free(command);
			free(argk);
			return;
		}
	}

	wordexp_t w = {0};

	if (argstr) {
		if (ssplit(argstr, &w) != 0) {
			perror("wordexp");
			free(command);
			free(argk);
			free(argstr);
			return;
		}
	}

	// Build argument list
	int	   total	= 1 + w.we_wordc + (argc - 1);
	char **fcommand = malloc(sizeof(char *) * (total + 1));

	if (!fcommand) {
		perror("malloc");
		free(command);
		free(argk);
		if (argstr)
			free(argstr);
		wordfree(&w);
		return;
	}

	int i = 0;

	fcommand[i++] = command;
	for (int j = 0; j < w.we_wordc; j++)
		fcommand[i++] = w.we_wordv[j];
	for (int j = 1; j < argc; j++)
		fcommand[i++] = argv[j];
	fcommand[i] = NULL;

	execcmd(fcommand);

	// Cleanup
	free(fcommand);
	free(command);
	free(argk);
	if (argstr)
		free(argstr);
	wordfree(&w);
}

// Command implementations
void cmd_helloWorld(int argc, char **argv) {
	printf("Hello, World!\n");
}

void cmd_help(int argc, char **argv) {
	printf("List of built-in commands:\n"
		   "  NAME          ALIAS     DESCRIPTION\n");
	for (int i = 0; command_table[i].cmd != NULL; i++) {
		printf("  %-10s    %-6s    %s\n", command_table[i].cmd,
			   command_table[i].alias, command_table[i].desc);
	}
}

void cmd_scfg(int argc, char **argv) {
	char *config = cJSON_Print(lconfig);

	puts(config);
	free(config);
}

void cmd_quit(int argc, char **argv) {
	printf("Goodbye.\n");
	if (argv[1] == NULL) {
		exit(0);
	}
	long val = strtol(argv[1], NULL, 10);

	exit(val);
}

void cmd_edit(int argc, char **argv) {
	cmdh_run(argc, argv, "editor");
}
void cmd_exec(int argc, char **argv) {
	cmdh_run(argc, argv, "executor");
}
void cmd_debug(int argc, char **argv) {
	cmdh_run(argc, argv, "debugger");
}
void cmd_mema(int argc, char **argv) {
	cmdh_run(argc, argv, "memanalyzer");
}

// Project manager

// subcommands index forward declaration
extern Command proman_subcommands[];

// Subfunction
static void psub_init(int argc, char **argv) {
	projectutil_init(&PCFG);
}
static void psub_deinit(int argc, char **argv) {
	// Safety
	if (!PCFG) {
		fprintf(stderr, "No project is currently initialized.\n");
		return;
	}
	// Delete
	cJSON_Delete(PCFG);
	PCFG = NULL;
	printf("Project deinitialized.\n");
}
static void psub_build(int argc, char **argv) {
	if (argc < 2) {
		printf("Usage: .proman build MODE TARGET\n");
		return;
	}

	projectutil_build(PCFG, argv[2], argv[1]);
}
static void psub_show(int argc, char **argv) {
	char *cconfig = cJSON_Print(PCFG);

	if (cconfig) {
		puts(cconfig);
		free(cconfig);
	} else {
		fprintf(stderr, "Configuration is empty.\n");
	}
}

static void psub_help(int argc, char **arg) {
	printf("Available project subcommands:\n"
		   "  NAME          ALIAS          DESCRIPTION\n");
	for (int i = 0; proman_subcommands[i].cmd != NULL; i++) {
		printf("  %-10s    %-10s    %s\n", proman_subcommands[i].cmd,
			   proman_subcommands[i].alias, proman_subcommands[i].desc);
	}
}

static void psub_custom_rule(int argc, char **argv) {
	if (argc < 1) {
		printf("No rule were provided.\n");
		return;
	}

	projectutil_custom_rule(argv[0], argc, argv);
}

// subcommands index
Command proman_subcommands[] = {
	{"init", "i", psub_init, "Initialize project."},
	{"deinit", "di", psub_deinit, "Deinitialize project."},
	{"build", "b", psub_build, "Build initiated project."},
	{"custom", "c", psub_custom_rule, "Execute a custom rule."},
	{"show", "s", psub_show, "Show configuration."},
	{"help", "h", psub_help, "Show help."},
	{NULL, NULL, NULL, NULL}};

//  Main function
void cmd_project(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr,
				"No subcommand were provided. Use \"help\" subcommands for "
				"list of subcommands.\n");
		return;
	}

	char **rargv = &argv[1];

	for (int i = 0; proman_subcommands[i].cmd != NULL; i++) {
		if ((strcmp(proman_subcommands[i].cmd, argv[1]) == 0) ||
			(strcmp(proman_subcommands[i].alias, argv[1]) == 0)) {
			proman_subcommands[i].handler(argc - 1, rargv);
			return;
		}
	}

	fprintf(stderr, "Subcommand not recognized.\n");
}

// Doctor
void cmd_doctor(int argc, char **argv) {
	int testc	 = 0;
	int problemc = 0;
	int warningc = 0;

	printf(INFO_TAG "Checking Midorix..\n");

	// Check configuration
	if (configfn) {
		printf("\n======== Configuration Check ========\n");
		testc++;
		printf("Checking if %s exist: ", configfn);
		if (chkfexist(configfn)) {
			printf(GREEN "OK.\n" RESET);

			testc++;
			printf("Checking configuration if %s can be parsed: ", configfn);
			cJSON *dummy;

			if (readjson(configfn, &dummy)) {
				printf(RED "PROBLEM" RESET ": Cannot parse configuration.\n");
				problemc++;
			} else {
				printf(GREEN "OK.\n" RESET);
			}
			cJSON_Delete(dummy);
		} else {
			printf(RED "PROBLEM.\n" RESET);
			problemc++;
		}
	} else {
		printf(INFO_TAG
			   "Skipping configuration check, configuration file name not "
			   "found.\n");
	}

	// Check project configuration
	if (PCFG) {
		testc++;
		// Check build_config
		printf("\nChecking build_config object existence in loaded project "
			   "configuration: ");
		cJSON *bcfg = cJSON_GetObjectItemCaseSensitive(PCFG, "build_config");

		if (!bcfg) {
			printf(RED "PROBLEM.\n" RESET);
			problemc++;
		} else {
			printf(GREEN "OK.\n" RESET);

			testc++;
			// Check languages object
			printf("	Checking languages object existence in build_config: ");
			cJSON *lang = cJSON_GetObjectItemCaseSensitive(bcfg, "languages");

			if (!lang) {
				printf(RED "PROBLEM.\n" RESET);
				problemc++;
			} else {
				printf(GREEN "OK.\n" RESET);

				// Iterate each language
				cJSON *inlang = NULL;
				cJSON_ArrayForEach(inlang, lang) {
					testc++;
					// Check language executor
					printf("	Checking language %s executor: ",
						   inlang->string);
					cJSON *executor =
						cJSON_GetObjectItemCaseSensitive(inlang, "executor");
					if (!executor) {
						printf(RED "PROBLEM" RESET
								   ": Executor does not exist.\n");
						problemc++;
					} else {
						printf(GREEN "OK.\n" RESET);

						testc++;
						// Check if executor can be found in path
						printf("		Checking if %s found in path and can "
							   "be executed: ",
							   executor->valuestring);
						if (chkbin(executor->valuestring)) {
							printf(GREEN "OK.\n" RESET);
						} else {
							printf(RED "PROBLEM.\n" RESET);
							problemc++;
						}
					}
				}
			}

			testc++;
			// Check target object
			printf("	Checking target object existence in build_config: ");
			cJSON *target = cJSON_GetObjectItemCaseSensitive(bcfg, "target");

			if (!target) {
				printf(RED "PROBLEM.\n" RESET);
				problemc++;
			} else {
				printf(GREEN "OK.\n" RESET);

				// Check target health
				cJSON *starget = NULL;

				cJSON_ArrayForEach(starget, target) {
					// Iterate each build target
					cJSON *jtarget = NULL;

					cJSON_ArrayForEach(jtarget, starget) {
						printf(
							"\n		Checking target %s in build target %s.\n",
							jtarget->string, starget->string);
						testc++;
						// Check target source
						printf("		Checking source: ");
						cJSON *source =
							cJSON_GetObjectItemCaseSensitive(jtarget, "source");

						if (!source) {
							printf(RED "PROBLEM" RESET
									   ": Source does not exist.\n");
							problemc++;
						} else {
							printf(GREEN "OK.\n" RESET);

							// Check source
							// existence
							testc++;
							printf("		Checking source %s existence: ",
								   source->valuestring);
							if (!chkfexist(source->valuestring)) {
								printf(RED "PROBLEM.\n" RESET);
								problemc++;
							} else {
								printf(GREEN "OK.\n" RESET);
							}

							// Check if source is
							// readable
							testc++;
							printf("		Checking source %s readability: ",
								   source->valuestring);
							if (access(source->valuestring, R_OK) == 0) {
								printf(GREEN "OK.\n" RESET);
							} else {
								printf(RED "PROBLEM.\n" RESET);
								problemc++;
							}
						}

						// Check target language
						testc++;
						printf("		Checking language: ");
						cJSON *tlang = cJSON_GetObjectItemCaseSensitive(
							jtarget, "language");

						if (!tlang) {
							printf(RED "PROBLEM" RESET
									   ": Language not found.\n");
							problemc++;
						} else {
							printf(GREEN "OK.\n" RESET);
							if (lang) {
								testc++;
								printf("			Checking language %s "
									   "existence: ",
									   tlang->valuestring);
								cJSON *stlang =
									cJSON_GetObjectItemCaseSensitive(
										lang, tlang->valuestring);

								if (stlang) {
									printf(GREEN "OK.\n" RESET);
								} else {
									printf(RED "PROBLEM.\n" RESET);
									problemc++;
								}
							}
						}
					}
				}
			}
		}

		testc++;
		// Check mode
		printf("\nChecking mode object existence in loaded project "
			   "configuration: ");
		cJSON *mode = cJSON_GetObjectItemCaseSensitive(PCFG, "mode");

		if (!mode) {
			printf(RED "PROBLEM.\n" RESET);
			problemc++;
		} else {
			printf(GREEN "OK.\n" RESET);
		}
	} else {
		printf(INFO_TAG
			   "Skipping project configuration check, No project has been "
			   "initialized.\n");
	}

	// Check custom command directory
	if (ccmd_path) {
		testc++;
		printf("\n======== Custom Command Check ========\n");
		printf("Checking %s existence: ", ccmd_path);
		if (dir_exist(ccmd_path)) {
			printf(GREEN "OK.\n" RESET);

			DIR *ccmdr = opendir(ccmd_path);

			if (ccmdr) {
				struct dirent *entry;

				while ((entry = readdir(ccmdr)) != NULL) {
					if (entry->d_name[0] == '.' &&
						(entry->d_name[1] == '\0' ||
						 (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
						continue;
					// Open a new Lua State
					lua_State *eL = luaL_newstate();

					luaL_openlibs(eL);

					// Construct full path for the file
					char *fullpath =
						concat(ccmd_path, "/", entry->d_name, NULL);

					testc++;
					printf("Checking %s syntax: ", fullpath);

					// Check syntax
					int syntaxCorrect = 0;

					if (luaL_dofile(eL, fullpath)) {
						printf(RED "PROBLEM" RESET "\n"
								   "=======================================\n");
						fprintf(stderr, "%s\n", lua_tostring(eL, -1));
						printf("=======================================\n");
						problemc++;
					} else {
						syntaxCorrect = 1;
						printf(GREEN "OK.\n" RESET);
					}

					// Check if executable by Midorix
					if (syntaxCorrect) {
						testc++;
						printf("Checking if %s is executable by Midorix: ",
							   entry->d_name);
						lua_getglobal(eL, "main");
						if (!lua_isfunction(eL, -1)) {
							printf(YELLOW "WARNING" RESET
										  ": %s cannot be executed by Midorix "
										  "as a custom command.\n",
								   fullpath);
							warningc++;
						} else {
							printf(GREEN "OK.\n" RESET);
						}
					}

					// Cleanup
					free(fullpath);
					lua_close(eL);
				}
				closedir(ccmdr);
			} else {
				perror("opendir");
				problemc++;
			}
		} else {
			printf(YELLOW "WARNING" RESET ": %s directory does not exist.\n",
				   ccmd_path);
			warningc++;
		}
	} else {
		printf(
			INFO_TAG
			"Skipping custom command directory existence check, custom command "
			"directory name not found.\n");
	}

	printf("\n" INFO_TAG "Done checking Midorix.\n");
	printf("Total Test: %d\n", testc);
	printf("Problem: %d.\n", problemc);
	printf("Warning: %d\n", warningc);
}
