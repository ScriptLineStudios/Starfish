#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

#ifndef STR_HEADER
#define STR_HEADER

typedef struct {
    char *string;
    size_t length;
    size_t consumption;
} Str;

Str str_new(char *src);
Str str_concat(Str dst, Str src);
Str str_token(Str *str, char delim);
void str_free(Str str);

#endif

#ifdef STR_HEADER_IMPLEMENTATION

Str str_new(char *string) {
    char *src = (char *)malloc(sizeof(char) * strlen(string) + 1);
    strcpy(src, string);
    return (Str){.string=src, .length=strlen(src), .consumption=0};
}

Str str_new_internal(char *string) {
    return (Str){.string=string, .length=strlen(string), .consumption=0};
}

Str str_concat(Str dst, Str src) {
    char *new = (char *)malloc(sizeof(char) * (dst.length + src.length));
    for (int i = 0; i < dst.length; i++) {
        new[i] = dst.string[i];
    }
    for (int i = 0; i < src.length; i++) {
        new[i + dst.length] = src.string[i];
    }
    free(dst.string);
    free(src.string);

    return str_new_internal(new);
}

Str str_append(Str str, char c) {
    str.length++;
    str.string = realloc(str.string, sizeof(char) * str.length + 1);
    str.string[str.length - 1] = c;
    str.string[str.length] = '\0';
    return str;
}

Str str_token(Str *str, char delim) {
    Str tmp = str_new("");
    for (size_t i = str->consumption; i < str->length; i++) {
        if (str->string[i] == delim) {
            str->consumption++;
            break;
        }
        else {
            tmp = str_append(tmp, str->string[i]);
            str->consumption++;
        }
    }

    return tmp;
}

Str str_read_line(FILE *stream) {
    Str tmp = str_new("");

    char c;
    while ((c = fgetc(stream)) != '\n') {
        if (feof(stream)) {
            break;
        }
        tmp = str_append(tmp, c);
    }

    return tmp;
}

void str_free(Str str) {
    free(str.string);
}

#endif
