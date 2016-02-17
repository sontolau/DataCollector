#include "libdc.h"
#include "log.h"

static char *timezone_now (char *buf, int szbuf) {
    struct tm tm;
    struct timeval time;
    char tmbuf[50] = {0};
    //time_t t = time (NULL);

    gettimeofday (&time, NULL);
    localtime_r (&time.tv_sec, &tm);
    strftime (tmbuf, sizeof (tmbuf), "%F %T",  &tm);

    snprintf (buf, szbuf, "%s.%lu", tmbuf, time.tv_usec);
    return buf;
}

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
    char timebuf[255] = {0};

    timezone_now (timebuf, sizeof (timebuf)-1);
    va_start(arg_ptr, fmt);
    fprintf (fp, "%s %s\t", timebuf, tag);
    vfprintf (fp, fmt, arg_ptr);
    fprintf (fp, "\n");
    fflush (fp);
}

