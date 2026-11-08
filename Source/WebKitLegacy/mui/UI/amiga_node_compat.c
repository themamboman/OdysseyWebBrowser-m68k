/* aros-overrides.h's NEWLIST/REMOVE/ADDTAIL/ADDHEAD/REMHEAD/REMTAIL macros
 * need to name exec.library's `struct Node`, but several files that include
 * aros-overrides.h (via mui.h) also transitively pull in WebCore headers
 * that make `Node` ambiguous in the global namespace -- no amount of `::`
 * qualification at the macro's use site or definition site fixes this,
 * because the ambiguity is empirically present regardless of ordering (see
 * WebCore_status.md). The only ordering-independent fix: do the real
 * struct Node work in a clean translation unit that never sees a WebCore
 * header, and expose it through void*-typed wrapper functions so the
 * ambiguous call sites never need to name "Node" themselves at all.
 */
#if defined(__AMIGAOS3__)

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <pwd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

/* Delay() is an inline/dos.h macro (__Delay_base LVO call), not a real
 * symbol -- sqlite3.a's prebuilt nanosleep() was compiled expecting a
 * real linkable Delay(), without <proto/dos.h> in scope at its own build
 * time (confirmed via a real OWB link: "undefined reference to Delay" --
 * see WebCore_status.md). dos.library is auto-opened by crt0 startup
 * (DOSBase, DOS_BASE_NAME's target, needs no explicit OpenLibrary call),
 * so this just needs to be a real function wrapping the same LVO macro
 * main.cpp's own dos.library calls already use. */
#undef Delay
VOID Delay(LONG timeout)
{
    __Delay_base((DOS_BASE_NAME), timeout);
}

/* clock_gettime() doesn't exist in clib2 at all, but gettimeofday() does
 * (confirmed via nm) -- CLOCK_REALTIME/CLOCK_MONOTONIC aren't meaningfully
 * different on this target (no real monotonic clock source exposed by
 * clib2), so both just forward to gettimeofday(). Needed by libcurl's
 * Curl_now() (timeval.c). */
int clock_gettime(int clk_id, struct timespec *tp)
{
    struct timeval tv;
    (void)clk_id;
    if (gettimeofday(&tv, 0) != 0)
        return -1;
    tp->tv_sec = tv.tv_sec;
    tp->tv_nsec = tv.tv_usec * 1000;
    return 0;
}

/* nl_langinfo() doesn't exist in clib2 -- needed by ICU's
 * uprv_getDefaultCodepage_55() (putil.cpp) purely to pick a fallback
 * codepage name when nothing more specific is configured. No real
 * per-user locale/codeset mechanism exists on this libc to query
 * honestly, so this always reports UTF-8 -- a reasonable default for
 * modern web content, not an attempt to reflect actual system
 * configuration. `item` is accepted as a plain int (matching nl_item's
 * usual `typedef int nl_item` on every platform that has one) since this
 * file doesn't need <langinfo.h>'s actual CODESET constant to ignore it. */
char *nl_langinfo(int item)
{
    (void)item;
    return (char *)"UTF-8";
}

/* fnmatch() doesn't exist in clib2 -- needed by libcurl's Curl_fnmatch()
 * for FTP wildcard downloads (an edge feature; most HTTP/HTTPS browsing
 * never calls this at all). Simple recursive glob matcher: '*' (any run
 * of characters), '?' (any one character), literal characters otherwise.
 * Deliberately does not support POSIX bracket expressions ([abc],
 * [!abc]) -- undocumented in curl's own minimal usage of this function,
 * and not worth the complexity for a rarely-exercised FTP-only path.
 * FNM_NOMATCH's standard value is 1. */
#define FNM_NOMATCH 1
static int fnmatch_inner(const char *pattern, const char *string)
{
    while (*pattern)
    {
        if (*pattern == '*')
        {
            while (*pattern == '*')
                pattern++;
            if (!*pattern)
                return 1;
            while (*string)
            {
                if (fnmatch_inner(pattern, string))
                    return 1;
                string++;
            }
            return 0;
        }
        else if (*pattern == '?')
        {
            if (!*string)
                return 0;
            pattern++;
            string++;
        }
        else
        {
            if (*pattern != *string)
                return 0;
            pattern++;
            string++;
        }
    }
    return *string == '\0';
}

int fnmatch(const char *pattern, const char *string, int flags)
{
    (void)flags;
    return fnmatch_inner(pattern, string) ? 0 : FNM_NOMATCH;
}

/* pipe() has no real equivalent on this target -- AmigaOS has no POSIX
 * pipe primitive, and clib2 doesn't provide one. Needed by libcurl's
 * Curl_pipe() (socketpair.c), used only for libcurl's internal
 * multi-handle wakeup mechanism (an optimization for cancelling a
 * blocking wait early), not for actual HTTP/HTTPS data transfer -- an
 * honest failure here (curl treats a failed pipe() as "wakeup
 * optimization unavailable" and falls back to its normal polling path,
 * not a transfer failure) is the right degradation, not a guess at fake
 * pipe semantics. */
int pipe(int fildes[2])
{
    (void)fildes;
    errno = ENOSYS;
    return -1;
}

/* __timezone/__tzname: ICU's uprv_timezone_55()/uprv_tzname_55()
 * (putil.cpp) read these as plain global data, newlib-style naming
 * (double underscore) -- clib2 has no timezone-related globals at all
 * (confirmed via nm: tzset()/localtime() exist as real functions, but
 * nothing populates any timezone/tzname-style global as a side effect).
 * Fixed at UTC/0 offset -- an honest limitation (no real per-user
 * timezone configuration support on this target), not a guess at
 * detecting the actual system timezone. */
long __timezone = 0;
char *__tzname[2] = { (char *)"UTC", (char *)"UTC" };

void Amiga_NewList(void *list)
{
    struct List *l = (struct List *)list;
    l->lh_TailPred = (struct Node *)l;
    l->lh_Tail     = 0;
    l->lh_Head     = (struct Node *)&l->lh_Tail;
}

void Amiga_Remove(void *n)
{
    Remove((struct Node *)n);
}

void *Amiga_RemHead(void *l)
{
    return RemHead((struct List *)l);
}

void Amiga_AddTail(void *l, void *n)
{
    AddTail((struct List *)l, (struct Node *)n);
}

void Amiga_AddHead(void *l, void *n)
{
    AddHead((struct List *)l, (struct Node *)n);
}

void *Amiga_RemTail(void *l)
{
    return RemTail((struct List *)l);
}

/* libcurl's netrc.c calls getpwuid(geteuid()) to find $HOME via the user
 * database when determining where ~/.netrc lives, and already handles a
 * NULL return gracefully (falls through to its other home-directory
 * detection). clib2's pwd.h says getpwuid() is normally supplied by the
 * optional usergroup.library -- not guaranteed present on any given
 * AmigaOS install, and genuinely absent from every libc variant in this
 * toolchain (confirmed via nm). Simplest honest answer: no such library,
 * no such user. */
struct passwd *getpwuid(uid_t uid)
{
    (void)uid;
    return 0;
}

#endif /* __AMIGAOS3__ */
