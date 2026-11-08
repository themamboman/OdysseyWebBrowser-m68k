#ifndef GRAPHICS_GFXBASE_H
#define GRAPHICS_GFXBASE_H

#include "exec/exec.h"
#include "graphics/gfx.h"

#define GRAPHICS_GFXBASE_H

typedef struct GfxBase {
    struct Library gfx_LibNode;
    struct DisplayInfo* gfx_DisplayInfo;
    struct ViewPort* gfx_ViewPort;
    struct ViewPortExtra* gfx_ViewPortExtra;
    struct MonitorSpec* gfx_MonitorSpec;
    APTR gfx_DefaultFont;
    UWORD gfx_Flags;
    UWORD gfx_pad;
} GfxBase;

extern struct GfxBase* GfxBase;

#endif /* GRAPHICS_GFXBASE_H */