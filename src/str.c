#include "libdc.h"
#include "str.h"

char *DC_strip(char *str) {
    register char *ptr = str + strlen(str) - 1;

    while (ptr >= str && isspace((int) *ptr)) {
        *ptr = '\0';
        ptr--;
    }

    ptr = str;
    while (*ptr && isspace((int) *ptr)) {
        *ptr = '\0';
        ptr++;
    }

    return ptr;
}

char *DC_strtok(char **str, const char *delim) {
    char *tail = *str, *head = *str;
    int len = strlen(delim);

    if (!*str || !**str) return NULL;

    while (tail && *tail) {
        if (*tail == delim[0] && !strncmp(tail, delim, len)) {
            memset(tail, '\0', len);
            *str = tail + len;
            break;
        }
        tail++;
    }

    return head;
}

int DC_substr(const char *str, const char *substr, int start) {
    register char *ptr;

    int szsubstr = strlen(substr);
    int szstr = strlen(str);

    if (start >= szstr) return ERR;
    if (szsubstr > szstr) return ERR;

    ptr = (char *) str + start;
    while (*ptr) {
        if (szstr - (ptr - str) < szsubstr) {
            break;
        }

        if (!strncmp(ptr, substr, szsubstr)) {
            return ptr - str;
        }
        ptr++;
    }

    return ERR;
}

static inline char *__find_str(char **start, const char *end) {
    register char *ptr = *start;

    while (ptr < end && !(*ptr)) ptr++;

    if (ptr >= end) return NULL;

    *start = (ptr + strlen(ptr));

    return ptr;
}

#define __find_start(p, endptr, delim, dlen) \
do {\
    while ((p < endptr) && !strncmp (p, delim, dlen)) {\
        p += dlen;\
    }\
    if (p >= endptr) p = NULL;\
} while (0)

#define __find_end(p, endptr, delim, dlen) \
do {\
    while ((p < endptr) && strncmp (p, delim, dlen)) p++;\
    if (p >= endptr) p = NULL;\
} while (0)

char **DC_split_str(const char *src, const char *delim, int *numstr) {
    char *ptr = NULL, *endptr = NULL;
    char *str = NULL;
    int num = 0, i;
    int szdelim = strlen(delim);
    char **p1 = NULL, **p2 = NULL;
    
    ptr = str = strdup (src);
    endptr = ptr + strlen(str);

    while (ptr < endptr) {
        __find_start (ptr, endptr, delim, szdelim);
        if (ptr) {
            num++;
            p2 = p1;
            p1 = realloc(NULL, (num + 1) * sizeof(char *));
            for (i = 0; p2 && i < num - 1; i++) {
                p1[i] = p2[i];
            }
            if (p2) free(p2);
            p1[num - 1] = strdup (ptr);
        } else {
            break;
        }

        __find_end (ptr, endptr, delim, szdelim);
        if (ptr) {
            *ptr = '\0';
            ptr += szdelim;
        } else {
            break;
        }
    }

    free (str);

    if (numstr) *numstr = num;

    return p1;
}

/*
int main ()
{
   char str[] = ",how,are,you,,good,,,,,love,you";
   int i;
   char **p = DC_split_str (str, ",");

   for (i=0; p[i]; i++) {
      printf ("%s\n", p[i]);
   }

   return 0;
}*/
