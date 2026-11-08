#ifndef AROS_OVERRIDES_H
#define AROS_OVERRIDES_H

#ifdef __cplusplus
extern "C" {
#endif
IPTR DoSuperNew(struct IClass *cl, Object *obj, Tag tag1, ...);
APTR AllocVecTaskPooled(ULONG byteSize);
VOID FreeVecTaskPooled(APTR memory);
APTR ARGB2BGRA(APTR src, ULONG stride, ULONG height);
VOID ARGB2BGRAFREE(APTR dst);
#ifdef __cplusplus
}
#endif

/* These #undef's were written for AROS's own compiler/headers, which
 * apparently have a real conflict with these inline macros ("something
 * about variadic", per the original comment). Classic AmigaOS3 doesn't
 * have that problem -- confirmed by the whole project already relying on
 * these exact inline macros successfully elsewhere (MUI_NewObject/
 * MUI_MakeObject's own vararg handling, fixed earlier this session).
 * Applying the undef here too (this file started firing for __AMIGAOS3__
 * as well as __AROS__ earlier this session) permanently breaks SetAttrs/
 * NewObject project-wide: the #include <proto/intuition.h> right below
 * that's meant to restore them is a no-op once that header's already been
 * included earlier in the same TU (which it always has, via gui.h/mui.h),
 * so the macros never come back -- confirmed via a real OWB link showing
 * "undefined reference to SetAttrs/NewObject" in nearly every UI file
 * (see WebCore_status.md). Scoped back to AROS only. */
#if defined(__AROS__)
#undef DoMethod
#undef SetAttrs
#undef CoerceMethod
#undef NewObject
#endif
#include <proto/intuition.h>

typedef struct _struct_Msg _Msg_;

/* struct Node is ambiguous in any translation unit that also pulls in
 * WebCore headers (confirmed empirically -- see WebCore_status.md: no
 * amount of :: qualification or reordering fixes it, because some WebCore
 * header already makes Node ambiguous before this point is even reached in
 * the files that hit it). Fix: the real struct Node manipulation now lives
 * in amiga_node_compat.c, a clean translation unit with no WebCore headers,
 * exposed here through void*-typed wrapper functions so these macros never
 * need to name "Node" at their use site at all. */
#ifdef __cplusplus
extern "C" {
#endif
void  Amiga_NewList(void *list);
void  Amiga_Remove(void *n);
void *Amiga_RemHead(void *l);
void  Amiga_AddTail(void *l, void *n);
void  Amiga_AddHead(void *l, void *n);
void *Amiga_RemTail(void *l);
#ifdef __cplusplus
}
#endif

#undef NEWLIST
#define NEWLIST(_l) Amiga_NewList((void *)(_l))

#undef REMOVE
#define REMOVE(n) Amiga_Remove((void *)n)

#undef REMHEAD
#define REMHEAD(l) ((APTR)Amiga_RemHead((void *)l))

#undef ADDTAIL
#define ADDTAIL(l,n) Amiga_AddTail((void *)l,(void *)n)

#undef ADDHEAD
#define ADDHEAD(l,n) Amiga_AddHead((void *)l,(void *)n)

#undef REMTAIL
#define REMTAIL(l) ((APTR)Amiga_RemTail((void *)l))

/* MUIB_MUI: this NDK's mui.h doesn't expose it as a symbol, but its own
 * already-defined attributes (MUIA_List_Active = 0x8042391c, etc.) show the
 * base is 0x80420000 -- confirmed by cross-referencing those raw hex
 * literals against the formula used below. Not an AROS-specific value. */
#ifndef MUIB_MUI
#define MUIB_MUI 0x80420000UL
#endif

/* RPTAG_PenMode/FgColor/BgColor: MorphOS true-color RastPort tags (direct
 * ARGB fill/text color, bypassing indexed pen allocation) with no classic
 * CyberGraphX equivalent -- confirmed absent from graphics/rpattr.h and
 * cybergraphics.h in this NDK. SetRPAttrs() itself is real graphics.library
 * API (not AROS-specific, only #undef'd for __AROS__ a few lines up in
 * owbbrowserclass.cpp) and ignores unrecognized tags per standard AmigaOS
 * tag-list semantics, so declaring these as values that cannot collide with
 * any real RPTAG_* (highest defined is RPTAG_DrawBounds = 0x80000008) gives
 * graceful degradation -- the few call sites drawing a drag-thumbnail
 * overlay fall back to the RastPort's default pens instead of the intended
 * translucent ARGB fill. Purely cosmetic, not a crash/corruption risk. */
#ifndef RPTAG_PenMode
#define RPTAG_PenMode 0x81000001UL
#endif
#ifndef RPTAG_FgColor
#define RPTAG_FgColor 0x81000002UL
#endif
#ifndef RPTAG_BgColor
#define RPTAG_BgColor 0x81000003UL
#endif

#define WA_PointerType                              (WA_Dummy + 164)
#define POINTERTYPE_NORMAL                          0
#define POINTERTYPE_MOVE                            14
#define SA_StopBlanker                              (SA_Dummy + 121)


#define MUIM_Menustrip_Popup                        (MUIB_MUI|0x00420e76) /* MUI: V20 */
#define MUIM_Menustrip_ExitChange                   (MUIB_MUI|0x0042ce4d) /* MUI: V20 */
#define MUIM_Menustrip_InitChange                   (MUIB_MUI|0x0042dcd9) /* MUI: V20 */
#define MUIA_List_DoubleClick                       (MUIB_MUI|0x00424635) /* MUI: V4  i.g BOOL */
#define MUIA_List_DragType                          (MUIB_MUI|0x00425cd3) /* MUI: V11 isg LONG */
#define MUIA_Text_Shorten                           (MUIB_MUI|0x00428bbd) /* MUI: V20 isg LONG */
#define MUIA_FrameDynamic                           (MUIB_MUI|0x004223c9) /* MUI: V20 isg BOOL */
#define MUIA_FrameVisible                           (MUIB_MUI|0x00426498) /* MUI: V20 isg BOOL */

#define MUIV_Text_Shorten_Cutoff                    1
#define MUIV_PushMethod_Delay(millis)               MIN(0x0ffffff0, (((ULONG)millis) << 8))

#define MCC_TI_TAGBASE                              ((TAG_USER)|((1307<<16)+0x712))
#define MCC_TI_ID(x)                                (MCC_TI_TAGBASE+(x))
#define MUIA_Textinput_CursorPos                    MCC_TI_ID(127)        /* V1  .sg ULONG */
#define MUIA_Textinput_MarkStart                    MCC_TI_ID(133)        /* V13 isg ULONG */
#define MUIA_Textinput_MarkEnd                      MCC_TI_ID(134)        /* V13 isg ULONG */
#define MUIA_Textinput_ResetMarkOnCursor            MCC_TI_ID(157)        /* V29 isg BOOL */

#define MUIM_Textinput_DoMarkAll                    MCC_TI_ID(11)         /* V1 */

#define NM_WHEEL_UP                                 0x7a
#define NM_WHEEL_DOWN                               0x7b
#undef XMLCALL
#define XMLCALL

#if defined(__AMIGAOS3__) && !defined(AROS_BIG_ENDIAN)
/* m68k-amigaos3 is always big-endian; AROS_BIG_ENDIAN is never defined on
 * this target (it's an AROS-only macro), which silently picked the wrong
 * (little-endian) RECTFMT below -- see SESSION_HANDOFF.md / WebCore_status.md. */
#define AROS_BIG_ENDIAN 1
#endif

#if (AROS_BIG_ENDIAN == 1)
#define NATIVE_ARGB RECTFMT_ARGB32   /* Big Endian Archs. */
#else
#define NATIVE_ARGB RECTFMT_BGRA32   /* Little Endian Archs. */
#endif

// redefines (to work around missing functionality, commented is actual value )

//#define MUIV_Frame_Page 20
#define MUIV_Frame_Page MUIV_Frame_Group

#endif


/*
MUIM_Application_KillPushMethod         NOT USED
MUIM_Group_GetPageChild                 NOT USED
MUIA_Title_Closable                     NOT USED
MUIA_Title_Dragable                     NOT USED
MUIM_Text                               NOT USED
MUIM_TextDim                            NOT USED
MUIA_Prop_Delta                         NOT USED
MUIA_Dtpic_Fade                         NOT USED
MUIA_Dtpic_MinHeight                    NOT USED
MUIA_Dtpic_MinWidth                     NOT USED

MUIF_PUSHMETHOD_SINGLE                  NOT IMPLEMENTED
MUII_Close                              NOT IMPLEMENTED
MUIM_Title_Close                        NOT IMPLEMENTED
MUIA_Title_Newable                      NOT IMPLEMENTED
MUIA_Title_Sortable                     NOT IMPLEMENTED
MUIM_Title_CreateNewButton              NOT IMPLEMENTED
MUIM_Title_New                          NOT IMPLEMENTED
MUIA_CustomBackfill                     NOT IMPLEMENTED
MUIM_FindObject                         NOT IMPLEMENTED
MUIA_Dtpic_Alpha                        NOT IMPLEMENTED
MUIA_Dtpic_DarkenSelState               NOT IMPLEMENTED
MUIA_Dtpic_LightenOnMouse               NOT IMPLEMENTED
MUIM_GoActive                           NOT IMPLEMENTED
MUIM_GoInactive                         NOT IMPLEMENTED
MUIA_List_TopPixel                      NOT IMPLEMENTED


MUIA_Group_ChildCount                   IMPLEMENTED
MUIF_DRAGIMAGE_SOURCEALPHA              IMPLEMENTED
MUIM_CreateDragImage                    IMPLEMENTED
MUIM_DeleteDragImage                    IMPLEMENTED
MUIM_Family_GetChild                    IMPLEMENTED
MUIM_Group_Insert                       IMPLEMENTED
MUIC_Title                              IMPLEMENTED (+/-)

*/
