#include "util.h"
#include "../inih/ini.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/wait.h>

void dict_set(Dictionary* dict, const char* key, const char* value) {
	// Check if we are exceeding the maximum number of entries
	if (dict->count >= MAX_ENTRY) {
		printf("Dictionary is full, Cannot add more entries.\n");
		return;
	}

	// Ensure key and value are within allowed bounds
	if (strlen(key) >= MAX_KEY) {
		printf("Error: Key too long, Max allowed length is %d.\n", MAX_KEY - 1);
		return;
	}
	if (strlen(value) >= MAX_VALUE) {
		printf("Error: Value too long, Max allowed length is %d.\n", MAX_VALUE - 1);
		return;
	}

	// Copy the key and value into the dictionary
	strncpy(dict->key[dict->count], key, MAX_KEY - 1);
	strncpy(dict->value[dict->count], value, MAX_VALUE - 1);

	// Ensure null-termination in case the string is cut off
	dict->key[dict->count][MAX_KEY - 1] = '\0';
	dict->value[dict->count][MAX_VALUE - 1] = '\0';

	// Increment the count to track the number of entries
	dict->count++;
}


const char* dict_get(Dictionary *dict, const char *key) {
	for (int i = 0; i < dict->count; i++) {
		if (strcmp(dict->key[i], key) == 0) {
			return dict->value[i];
		}
	}
	return NULL;
}

int inihandler(void* user, const char* section, const char* name, const char* value) {
	Dictionary* dict = (Dictionary*)user;
	dict_set(dict, name, value);
	return 1;
}


int readini(const char* inifn, Dictionary *dict) {
	if (ini_parse(inifn, inihandler, dict) < 0) {
		fprintf(stderr, "Failed to parse configuration file.\n");
		return 1;
	}
	return 0;
}

int chkfexist(const char* filename) {
	return access(filename, F_OK);
}

void rmquote(const char* src, char* dest, size_t maxlen) {
	size_t len = strlen(src);

	if ((src[0] == '"' && src[len - 1] == '"') || (src[0] == '\'' && src[len - 1] == '\'')) {
		size_t clen = len - 2;
		if (clen >= len) clen = maxlen - 1;

		strncpy(dest, src + 1, clen);
		dest[clen] = '\0';
	} else {
		strncpy(dest, src, maxlen - 1);
		dest[maxlen - 1] = '\0';
	}
}

void unescape(const char* input, char* output, size_t maxlen) {
	size_t i = 0, j = 0;

	while (input[i] != '\0' && j < maxlen - 1) {
		if (input[i] == '\\') {
			i++;
			switch (input[i]) {
				case 'n': output[j++] = '\n'; break;
				case 't': output[j++] = '\t'; break;
				case 'r': output[j++] = '\r'; break;
				case '"': output[j++] = '"'; break;
				case '\'': output[j++] = '\''; break;
				case '\\': output[j++] = '\\'; break;
				case '0': output[j++] = '\0'; break;
			}
		} else {
			output[j++] = input[i];
		}
		i++;
	}
	output[j] = '\0';
}

void feval(const char* input, char* output, size_t maxlen) {
	rmquote(input, output, maxlen);
	unescape(output, output, maxlen);
}

void ssplit(const char* input, wordexp_t* dest) {
	wordfree(dest);

	int ret = wordexp(input, dest, 0);
	if (ret != 0) {
		dest->we_wordc = 0;
		dest->we_wordv = NULL;
	}
}

void flush_stdin() {
	int c;
	while ((c = getchar()) != '\n' && c != EOF);
	return;
}

void execcmd(char* argv[]) {
	pid_t pid = fork();

	if (pid == 0) {
		// Child Process
		execvp(argv[0], argv);
		perror("execvp failed");
		exit(EXIT_FAILURE);
	} else if (pid > 0) {
		// Parent Process
		int status;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status)) {
			printf("Command exited with code: %d\n", WEXITSTATUS(status));
		} else if (WTERMSIG(status)) {
			printf("Command terminated with signal: %d\n", WTERMSIG(status));
		}
	} else {
		perror("fork failed");
	}
}
