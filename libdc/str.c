#include "libdc.h"
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
            memset (tail, '\0', len);
            *str = tail + len;
            break;
        }
        tail++;
    }

    return head;
}

int DC_substr (const char *str, const char *substr, int start)
{
	register char *ptr;

	int szsubstr = strlen (substr);
	int szstr = strlen (str);

	if (start >= szstr) return -1;
	if (szsubstr > szstr) return -1;

	ptr = (char*)str + start;
	while (*ptr) {
		if (szstr - (ptr - str) < szsubstr) {
			break;
		}

		if (!strncmp (ptr, substr, szsubstr)) {
			return ptr - str;
		}
		ptr++;
	}

	return -1;
}
