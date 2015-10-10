#include "str.h"

const char *DC_struncate (char *str)
{
    register char *ptr = str + strlen (str) - 1;

    while (ptr >= str && isspace ((int)*ptr)) {
        *ptr = '\0';
        ptr--;
    }

    ptr = str;
    while (*ptr && isspace ((int)*ptr)) {
        *ptr = '\0';
        ptr++;
    }

    return ptr;
}

const char *DC_strtok (char **str, const char *delim)
{
    char *tail = *str, *head = *str;
    int len = strlen (delim);

    if (!*str || !**str) return NULL;

    while (tail && *tail) {
        if (*tail == delim[0] && !strncmp (tail, delim,  len)) {
            *tail = '\0';
            *(tail + 1) = '\0';
            *str = tail + len;
            break;
        }
        tail++;
    }

    return head;
}
