#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int count_char(char *str, char sep) {
    int count = 0;
    while (*str) {
        if (*str == sep) count++;
        str++;
    }
    return count;
}

int strindexC(char *s, char sep) {
    char *found = strchr(s, sep);
    return (found != NULL) ? (found - s) : -1;
}

char **strsplit(char *str, char sep, int *N) {
    *N = count_char(str, sep) + 1;
    char **s = (char **)malloc(sizeof(char*) * (*N));
    if (!s) return NULL; // Check malloc success

    int i, idx1 = 0, idx2, next;
    for (i = 0; i < *N; i++) {
        idx2 = strindexC(str + idx1, sep);
        if (idx2 == -1) idx2 = strlen(str) - idx1; // Adjust if no more separators
        s[i] = (char *)malloc(sizeof(char) * (idx2 + 1));
        if (!s[i]) { // Handle malloc failure
            for (int j = 0; j < i; j++) free(s[j]); // Free previously allocated strings
            free(s); // Free the string array
            return NULL;
        }
        memcpy(s[i], str + idx1, idx2);
        s[i][idx2] = '\0'; // Null terminate
        idx1 += idx2 + 1; // Move past the current string and separator
    }

    return s;
}

char *read_stdin(char *prompt)
{
    size_t cap = 4096, /* Initial capacity for the char buffer */
           len =    0; /* Current offset of the buffer */
    char *buffer = malloc(cap * sizeof (char));
    int c;
    
    if (prompt) {
        fprintf(stderr, "%s\n", prompt);
    }

    /* Read char by char, breaking if we reach EOF or a newline */
    while ((c = fgetc(stdin)) && !feof(stdin)) {
        if (prompt && c == '\n') {
            break;
        }

        buffer[len] = c;

        /* When cap == len, we need to resize the buffer
        * so that we don't overwrite any bytes
        */
        if (++len == cap) {
            /* Make the output buffer twice its current size */
            buffer = realloc(buffer, (cap *= 2) * sizeof (char));
        }
    }

    /* Trim off any unused bytes from the buffer */
    buffer = realloc(buffer, (len + 1) * sizeof (char));

    /* Pad the last byte so we don't overread the buffer in the future */
    buffer[len] = '\0';

    return buffer;
}