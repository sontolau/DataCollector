#ifndef _DC_LOG_H
#define _DC_LOG_H

#define ENV_LOG_SIZE "MAX_LOG_SIZE"

extern void  DC_log (const char *tag, const char *fmt, ...);

#define Dlog(fmt, ...) DC_log("DEBUG", fmt,  ## __VA_ARGS__)
#define Elog(fmt, ...) DC_log("ERROR", fmt,  ## __VA_ARGS__)
#define Wlog(fmt, ...) DC_log ("WARN", fmt,  ## __VA_ARGS__)

#endif //
