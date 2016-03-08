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

int DC_log_open (const char *path, unsigned int maxsz)
{
    FILE *fp = NULL;
    static char szbuf[100] = {0};

    if (!(fp = fopen (path, "w+"))) {
        return -1;
    } else {
        fseek (fp, 0, SEEK_END);
    }

    if (maxsz > 0) {
        snprintf (szbuf, sizeof (szbuf), "%s=%u", ENV_LOG_SIZE, maxsz);
        putenv (szbuf);
    }

    fclose (stdin);
    fclose (stdout);
    fclose (stderr);

    dup2 (fileno (fp), fileno (stdin));
    dup2 (fileno (fp), fileno (stdout));
    dup2 (fileno (fp), fileno (stderr));

    return 0;
}
