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
    va_list arg_ptr;
    int szmaxlog = 0;
    char *strp;

    char timebuf[255] = {0};

    if ((strp = getenv (ENV_LOG_SIZE))) {
    	szmaxlog = atoi (strp);
    }

    if (szmaxlog > 0) {
    	if (ftell (fp) >= szmaxlog) {
    		fseek (fp, 0L, SEEK_SET);
    		ftruncate (fileno (fp), 0);
    	}
    }

    timezone_now (timebuf, sizeof (timebuf)-1);
    va_start(arg_ptr, fmt);
    fprintf (fp, "%s %s\t", timebuf, tag);
    vfprintf (fp, fmt, arg_ptr);
    fprintf (fp, "\n");
    fflush (fp);
}

