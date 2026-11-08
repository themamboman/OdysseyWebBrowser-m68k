/*
 * CyberGraphX runtime layer for m68k-AmigaOS3 (Agent D, RTG backend).
 *
 * proto/cybergraphics.h declares "extern struct Library *CyberGfxBase" and
 * every binding jumps through it. That symbol has to be *defined* somewhere,
 * and the library has to actually be opened, or the link succeeds and the
 * first call faults on a NULL base. Nothing in this tree currently does
 * either: there is no #include of a supplier and no OpenLibrary() call for
 * cybergraphics.library anywhere in the project.
 *
 * This is that missing piece. Create exactly one CyberGfx object and call
 * CyberGfx_Init() before any cybergraphics call, CyberGfx_Cleanup() after the
 * last one. Opening is idempotent and reference-counted so it is safe to call
 * from more than one place, and failure is reported rather than silently
 * leaving a NULL base.
 */

#ifndef RTG_LIB_H
#define RTG_LIB_H 1

#include <exec/types.h>

/*
 * The base used by proto/cybergraphics.h. Declared extern in that header,
 * DEFINED ONCE in rtg_lib.c -- do not turn this into a second definition
 * here, or -fno-common (GCC 10+) will fail every link with "multiple
 * definition of CyberGfxBase".
 */

/* Return values for CyberGfx_Init(). */
#define CYBERGFX_OK           0
#define CYBERGFX_NO_LIBRARY  (-1)   /* cybergraphics.library absent (plain AGA/ECS machine) */
#define CYBERGFX_BAD_VERSION (-2)   /* too old to be useful */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Opens cybergraphics.library v41 or newer. Safe to call repeatedly; returns 0
 * on success or one of the CYBERGFX_ error codes.
 */
int CyberGfx_Init(void);

/* Drops the reference taken by CyberGfx_Init(). Safe to over-call. */
void CyberGfx_Cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* RTG_LIB_H */
