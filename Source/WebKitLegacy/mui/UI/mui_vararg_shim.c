/* NO_INLINE_STDARG (see PlatformMUI.cmake) disables inline/muimaster.h's
 * __inline/macro wrappers for MUI_NewObject/MUI_MakeObject/
 * MUI_AllocAslRequestTags/MUI_AslRequestTags, because their varargs
 * handling ("ULONG _tags[] = {tags}" aggregate-initializer syntax) requires
 * implicit pointer->ULONG narrowing that's invalid in C++. Disabling them
 * left clib/muimaster_protos.h's plain declarations with no linkable
 * definition anywhere -- confirmed via a real OWB link attempt (nm showed
 * these going from weakly-defined to flatly undefined the moment
 * NO_INLINE_STDARG was defined). This file provides real, linkable
 * definitions, compiled as C (where the narrowing the NDK macros rely on
 * is legal) instead of C++.
 *
 * MUI_NewObject / MUI_AllocAslRequestTags / MUI_AslRequestTags: real
 * Tag/Value pair lists terminated by TAG_DONE or TAG_MORE, same shape as
 * DoSuperNew's varargs (see aros_stuff.c) -- walked with va_arg into a
 * local TagItem array using the identical, already-verified technique.
 *
 * MUI_MakeObject: genuinely different shape. Its *A entry point takes a
 * flat ULONG* parameter block whose length depends on which MUIO_* `type`
 * is being built (checked against every call site in this tree: 1 to 3
 * IPTR-sized args, no TAG_DONE-style terminator at all -- there is no
 * generic way to know how many args were really passed). Pulling a fixed
 * 4 va_arg slots (one more than the largest real call site needs) is not
 * airtight C (reading past the actual argument count is UB), but is safe
 * in practice on this target: arguments are passed linearly on the stack,
 * so the one extra slot just reads whatever stack content follows (never
 * dereferenced, only written into a scratch array MUI itself only reads
 * exactly `type`'s own known number of slots from). Documented rather than
 * hidden, consistent with this project's other honest-limitation notes.
 */

#include <exec/types.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>
#include <utility/tagitem.h>
#include <stdarg.h>

/* MUI_NewObjectA/MUI_MakeObjectA/MUI_AllocAslRequest/MUI_AslRequest are NOT
 * real linkable functions on this toolchain -- they're inline/muimaster.h
 * macros that expand to a direct LVO call at the use site (confirmed: a
 * plain `extern` declaration of them, as this file originally had, links
 * clean but leaves an undefined reference, since no archive anywhere
 * actually defines them as real symbols). Including the real header lets
 * the compiler generate the correct inline call instead. These four are
 * NOT gated by NO_INLINE_STDARG (only the variadic MUI_NewObject/
 * MUI_MakeObject/MUI_AllocAslRequestTags/MUI_AslRequestTags wrappers this
 * file exists to replace are), so they're unconditionally available. */
#include <proto/muimaster.h>

static int walk_tags(struct TagItem *out, int max, Tag firstTag, va_list args)
{
    int n = 0;
    Tag tag = firstTag;
    for (;;)
    {
        out[n].ti_Tag = tag;
        if (tag == TAG_DONE || tag == TAG_MORE)
        {
            out[n].ti_Data = (tag == TAG_MORE) ? va_arg(args, ULONG) : 0;
            n++;
            break;
        }
        out[n].ti_Data = va_arg(args, ULONG);
        n++;
        if (n >= max - 1)
        {
            out[n].ti_Tag = TAG_DONE;
            out[n].ti_Data = 0;
            n++;
            break;
        }
        tag = va_arg(args, Tag);
    }
    return n;
}

Object *MUI_NewObject(CONST_STRPTR classname, Tag tag1, ...)
{
    struct TagItem tags[128];
    va_list args;
    va_start(args, tag1);
    walk_tags(tags, 128, tag1, args);
    va_end(args);
    return MUI_NewObjectA(classname, tags);
}

APTR MUI_AllocAslRequestTags(ULONG reqType, Tag tag1, ...)
{
    struct TagItem tags[64];
    va_list args;
    va_start(args, tag1);
    walk_tags(tags, 64, tag1, args);
    va_end(args);
    return MUI_AllocAslRequest(reqType, tags);
}

BOOL MUI_AslRequestTags(APTR requester, Tag tag1, ...)
{
    struct TagItem tags[64];
    va_list args;
    va_start(args, tag1);
    walk_tags(tags, 64, tag1, args);
    va_end(args);
    return MUI_AslRequest(requester, tags);
}

Object *MUI_MakeObject(LONG type, ...)
{
    ULONG params[4] = {0, 0, 0, 0};
    va_list args;
    va_start(args, type);
    params[0] = va_arg(args, ULONG);
    params[1] = va_arg(args, ULONG);
    params[2] = va_arg(args, ULONG);
    params[3] = va_arg(args, ULONG);
    va_end(args);
    return MUI_MakeObjectA(type, params);
}

/* NewObjectA/SetAttrsA are themselves inline/intuition.h macros (the real
 * array-based LVO calls), and NewObject/SetAttrs (no suffix) are ALSO
 * unconditional macros there -- fully self-contained static __stdargs
 * inline functions using the stack-address varargs trick, not gated by
 * NO_INLINE_STDARG at all. Defining real NewObjectA/SetAttrsA functions
 * here (an earlier attempt, now reverted) collides with the NewObjectA/
 * SetAttrsA macros themselves ("expected identifier or '(' before '{'
 * token" -- the macro eats the function name before the compiler ever
 * sees a declaration). The undefined NewObject/SetAttrs/IntuitionBase/
 * UtilityBase references in the bookmark*.cpp files are just missing
 * #include <proto/intuition.h>/<proto/utility.h> in those files -- see
 * WebCore_status.md / OWB_BUILD_HANDOFF.md. No shim needed here.*/
