#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>

#ifndef POP_BRIGHTEN
#define POP_BRIGHTEN   0
#endif

#ifndef POP_DARKEN
#define POP_DARKEN     1
#endif

#if !defined(__AMIGAOS3__)
#ifndef ProcessPixelArray
VOID ProcessPixelArray(struct RastPort *, ULONG, ULONG, ULONG, ULONG, ULONG, LONG, struct TagItem *);
#endif
#endif

#include "gui.h"

struct Data
{
    ULONG brightness_threshold;
    ULONG brightness_checked;
};

DEFNEW
{
    FORTAG( INITTAGS )
    {
        case MUIA_Background:
            ((struct TagItem *)tag)->ti_Tag = TAG_IGNORE;
            break;

        case MUIA_Frame:
            ((struct TagItem *)tag)->ti_Tag = TAG_IGNORE;
            break;
    }
    NEXTTAG

    obj = (Object *) DoSuperNew(cl, obj,
        MUIA_CycleChain,   FALSE,
        InnerSpacing(0,0),
        TAG_MORE, INITTAGS
    );

    if (obj)
    {
        GETDATA;

        data->brightness_checked   = FALSE;
        data->brightness_threshold = 255;
    }

    return ((IPTR)obj);
}

#define SPACE_WIDTH  10

DEFMMETHOD(AskMinMax)
{
    ULONG maxwidth, defwidth, minwidth;

    DOSUPER;

    maxwidth = SPACE_WIDTH;
    defwidth = SPACE_WIDTH;
    minwidth = SPACE_WIDTH;

    msg->MinMaxInfo->MinWidth  += minwidth;
    msg->MinMaxInfo->DefWidth  += defwidth;
    msg->MinMaxInfo->MaxWidth  += maxwidth;

    msg->MinMaxInfo->MinHeight += 22;
    msg->MinMaxInfo->MaxHeight += 48;
    msg->MinMaxInfo->DefHeight += 22;

    return 0;
}

DEFMMETHOD(Draw)
{
    DOSUPER;

    if (msg->flags & MADF_DRAWOBJECT)
    {
        struct RastPort *rp = _rp(obj);

        ULONG mleft         = _mleft(obj);
        ULONG mtop          = _mtop(obj);
        ULONG mwidth        = _mwidth(obj);
        ULONG mheight       = _mheight(obj);

        ULONG vertoffs   = mleft + (mwidth / 2);

#if !defined(__AMIGAOS3__)
        ProcessPixelArray( rp, vertoffs,     mtop + 1, 1, mheight - 2, POP_BRIGHTEN, 70, NULL);
        ProcessPixelArray( rp, vertoffs - 1, mtop + 1, 1, mheight - 2, POP_DARKEN,   70, NULL);
#else
        /* ProcessPixelArray (brighten/darken existing pixels in place) is a
         * MorphOS-only cybergraphics.library extension, absent from the
         * classic CyberGraphX Developer Kit this target uses -- confirmed
         * not present anywhere in Agent D's DevKit FD/pragma tables. Same
         * visual intent (a two-line 3D groove bevel), drawn the classic way
         * with MUI's own shine/shadow pens instead of post-processing
         * existing pixels. */
        SetAPen(rp, _pens(obj)[MPEN_SHINE]);
        Move(rp, vertoffs, mtop + 1);
        Draw(rp, vertoffs, mtop + mheight - 2);

        SetAPen(rp, _pens(obj)[MPEN_SHADOW]);
        Move(rp, vertoffs - 1, mtop + 1);
        Draw(rp, vertoffs - 1, mtop + mheight - 2);
#endif
    }

    return 0;
}

BEGINMTABLE
DECNEW
DECMMETHOD(AskMinMax)
DECMMETHOD(Draw)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, spacerclass)

