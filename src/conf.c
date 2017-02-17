
#include "conf.h"
#include "errcode.h"
#include "str.h"

FASTCALL bool_t get_key_value(char *buf, char **p, int num) {
    char *ptr = NULL;

    p[0] = NULL;
    p[1] = NULL;

    ptr = strchr(buf, '=');
    if (!ptr) return FALSE;

    *ptr = '\0';
    ptr++;

    p[0] = DC_strip(buf);
    p[1] = DC_strip(ptr);

    return TRUE;
}

FASTCALL bool_t get_section_name(char *buf, char *sec, uint8_t size) {
    int len = strlen(buf);

    if (!(len > 0 && buf[0] == '[' && buf[len - 1] == ']')) {
        return FALSE;
    }


    buf[0] = '\0';
    buf[len - 1] = '\0';

    strncpy(sec, DC_strip(&buf[1]), size);
    return TRUE;
}

err_t DC_read_ini(const char *path,
                  bool_t (*cb)(const char *,
                               const char *,
                               char *)) {
    FILE *fp;
#define MAX_SZSEC 255
    uint32_t size = 0;
    char *buffer = NULL;
    char *bufptr = NULL;
    char section[MAX_SZSEC] = {0};
    char *p[2] = {NULL};
    int err = E_OK;

    if (ISNULL ((fp = fopen(path, "r")))) {
        return E_SYSTEM;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    if (ISNULL ((buffer = malloc(size + 1)))) {
        fclose(fp);
        return E_NOMEM;
    }

    while (!feof(fp)) {
        memset(buffer, '\0', size + 1);
        if (ISNULL (bufptr = fgets(buffer, size, fp))) {
            break;
        }

        bufptr = DC_strip(buffer);
        if (!(*bufptr)) continue;

        //skip the comment line.
        if (bufptr[0] == '#') continue;

        // test if it's section name.
        if (get_section_name(bufptr, section, sizeof(section))) {
            continue;
        }

        if (!get_key_value(bufptr, p, 2)) {
            continue;
        }

        if (!(p[0] && p[1])) continue;

        if (cb) {
            if (!cb(section, p[0], p[1])) {
                err = E_ERROR;
                break;
            }
        }
    }

    fclose(fp);
    free(buffer);

    return err;
}
