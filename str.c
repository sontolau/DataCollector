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

static inline char *__find_str (char **start, const char *end)
{
    register char *ptr = *start;

    while (ptr<end && !(*ptr)) ptr++;

    if (ptr>=end) return NULL;

    *start = (ptr+strlen (ptr));

    return ptr;
}

char **DC_split_str (char *str, const char *delim, int *numstr)
{
    char *ptr = str;
    int num = 0, i;
    int szdelim = strlen (delim);
    int szstr   = strlen (str);
    char **p = NULL;

    while (*ptr) {
        if (!strncmp (ptr, delim, szdelim)) {
            memset (ptr, '\0', szdelim);
            num++;
            ptr += szdelim;
        } else {
            ptr++;
        }   
    }

    if (!(p = calloc (num+1, sizeof (char*)))) {
        return NULL;
    }

    for (i=0, ptr=str; i<num && (p[i] = __find_str (&ptr, &str[szstr])); i++);
    if (numstr) *numstr = i;

    return p;
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
