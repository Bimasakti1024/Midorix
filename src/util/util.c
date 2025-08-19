// src/util/util.c
#include "util.h"
#include <cjson/cJSON.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

char *readf(const char *path) {
	FILE *f = fopen(path, "rb");
	if (!f)
		return NULL;

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	rewind(f);

	char *buffer = (char *)malloc(size + 1);
	fread(buffer, 1, size, f);
	buffer[size] = '\0';
	fclose(f);
	return buffer;
}

int readjson(const char *jsonf, cJSON **jsonout) {
	char  *buffer = readf(jsonf);
	cJSON *parsed = cJSON_Parse(buffer);
	free(buffer);
	if (!parsed) {
		fprintf(stderr, "Failed to parse JSON.\n");
		return 1;
	}
	cJSON *dup = cJSON_Duplicate(parsed, 1);
	cJSON_Delete(parsed);

	if (!dup)
		return 1;

	*jsonout = dup;
	return 0;
}

int chkfexist(const char *filename) {
	return access(filename, F_OK);
}

char *append(const char *fstr, const char *sstr) {
	int size = strlen(fstr) + strlen(sstr) + 1;

	char *result = malloc(size);
	if (!result) {
		perror("malloc");
		return NULL;
	}
	snprintf(result, size, "%s%s", fstr, sstr);
	return result;
}

int ssplit(const char *input, wordexp_t *dest) {
	int ret = wordexp(input, dest, 0);
	if (ret != 0) {
		dest->we_wordc = 0;
		dest->we_wordv = NULL;
		return ret;
	}
	return 0;
}

void flush_stdin() {
	int c;
	while ((c = getchar()) != '\n' && c != EOF)
		;
	return;
}
void execcmd(char *argv[]) {
	for (int i = 0; argv[i] != NULL; i++) {
		printf("%s", argv[i]);
		if (argv[i + 1] != NULL)
			printf(" ");
	}
	printf("\n");
	pid_t pid = fork();

	if (pid == 0) {
		// Child Process
		execvp(argv[0], argv);
		perror("execvp");
		exit(EXIT_FAILURE);
	} else if (pid > 0) {
		// Parent Process
		int status;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status)) {
			printf("\nCommand exited with code: %d\n", WEXITSTATUS(status));
		} else if (WTERMSIG(status)) {
			printf("\nCommand terminated with signal: %d\n", WTERMSIG(status));
		}
	} else {
		perror("fork");
	}
}
