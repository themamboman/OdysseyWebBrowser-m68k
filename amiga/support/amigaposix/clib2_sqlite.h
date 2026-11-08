/* Forced include for building SQLite's unix VFS against clib2 on AmigaOS 3.x (-include).
 * exec/types.h defines GLOBAL, which SQLite uses for its own macro. clib2 has struct timespec
 * but no nanosleep(). utimes() comes from libamigaposix. */
#ifndef OWB_CLIB2_SQLITE_H
#define OWB_CLIB2_SQLITE_H
#include <locale.h>
#include <exec/types.h>
#undef GLOBAL
#include <time.h>
#include <sys/types.h>
extern void Delay(long);
static inline int nanosleep(const struct timespec *req, struct timespec *rem)
{
    if (!req)
        return -1;
    unsigned long ticks = (req->tv_sec * 1000UL + req->tv_nsec / 1000000UL + 19UL) / 20UL;
    if (ticks)
        Delay(ticks);
    if (rem) { rem->tv_sec = 0; rem->tv_nsec = 0; }
    return 0;
}
struct timeval;
int utimes(const char *, const struct timeval *);
#endif
