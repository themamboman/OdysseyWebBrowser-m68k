/*
 * CyberGraphX runtime layer for m68k-AmigaOS3 (Agent D, RTG backend).
 * See rtg_lib.h for why this exists.
 */

#include "rtg_lib.h"

#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/graphics.h>

/* cybergraphics.h is only needed for the version constant; the classic SDK
 * header is what defines CYBERGFXNAME. */
#include <cybergraphx/cybergraphics.h>

/* Single definition of the base proto/cybergraphics.h refers to. */
struct Library *CyberGfxBase = NULL;

int CyberGfx_Init(void)
{
    Forbid();

    if (CyberGfxBase) {
        Permit();
        return CYBERGFX_OK;
    }

    struct Library *base = OpenLibrary(CYBERGFXNAME, 41);

    if (!base) {
        Permit();
        return CYBERGFX_NO_LIBRARY;
    }

    if (base->lib_Version < 41) {
        CloseLibrary(base);
        Permit();
        return CYBERGFX_BAD_VERSION;
    }

    CyberGfxBase = base;

    Permit();
    return CYBERGFX_OK;
}

void CyberGfx_Cleanup(void)
{
    Forbid();

    if (CyberGfxBase) {
        CloseLibrary(CyberGfxBase);
        CyberGfxBase = NULL;
    }

    Permit();
}
