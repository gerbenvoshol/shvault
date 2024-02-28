#ifndef LIB_STR_H_
#define LIB_STR_H_

#ifdef __cplusplus
extern "C" {
#endif

char **strsplit(char *str, char sep, int *N);
char *read_stdin(char *prompt);

#ifdef __cplusplus
} // extern "C"
#endif

#endif