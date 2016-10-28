#include "conf.h"
#include "errcode.h"

err_t DC_read_ini (const char *path,
                   bool_t (*cb) (const char*,
                                 const char*,
                                 char*))
{
    FILE *fp;
    uint32_t size = 0;
    char *buffer = NULL;
    register char *bufptr = NULL;
    char *secptr = NULL;

    if (ISNULL ((fp = fopen (path, "r")))) {
        return E_SYSTEM;
    }

    fseek (fp, 0, SEEK_END);
    size = ftell ();
    rewind (fp);

    if (ISNULL ((buffer = malloc (size+1)))) {
        fclose (fp);
        return E_NOMEM;
    }

    while (!feof (fp)) {
        memset (buffer, '\0', size+1);
        if (ISNULL (bufptr = fgets (buffer, size, fp))) {
            break;
        }
        
        bufptr = DC_strip (buffer);
        if (!(*bufptr)) continue;

        
    }
}
