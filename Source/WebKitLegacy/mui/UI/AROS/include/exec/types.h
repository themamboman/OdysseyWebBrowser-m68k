/*
 * AROS-compat shim for real AmigaOS3 builds.
 *
 * On real AROS, <exec/types.h> defines STACKED/QUAD/UQUAD directly and pulls in
 * <aros/pp.h> (which provides AROS_PP_VARIADIC_CAST2IPTR, used by
 * WebKitLegacy/mui/UI/methodstack.h). None of that exists in the real AmigaOS3
 * NDK's own <exec/types.h>, which this file (found first on the include path --
 * see WebCore_INCLUDE_DIRECTORIES) shadows. #include_next hands off to the real
 * NDK header for everything else, then we add just the missing pieces.
 *
 * Definitions below match AROS's real ones (compiler/include/aros/cpu.h,
 * compiler/include/exec/types.h, compiler/include/aros/preprocessor/variadic/
 * cast2iptr.hpp in https://github.com/deadwood2/AROS), reimplemented without
 * pulling in AROS's own Boost.Preprocessor-based machinery.
 */
#include_next <exec/types.h>

#ifndef STACKED
#define STACKED __attribute__((aligned(4)))
#endif

#ifndef __typedef_QUAD
#define __typedef_QUAD
typedef signed long long QUAD;
typedef unsigned long long UQUAD;
#endif

/* AROS_PP_VARIADIC_CAST2IPTR(...): cast every variadic argument to (IPTR), as a
 * comma-separated list, for methodstack_push(_sync)'s `IPTR __args[] = { ... }`.
 * Standard count-then-dispatch preprocessor trick; sized generously (16 args)
 * since nothing else here needs to know the real argument count up front. */
#ifndef AROS_PP_VARIADIC_CAST2IPTR
#define WK_CAST2IPTR_1(a1) ((IPTR)(a1))
#define WK_CAST2IPTR_2(a1,a2) ((IPTR)(a1)),((IPTR)(a2))
#define WK_CAST2IPTR_3(a1,a2,a3) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3))
#define WK_CAST2IPTR_4(a1,a2,a3,a4) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3)),((IPTR)(a4))
#define WK_CAST2IPTR_5(a1,a2,a3,a4,a5) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3)),((IPTR)(a4)),((IPTR)(a5))
#define WK_CAST2IPTR_6(a1,a2,a3,a4,a5,a6) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3)),((IPTR)(a4)),((IPTR)(a5)),((IPTR)(a6))
#define WK_CAST2IPTR_7(a1,a2,a3,a4,a5,a6,a7) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3)),((IPTR)(a4)),((IPTR)(a5)),((IPTR)(a6)),((IPTR)(a7))
#define WK_CAST2IPTR_8(a1,a2,a3,a4,a5,a6,a7,a8) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3)),((IPTR)(a4)),((IPTR)(a5)),((IPTR)(a6)),((IPTR)(a7)),((IPTR)(a8))
#define WK_CAST2IPTR_9(a1,a2,a3,a4,a5,a6,a7,a8,a9) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3)),((IPTR)(a4)),((IPTR)(a5)),((IPTR)(a6)),((IPTR)(a7)),((IPTR)(a8)),((IPTR)(a9))
#define WK_CAST2IPTR_10(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3)),((IPTR)(a4)),((IPTR)(a5)),((IPTR)(a6)),((IPTR)(a7)),((IPTR)(a8)),((IPTR)(a9)),((IPTR)(a10))
#define WK_CAST2IPTR_11(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3)),((IPTR)(a4)),((IPTR)(a5)),((IPTR)(a6)),((IPTR)(a7)),((IPTR)(a8)),((IPTR)(a9)),((IPTR)(a10)),((IPTR)(a11))
#define WK_CAST2IPTR_12(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3)),((IPTR)(a4)),((IPTR)(a5)),((IPTR)(a6)),((IPTR)(a7)),((IPTR)(a8)),((IPTR)(a9)),((IPTR)(a10)),((IPTR)(a11)),((IPTR)(a12))
#define WK_CAST2IPTR_13(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3)),((IPTR)(a4)),((IPTR)(a5)),((IPTR)(a6)),((IPTR)(a7)),((IPTR)(a8)),((IPTR)(a9)),((IPTR)(a10)),((IPTR)(a11)),((IPTR)(a12)),((IPTR)(a13))
#define WK_CAST2IPTR_14(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3)),((IPTR)(a4)),((IPTR)(a5)),((IPTR)(a6)),((IPTR)(a7)),((IPTR)(a8)),((IPTR)(a9)),((IPTR)(a10)),((IPTR)(a11)),((IPTR)(a12)),((IPTR)(a13)),((IPTR)(a14))
#define WK_CAST2IPTR_15(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3)),((IPTR)(a4)),((IPTR)(a5)),((IPTR)(a6)),((IPTR)(a7)),((IPTR)(a8)),((IPTR)(a9)),((IPTR)(a10)),((IPTR)(a11)),((IPTR)(a12)),((IPTR)(a13)),((IPTR)(a14)),((IPTR)(a15))
#define WK_CAST2IPTR_16(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16) ((IPTR)(a1)),((IPTR)(a2)),((IPTR)(a3)),((IPTR)(a4)),((IPTR)(a5)),((IPTR)(a6)),((IPTR)(a7)),((IPTR)(a8)),((IPTR)(a9)),((IPTR)(a10)),((IPTR)(a11)),((IPTR)(a12)),((IPTR)(a13)),((IPTR)(a14)),((IPTR)(a15)),((IPTR)(a16))

#define WK_NARG_SEQ(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,N,...) N
#define WK_NARG(...) WK_NARG_SEQ(__VA_ARGS__,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1)

#define WK_CAST2IPTR_DISPATCH_(N) WK_CAST2IPTR_##N
#define WK_CAST2IPTR_DISPATCH(N) WK_CAST2IPTR_DISPATCH_(N)
#define AROS_PP_VARIADIC_CAST2IPTR(...) WK_CAST2IPTR_DISPATCH(WK_NARG(__VA_ARGS__))(__VA_ARGS__)
#endif /* !AROS_PP_VARIADIC_CAST2IPTR */

/* stccpy(): classic Amiga clib "bounded string copy, always NUL-terminated"
 * helper (SAS/C runtime), used throughout WebCore/WebKitLegacy but not present
 * in this real-NDK/libc combination. This header is transitively included by
 * every call site (directly or via <proto/*.h>), so it's a convenient common
 * place for the one shared implementation. */
#ifndef STCCPY_COMPAT
#define STCCPY_COMPAT
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
static __inline char *stccpy(char *to, const char *from, size_t len)
{
    char *d = to;
    if (len) {
        while (--len && *from)
            *d++ = *from++;
        *d = '\0';
    }
    return to;
}
#ifdef __cplusplus
}
#endif
#endif /* !STCCPY_COMPAT */
