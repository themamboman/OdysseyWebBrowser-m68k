/* Minimal shims for AmigaOS3 m68k, needed by sqlite3's unix VFS.
 * Injected with gcc -include, so no sqlite source is patched.
 * Deliberately avoids <proto/dos.h> and <sys/time.h>: both transitively pull
 * in the AmigaOS DOS "struct DateTime", which collides with sqlite's own
 * internal struct DateTime of the same name. Only Delay() is needed here, so
 * it is forward-declared instead of dragging the header in. */
#ifndef _AMIGAOS_POSIX_SHIM_H
#define _AMIGAOS_POSIX_SHIM_H

#include <sys/types.h>

#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
struct timespec {
    long tv_sec;
    long tv_nsec;
};
#endif

/* dos.library's busy-wait, in vertical-blank ticks (20 ms on PAL). */
extern void Delay(long);

/* AmigaOS has no nanosleep(); round up so sleeps are never short. */
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

#endif
