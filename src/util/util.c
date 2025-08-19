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

void feval(const char *input, char *output, size_t maxlen) {
	size_t i = 0, j = 0;
	size_t len = strlen(input);

	// Check string lenght
	if (len >= 2 && ((input[0] == '"' && input[len - 1] == '"') ||
					 (input[0] == '\'' && input[len - 1] == '\''))) {
		// Remove first and last quote
		i = 1;
		len -= 1;
	}

	while (i < len && j < maxlen - 1) {
		if (input[i] == '\\') {
			i++;
			if (i >= len)
				break;
			switch (input[i]) {
				case 'n':
					output[j++] = '\n';
					break;
				case 't':
					output[j++] = '\t';
					break;
				case 'r':
					output[j++] = '\r';
					break;
				case '"':
					output[j++] = '"';
					break;
				case '\'':
					output[j++] = '\'';
					break;
				case '\\':
					output[j++] = '\\';
					break;
				case '0':
					output[j++] = '\0';
					break;
				default:
					output[j++] = input[i];
					break;
			}
		} else {
			output[j++] = input[i];
		}
		i++;
	}

	output[j] = '\0';
}

void sfeval(const char *input, char *output, size_t maxlen) {
	char buffer[maxlen];
	strncpy(buffer, input, maxlen);
	buffer[maxlen - 1] = '\0';

	feval(buffer, output, maxlen);
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
		if (argv[i + 1] != NULL) printf(" ");
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
