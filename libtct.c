#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdbool.h>

#include "libtct.h"

#pragma warning(disable : 4996)

tct_arguments_t* tct_add_argument_(tct_arguments_t *next_argument, char const *name, char *format, ...) {
	va_list argp;
	tct_arguments_t* argument;
	size_t arg_len, name_len;

	name_len = strlen(name);
	va_start(argp, format);
	arg_len = vsnprintf(NULL, 0, format, argp);
	va_end(argp);
	argument = calloc(1, sizeof(tct_arguments_t) + name_len + 1 + arg_len + 1);

	if (argument != NULL) {

		strcpy(argument->data, name);
		va_start(argp, format);
		vsnprintf(&argument->data[name_len + 1], arg_len + 1, format, argp);
		va_end(argp);
		argument->next = next_argument;

	}
	return argument;
}

tct_arguments_t* tct_find_arguments(tct_arguments_t *arguments, char const *name, const size_t name_len) {
	while (arguments)
		if (memcmp(arguments->data, name, name_len) == 0 && arguments->data[name_len] == 0)
			return arguments;
		else
			arguments = arguments->next;
	return NULL;
}

char* tct_get_valuen(tct_arguments_t* arguments, char const *name, size_t name_len) {
	tct_arguments_t* argument = tct_find_arguments(arguments, name, name_len);
	return argument ? &argument->data[name_len + 1] : "";
}

static bool tct_find_symbol(char *template,  char **start_,  char  **end_) {
	char* start;
	char* end;

	*start_ = NULL;
	*end_ = NULL;

	start = template;

	while (*start) {
		if (memcmp(start, TCT_START_SIGN, TCT_START_SIGN_LEN) != 0) {
			start++; continue;
		}

		end = start + TCT_START_SIGN_LEN;

		while (*end) {
			if (memcmp(end, TCT_END_SIGN, TCT_END_SIGN_LEN) != 0) {
				end++; continue;
			}

			*start_ = start;
			*end_ = end;

			return true;
		}
		break;
	}

	return false;
}

void tct_free_argument(tct_arguments_t* arguments) {
	while (arguments) {
		tct_arguments_t* argument = arguments;
		arguments = argument->next;
		free(argument);
	}
}

char* tct_render( char *template, tct_arguments_t* argument) {
	tct_section_t* section_start;
	tct_section_t* section_current;

	char* start;
	char* end;

	char* result = NULL;

	section_start = calloc(1, sizeof(tct_section_t));

	if (section_start != NULL) {
		section_current = section_start;

		size_t result_len = strlen(template);
		while (tct_find_symbol(template, &start, &end)) {

			section_current->data = template;
			section_current->length = start - template;
			result_len += section_current->length;
			section_current->next = calloc(1, sizeof(tct_section_t));

			if (section_current->next != NULL) {
				section_current = section_current->next;

				char* trim_start, * trim_end;

				trim_start = start + TCT_START_SIGN_LEN;
				trim_end = end;
				while (isspace(trim_start[0])) trim_start++;
				while (isspace(trim_end[-1])) trim_end--;
				section_current->data = tct_get_valuen(argument, trim_start, trim_end - trim_start);
				section_current->length = strlen(section_current->data);
				result_len += section_current->length;
				section_current->next = calloc(1, sizeof(tct_section_t));
				section_current = section_current->next;

				template = end + TCT_END_SIGN_LEN;
			}
		}
		section_current->data = template;
		section_current->length = strlen(template);

		result = malloc(result_len + 1);

		if (result != NULL) {

			char* write = result;

			while (section_start) {
				tct_section_t* section = section_start;
				memcpy(write, section->data, section->length);
				write += section->length;
				section_start = section->next;
				free(section);
			}
			*write = 0;
		}
	}
	return result;
}