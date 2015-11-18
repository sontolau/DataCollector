#include "libdc.h"
#include "log.h"

void DC_log (const char *tag, const char *fmt, ...)
{
    FILE *fp = stdout;
    //static FILE *logfp = NULL;
    //static unsigned int logsize = 0;
    
    //char *env;
    va_list arg_ptr;
/*
    if (logfp == NULL) {
        env = getenv ("LOG_FILE");
        if (!env) {
            logfp = stdout;
        } else if (!strcmp (env, "STDOUT")) {
            logfp = stdout;
        } else if (!strcmp (env, "STDERR")) {
            logfp = stderr;
        } else {
            logfp = fopen (env, "w+");
        }

        env = getenv ("LOG_SIZE");
        if (env) {
            logsize = strtoul (env, NULL, 10);
        }
    }

    if (logfp == NULL)  {
        logfp = stdout;
        setlinebuf (logfp);
    }

    if (logsize && ftell (logfp) > logsize) {
        rewind (logfp);
    }
*/
    va_start(arg_ptr, fmt);
    fprintf (fp, "%s\t", tag);
    vfprintf (fp, fmt, arg_ptr);
    fprintf (fp, "\n");
    fflush (fp);
}

