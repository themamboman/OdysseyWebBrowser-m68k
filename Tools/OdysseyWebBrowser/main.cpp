/*
 * Copyright (C) 2009 Fabien Coeurjoly
 * Copyright (C) 2008 Pleyo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <cairo.h>

#include <proto/timer.h>
#include <dos/dos.h>
#include <dos/notify.h>
#include <proto/dos.h>

#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/icon.h>
#include <proto/graphics.h>
#include <proto/locale.h>
#include <proto/rexxsyslib.h>
#include <proto/wb.h>
#include <proto/mathieeedoubbas.h>
#include <proto/mathieeesingbas.h>
#include <proto/mathieeedoubtrans.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if !defined(__AROS__)
/* Same fix as WebKitLegacy/mui/UI/splashwindowclass.cpp: only plain
 * signal(SIGINT, SIG_IGN) below is used, and <sys/signal.h> (the richer
 * BSD sigaction/sigset_t header, no clib2 equivalent) conflicts with
 * clib2's own simpler signal.h -- see WebCore_status.md. */
#include <signal.h>
#endif
#include <locale.h>
#include <setjmp.h>

#include <clib/debug_protos.h>

#include "rtg_lib.h"

#include "gui.h"
#include "alocale.h"

#undef accept

#define D(x)

void close_dictionary();
void destroy_application(void);
void close_libs(void);

extern "C"
{
/* This debug override of raise() (dumps a stack frame via kprintf, a
 * MorphOS-only debug call) conflicts with clib2's real raise() at link
 * time on classic AmigaOS3 ("multiple definition of raise" -- see
 * WebCore_status.md). clib2's own implementation is the correct,
 * POSIX-conformant one WebCore/JSC actually need; skip this debug
 * shadow there instead of trying to keep both. */
#if !defined(__AMIGAOS3__)
	int raise(int sig)
	{
		kprintf("raise(%d) was called, dumping stackframe...\n", sig);

#if !defined(__AROS__)
		DumpTaskState(FindTask(NULL));
#endif

		return(0);
	}
#endif /* !__AMIGAOS3__ */
	/*
	void abort(void)
	{
		kprintf("abort was called, dumping stackframe...\n");

        DumpTaskState(FindTask(NULL));

		Wait(0);
	} */
}

/* Globals */
unsigned long __stack = 2*1024*1024;
jmp_buf bailout_env;

struct Library * IconBase = NULL;
struct Library * MUIMasterBase = NULL;
struct Library * KeymapBase = NULL;
struct Library * CharsetsBase = NULL;
struct Library * SpellCheckerBase = NULL;
/* IntuitionBase/UtilityBase/IFFParseBase: this codebase uses SetAttrs()/
 * NewObject()/IFF clipboard calls throughout WebCore and WebKitLegacy as
 * if these were already open -- true on MorphOS/AROS, which auto-open
 * their own base libraries, but not on classic AmigaOS3, where every
 * application must OpenLibrary() these itself. Confirmed via a real OWB
 * link: dozens of files across WebCore/WebKitLegacy undefined-referencing
 * these three globals, with no OpenLibrary call for any of them anywhere
 * in the project -- see WebCore_status.md. */
struct IntuitionBase * IntuitionBase = NULL;
struct Library * UtilityBase = NULL;
struct Library * IFFParseBase = NULL;
/* Same MorphOS/AROS-auto-open gap as above: graphics.library (used
 * directly in owbbrowserclass.cpp's drawing code), locale.library (used
 * by locale.cpp's catalog-based localization -- see WebCore_status.md's
 * answer on non-English language support), rexxsyslib.library (ARexx
 * scripting in utils.cpp) and workbench.library (Workbench drag&drop in
 * downloadlistclass.cpp) were all referenced as if already open. */
struct GfxBase * GfxBase = NULL;
/* clib2's own libm.a (software/IEEE floating point helpers -- comparison,
 * conversion, and transcendental routines used throughout WTF/JSC's
 * floating-point code) calls into these three classic IEEE math
 * libraries, never opened anywhere in this project either -- same
 * MorphOS/AROS-auto-open gap as the others above, just not discovered
 * until these specific libm.a objects started linking in correctly (see
 * WebCore_status.md). */
/* Defined, opened (before the C++ static constructors) and closed (after the
 * static destructors) by clib2's own math_init_exit.o in libm.a. */
extern struct MathIEEEBase * MathIeeeDoubBasBase;
extern struct MathIEEEBase * MathIeeeSingBasBase;
extern struct MathIEEEBase * MathIeeeDoubTransBase;
struct LocaleBase * LocaleBase = NULL;
struct RxsLib * RexxSysBase = NULL;
struct Library * WorkbenchBase = NULL;
struct Library * ExpatBase = NULL;

static ULONG __morphos2 = FALSE;

/* Dos notify stuff */

ULONG dosnotifysig=0;
LONG dosnotifybit=-1;
struct NotifyRequest nr;

LONG dosnotify_init(void)
{
	dosnotifybit = AllocSignal(-1);
	if(dosnotifybit!=-1)
	{
		dosnotifysig = 1L << dosnotifybit;
		return (TRUE);
	}
	return (FALSE);
}

void dosnotify_cleanup(void)
{
	if(dosnotifybit)
	{
		FreeSignal(dosnotifybit);
	}
}

struct NotifyRequest *dosnotify_start(CONST_STRPTR name)
{
	struct NotifyRequest *nr;
	ULONG namelen;

	namelen = strlen(name);
	if((namelen) && (dosnotifysig))
	{
		if( (nr = (struct NotifyRequest *)malloc(sizeof(*nr) + namelen + 1)) )
		{
			nr->nr_Name= (char *) (nr + 1);
			memcpy(nr->nr_Name, name, namelen + 1);
			nr->nr_stuff.nr_Signal.nr_Task   = FindTask(NULL);
			nr->nr_stuff.nr_Signal.nr_SignalNum = dosnotifysig;
			nr->nr_Flags           = NRF_SEND_SIGNAL;
			if(StartNotify(nr))
			{
				return (nr);
			}
			free(nr);
		}
	}
	return (NULL);
}


void dosnotify_stop(struct NotifyRequest *nr)
{
	if(nr)
	{
		EndNotify(nr);
		free(nr);
	}
}

ULONG is_morphos2(void)
{
	return __morphos2;
}

ULONG open_libs(void)
{
	if(!(KeymapBase = OpenLibrary("keymap.library", 39)))
	{
		fprintf(stderr, "Failed to open keymap.library.\n");
		return FALSE;
	}

	if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
	{
		fprintf(stderr, "Failed to open " MUIMASTER_NAME ".\n");
		return FALSE;
	}

	if(!(IconBase = OpenLibrary("icon.library", 39)))
	{
		fprintf(stderr, "Failed to open icon.library.\n");
		return FALSE;
	}

	if(!(IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 36)))
	{
		fprintf(stderr, "Failed to open intuition.library.\n");
		return FALSE;
	}

	if(!(UtilityBase = OpenLibrary("utility.library", 36)))
	{
		fprintf(stderr, "Failed to open utility.library.\n");
		return FALSE;
	}

	/* iffparse.library is only used for IFF clipboard data (PasteboardMorphOS.cpp/
	 * clipboard.cpp) -- not critical enough to hard-fail startup over. */
	IFFParseBase = OpenLibrary("iffparse.library", 36);

	if(!(GfxBase = (struct GfxBase *) OpenLibrary("graphics.library", 36)))
	{
		fprintf(stderr, "Failed to open graphics.library.\n");
		return FALSE;
	}

	if(!MathIeeeDoubBasBase) /* opened by clib2 math_init_exit.o before static constructors */
	{
		fprintf(stderr, "Failed to open mathieeedoubbas.library.\n");
		return FALSE;
	}

	if(!MathIeeeSingBasBase) /* opened by clib2 math_init_exit.o before static constructors */
	{
		fprintf(stderr, "Failed to open mathieeesingbas.library.\n");
		return FALSE;
	}

	if(!MathIeeeDoubTransBase) /* opened by clib2 math_init_exit.o before static constructors */
	{
		fprintf(stderr, "Failed to open mathieeedoubtrans.library.\n");
		return FALSE;
	}

	/* locale.library/rexxsyslib.library/workbench.library are all used for
	 * optional features (catalog-based localization, ARexx scripting,
	 * Workbench drag&drop) -- not critical enough to hard-fail startup. */
	LocaleBase = (struct LocaleBase *) OpenLibrary("locale.library", 38);
	RexxSysBase = (struct RxsLib *) OpenLibrary("rexxsyslib.library", 0);
	WorkbenchBase = OpenLibrary("workbench.library", 36);

	/* Real expat 2.7.4 is statically linked (-lexpat) on this target --
	 * suggestpopstringclass.cpp calls XML_Parse()/etc. directly, never
	 * through ExpatBase's LVO table (confirmed: ExpatBase isn't referenced
	 * anywhere outside this file). expat.library itself isn't a real
	 * AmigaOS3 system library, so requiring it to open (as this used to,
	 * hard-failing the whole app on refusal) would have broken startup
	 * outright. Kept as a soft, non-fatal open only so close_libs() has
	 * something consistent to do; failure here doesn't matter. */
	ExpatBase = OpenLibrary("expat.library", 0);

	struct Library * ibase = (struct Library *) IntuitionBase;
	if(ibase)
	{
		if(ibase->lib_Version > 50 || (ibase->lib_Version == 50 && ibase->lib_Revision >= 61))
		{
			if(KeymapBase->lib_Version > 50) // Henes 1.5 check
			{
				__morphos2 = TRUE;
			}
		}
	}

	extern void init_useragent();
	init_useragent();

	// Use charsets.library if available
	CharsetsBase = OpenLibrary("charsets.library", 51);

	// Use spellchecker if available
	SpellCheckerBase = OpenLibrary("spellchecker.library", 0);

	// BsdSocket check. Making it later is too risky.
	struct Library *mainSocketBase = OpenLibrary("bsdsocket.library", 4);

	if(!mainSocketBase)
	{
		fprintf(stderr, "Failed to open bsdsocket.library. OWB requires a running TCP/IP stack.\n");
		return FALSE;
	}
	else
	{
		CloseLibrary(mainSocketBase);
		mainSocketBase = NULL;
	}

	// Hack to make sure cairo mutex are initialized
	{
		cairo_surface_t *dummysurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 4, 4);
		cairo_surface_destroy(dummysurface);
	}

	// RTG (CyberGraphX): opens cybergraphics.library and defines CyberGfxBase.
	// Nothing in this tree called this anywhere before -- proto/cybergraphics.h's
	// extern CyberGfxBase would otherwise stay NULL and the first blit
	// (owbbrowserclass.cpp's WritePixelArray path) would fault. See
	// build/phase2-agentd-rtg/rtg_lib.h. NOTE: owbbrowserclass.cpp's
	// WritePixelArray call sites do not themselves check CyberGfxBase for
	// NULL, so if CyberGfx_Init() fails here (no RTG board -- unlikely under
	// WinUAE/emulation, which normally provides one, but not impossible on
	// plain AGA/ECS), the first paint will crash rather than degrade
	// gracefully. Treated as acceptable for now since the target environment
	// is expected to have RTG; flagging rather than silently assuming it.
	CyberGfx_Init();

	return TRUE;
}

void close_libs(void)
{
	if(MUIMasterBase)
	{
		CloseLibrary(MUIMasterBase);
		MUIMasterBase = NULL;
	}

	if(ExpatBase)
	{
		CloseLibrary(ExpatBase);
		ExpatBase = NULL;
	}

	if(IconBase)
	{
		CloseLibrary(IconBase);
		IconBase = NULL;
	}

	if(IFFParseBase)
	{
		CloseLibrary(IFFParseBase);
		IFFParseBase = NULL;
	}

	if(WorkbenchBase)
	{
		CloseLibrary(WorkbenchBase);
		WorkbenchBase = NULL;
	}

	if(RexxSysBase)
	{
		CloseLibrary((struct Library *) RexxSysBase);
		RexxSysBase = NULL;
	}

	if(LocaleBase)
	{
		CloseLibrary((struct Library *) LocaleBase);
		LocaleBase = NULL;
	}

	if(GfxBase)
	{
		CloseLibrary((struct Library *) GfxBase);
		GfxBase = NULL;
	}

	if(UtilityBase)
	{
		CloseLibrary(UtilityBase);
		UtilityBase = NULL;
	}

	if(IntuitionBase)
	{
		CloseLibrary((struct Library *) IntuitionBase);
		IntuitionBase = NULL;
	}

	if(KeymapBase)
	{
		CloseLibrary(KeymapBase);
		KeymapBase = NULL;
	}

	if(CharsetsBase)
	{
		CloseLibrary(CharsetsBase);
		CharsetsBase = NULL;
	}

	if(SpellCheckerBase)
	{
		close_dictionary();
		CloseLibrary(SpellCheckerBase);
		SpellCheckerBase = NULL;
	}

	CyberGfx_Cleanup();
}

void destroy_application(void)
{
	if(app)
	{
		MUI_DisposeObject(app);
		methodstack_cleanup();
		classes_cleanup();
		app = NULL;
	}

	locale_cleanup();
}

// Safety checks
extern ULONG icu_check(void);

ULONG safety_check(void)
{
	ULONG passed = TRUE;

	passed &= icu_check();

	return passed;
}

Object *create_application(char *url)
{
	Object *obj = NULL;

	locale_init();

	if(classes_init())
	{
	    methodstack_init();
		obj = (Object *) NewObject(getowbappclass(), NULL, MA_OWBBrowser_URL, (IPTR) url, TAG_DONE);
	}
	else
	{
		fprintf(stderr, "Failed to create internal classes.\n");
	}

	return obj;
}

extern "C" {
#if defined(__AROS__)
void aros_register_trap_handler();
#endif
int aros_timer_open();
void aros_timer_close();
}

void main_loop(void)
{
	ULONG running = TRUE;
	ULONG signals;

	setIsSafeToQuit(TRUE);
	
	if(setjmp(bailout_env) == -1)
	{
	    running = FALSE;
	}

#if defined(__AROS__)
    if (running)
        aros_register_trap_handler();
#endif

	while (running)
	{
		ULONG ret = DoMethod(app,MUIM_Application_NewInput,&signals);

		switch(ret)
		{
			case (ULONG) MUIV_Application_ReturnID_Quit:
			{
				setIsQuitting(TRUE);
				if(isSafeToQuit())
					running = FALSE;
				break;
			}
		}

		methodstack_check();

		/* Refresh each active browser if needed */
		DoMethod(app, MM_OWBApp_Expose);

		if(running && signals) signals = Wait(signals | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F/* | dosnotifysig */);

		if((signals & SIGBREAKF_CTRL_C) || isQuitting())
		{
			setIsQuitting(TRUE);
			if(isSafeToQuit())
				running = FALSE;
		}

		if(signals & SIGBREAKF_CTRL_E)
		{
			/* Run webkit events for each active browser */
			DoMethod(app, MM_OWBApp_WebKitEvents);
		}
		
		/*
		if(signals & dosnotifysig )
		{
			//kprintf("DosNotify Trig\n");
			//DoMethod(getv(app, MA_OWBApp_BookmarkGroup), MM_Bookmarkgroup_DosNotify);
		}
		*/

		methodstack_check();
	}
}

/* appclass.cpp declares this `extern` (used for GetDiskObject() to fetch
 * the app's own Workbench icon) but nothing anywhere defines it -- some
 * Amiga C runtimes (SAS/C) provide it automatically from crt0, this one
 * doesn't. argv[0] is exactly the right value: the program's own path. */
char *_ProgramName = 0;

/* PROGDIR: is set automatically by Workbench when launching via icon
 * double-click, but *not* by a CLI/Shell launch -- nothing in this codebase
 * (or in clib2's startup) ever established it for that case, so every CLI
 * run required a manual `assign PROGDIR: <dir>` first. Derive it from
 * argv[0] (the invoked path) instead, same source already used for
 * _ProgramName above. Leaves any existing assign alone. */
static void ensure_progdir_assign(const char *programPath)
{
	BPTR existing = Lock((CONST_STRPTR) "PROGDIR:", ACCESS_READ);
	if (existing)
	{
		UnLock(existing);
		return;
	}

	if (!programPath || !programPath[0])
		return;

	char dir[512];
	stccpy(dir, programPath, sizeof(dir));

	/* Keep everything up to (and, for ':', including) the last path
	 * separator, dropping the program's own filename. */
	char *lastColon = strrchr(dir, ':');
	char *lastSlash = strrchr(dir, '/');
	char *cut = NULL;

	if (lastSlash && (!lastColon || lastSlash > lastColon))
		cut = lastSlash;
	else if (lastColon)
		cut = lastColon + 1;

	if (cut)
		*cut = '\0';
	else
		dir[0] = '\0'; /* bare filename, no path info -- fall back to cwd */

	BPTR lock = Lock((CONST_STRPTR) dir, ACCESS_READ);
	if (!lock)
		return;

	if (!AssignLock((STRPTR) "PROGDIR", lock))
		UnLock(lock); /* AssignLock takes ownership of the lock on success */
}

/* libamisslauto registers these through a.out "set" symbols, which made the
 * linker replace the whole C++ static-constructor list with AmiSSL's single
 * entry. The installed library has those set symbols renamed, so they are
 * called explicitly instead. See OWB_RUNTIME_DEBUG_HANDOFF.md. */
extern "C" void __init_amissl(void);
extern "C" void __exit_amissl(void);


#if defined(__AMIGAOS3__)
#include <unicode/putil.h>
#endif

/* Use clib2's slab allocator. Left at 0, every malloc()/free() goes through an Exec memory
 * pool with 4 KB puddles, and those walk a list that grows with the number of puddles: a
 * browser's heap spends its time searching it. Read by clib2's start-up, before main(). */
extern "C" { unsigned long __slab_max_size = 16384; }

/* clib2 implements every soft-float compare (__eqdf2, __nedf2, __ltdf2, ...) as a bare call to
 * mathieeedoubbas/mathieeesingbas IEEEDPCmp/IEEESPCmp, which has no "unordered" result: NaN compares
 * equal to itself and is ordered against numbers. JavaScript then treats NaN as true, so
 * `if (parseInt(undefined,10)) for (; r--;)` in Modernizr never ends and allocates until memory runs out.
 * These replace clib2's members at link time. They follow libgcc's contract and work on the raw bits. */
extern "C" {
static int owbCmpBits(unsigned int ah, unsigned int al, unsigned int bh, unsigned int bl, int unordered)
{
    unsigned int am = ah & 0x7fffffffu, bm = bh & 0x7fffffffu;
    if (am > 0x7ff00000u || (am == 0x7ff00000u && al))
        return unordered;
    if (bm > 0x7ff00000u || (bm == 0x7ff00000u && bl))
        return unordered;
    if (!(am | al | bm | bl))
        return 0; /* +0 == -0 */
    if ((ah ^ bh) & 0x80000000u)
        return (ah & 0x80000000u) ? -1 : 1;
    int r = am != bm ? (am < bm ? -1 : 1) : (al != bl ? (al < bl ? -1 : 1) : 0);
    return (ah & 0x80000000u) ? -r : r;
}
static int owbCmpBitsF(unsigned int a, unsigned int b, int unordered)
{
    unsigned int am = a & 0x7fffffffu, bm = b & 0x7fffffffu;
    if (am > 0x7f800000u || bm > 0x7f800000u)
        return unordered;
    if (!(am | bm))
        return 0; /* +0 == -0 */
    if ((a ^ b) & 0x80000000u)
        return (a & 0x80000000u) ? -1 : 1;
    int r = am != bm ? (am < bm ? -1 : 1) : 0;
    return (a & 0x80000000u) ? -r : r;
}
int __cmpdf2(unsigned int ah, unsigned int al, unsigned int bh, unsigned int bl) { return owbCmpBits(ah, al, bh, bl, 1); }
int __eqdf2(unsigned int ah, unsigned int al, unsigned int bh, unsigned int bl) { return owbCmpBits(ah, al, bh, bl, 1); }
int __nedf2(unsigned int ah, unsigned int al, unsigned int bh, unsigned int bl) { return owbCmpBits(ah, al, bh, bl, 1); }
int __gtdf2(unsigned int ah, unsigned int al, unsigned int bh, unsigned int bl) { return owbCmpBits(ah, al, bh, bl, -1); }
int __gedf2(unsigned int ah, unsigned int al, unsigned int bh, unsigned int bl) { return owbCmpBits(ah, al, bh, bl, -1); }
int __ltdf2(unsigned int ah, unsigned int al, unsigned int bh, unsigned int bl) { return owbCmpBits(ah, al, bh, bl, 1); }
int __ledf2(unsigned int ah, unsigned int al, unsigned int bh, unsigned int bl) { return owbCmpBits(ah, al, bh, bl, 1); }
int __cmpsf2(unsigned int a, unsigned int b) { return owbCmpBitsF(a, b, 1); }
int __eqsf2(unsigned int a, unsigned int b) { return owbCmpBitsF(a, b, 1); }
int __nesf2(unsigned int a, unsigned int b) { return owbCmpBitsF(a, b, 1); }
int __gtsf2(unsigned int a, unsigned int b) { return owbCmpBitsF(a, b, -1); }
int __gesf2(unsigned int a, unsigned int b) { return owbCmpBitsF(a, b, -1); }
int __ltsf2(unsigned int a, unsigned int b) { return owbCmpBitsF(a, b, 1); }
int __lesf2(unsigned int a, unsigned int b) { return owbCmpBitsF(a, b, 1); }
}

/* clib2's __log10 loads MathIeeeDoubBasBase but calls IEEEDPLog10 (-126), which is in
 * mathieeedoubtrans: it jumps past the end of the wrong library's table, so Math.log10() crashes
 * (illegal instruction). Its log10() also returns -Inf for negative input, where NaN is required.
 * These replace clib2's math_log10 member at link time. */
asm(
"	.text\n"
"	.even\n"
"	.globl	___log10\n"
"___log10:\n"
"	move.l	a6,-(sp)\n"
"	move.l	_MathIeeeDoubTransBase,a6\n"
"	movem.l	8(sp),d0/d1\n"
"	jsr	-126(a6)\n"
"	move.l	(sp)+,a6\n"
"	rts\n"
);
extern "C" {
double __log10(double x);
union owbLog10Bits { double d; unsigned int w[2]; };
double log10(double x)
{
    owbLog10Bits u;
    u.d = x;
    unsigned int m = u.w[0] & 0x7fffffffu;
    if (m > 0x7ff00000u || (m == 0x7ff00000u && u.w[1]))
        return x; /* NaN stays NaN */
    if (!(m | u.w[1])) { /* log10(+-0) = -Inf */
        u.w[0] = 0xfff00000u; u.w[1] = 0;
        return u.d;
    }
    if (u.w[0] & 0x80000000u) { /* negative, including -Inf: NaN */
        u.w[0] = 0x7ff80000u; u.w[1] = 0;
        return u.d;
    }
    if (m == 0x7ff00000u)
        return x; /* log10(+Inf) = +Inf */
    return __log10(x);
}
}

/* clib2's maths functions don't follow IEEE/ECMAScript rules outside the normal range, and these are
 * what JavaScript's Math and % operator call. fmod() computes x/y and multiplies the fraction back, so
 * it loses the sign and exactness (-5 % 3 gave 2) and returns x for y=0. sqrt/asin/acos return 0, and log
 * returns -Inf, where NaN is required; pow returns HUGE_VAL (DBL_MAX) for domain errors; atan2 ignores
 * signed zero. These replace those clib2 members at link time. Each asm stub is clib2's own. */
asm(
"	.text\n"
"	.even\n"
"	.globl	___log\n"
"___log:\n"
"	move.l	a6,-(sp)\n"
"	move.l	_MathIeeeDoubTransBase,a6\n"
"	movem.l	8(sp),d0/d1\n"
"	jsr	-84(a6)\n"
"	move.l	(sp)+,a6\n"
"	rts\n"
"	.globl	___sqrt\n"
"___sqrt:\n"
"	move.l	a6,-(sp)\n"
"	move.l	_MathIeeeDoubTransBase,a6\n"
"	movem.l	8(sp),d0/d1\n"
"	jsr	-96(a6)\n"
"	move.l	(sp)+,a6\n"
"	rts\n"
"	.globl	___asin\n"
"___asin:\n"
"	move.l	a6,-(sp)\n"
"	move.l	_MathIeeeDoubTransBase,a6\n"
"	movem.l	8(sp),d0/d1\n"
"	jsr	-114(a6)\n"
"	move.l	(sp)+,a6\n"
"	rts\n"
"	.globl	___acos\n"
"___acos:\n"
"	move.l	a6,-(sp)\n"
"	move.l	_MathIeeeDoubTransBase,a6\n"
"	movem.l	8(sp),d0/d1\n"
"	jsr	-120(a6)\n"
"	move.l	(sp)+,a6\n"
"	rts\n"
"	.globl	___pow\n"
"___pow:\n"
"	movem.l	d2/d3/a6,-(sp)\n"
"	move.l	_MathIeeeDoubTransBase,a6\n"
"	movem.l	16(sp),d0/d1\n"
"	movem.l	24(sp),d2/d3\n"
"	jsr	-90(a6)\n"
"	movem.l	(sp)+,d2/d3/a6\n"
"	rts\n"
);
extern "C" {
double __log(double); double __sqrt(double); double __asin(double); double __acos(double); double __pow(double, double); double atan(double);
union owbD { double d; unsigned long long u; };
static const unsigned long long owbAbsMask = 0x7fffffffffffffffULL, owbInfBits = 0x7ff0000000000000ULL, owbSignBit = 0x8000000000000000ULL;
static double owbFromBits(unsigned long long b) { owbD v; v.u = b; return v.d; }
static unsigned long long owbBits(double d) { owbD v; v.d = d; return v.u; }
#define OWB_NAN owbFromBits(0x7ff8000000000000ULL)
#define OWB_INF owbFromBits(owbInfBits)

/* 0 = not an integer, 1 = odd integer, 2 = even integer (bits of a finite value). */
static int owbIntKind(unsigned long long b)
{
    unsigned long long a = b & owbAbsMask;
    if (!a)
        return 2;
    int e = (int)(a >> 52);
    if (e < 1023)
        return 0;
    if (e >= 1023 + 53)
        return 2;
    unsigned long long m = (a & 0x000fffffffffffffULL) | 0x0010000000000000ULL;
    int shift = 1075 - e;
    if (m & ((1ULL << shift) - 1))
        return 0;
    return (m >> shift) & 1 ? 1 : 2;
}

double log(double x)
{
    unsigned long long b = owbBits(x), a = b & owbAbsMask;
    if (a > owbInfBits) return x;
    if (!a) return -OWB_INF;
    if (b & owbSignBit) return OWB_NAN;
    if (a == owbInfBits) return x;
    return __log(x);
}
double sqrt(double x)
{
    unsigned long long b = owbBits(x), a = b & owbAbsMask;
    if (a > owbInfBits || !a) return x;
    if (b & owbSignBit) return OWB_NAN;
    if (a == owbInfBits) return x;
    return __sqrt(x);
}
double asin(double x)
{
    if ((owbBits(x) & owbAbsMask) > 0x3ff0000000000000ULL) return OWB_NAN;
    return __asin(x);
}
double acos(double x)
{
    if ((owbBits(x) & owbAbsMask) > 0x3ff0000000000000ULL) return OWB_NAN;
    return __acos(x);
}
double pow(double x, double y)
{
    unsigned long long xb = owbBits(x), yb = owbBits(y), xa = xb & owbAbsMask, ya = yb & owbAbsMask;
    static const unsigned long long one = 0x3ff0000000000000ULL;
    if (!ya || xb == one) return 1.0;
    if (xa > owbInfBits || ya > owbInfBits) return OWB_NAN;
    bool yNeg = (yb & owbSignBit) != 0, xNeg = (xb & owbSignBit) != 0;
    if (ya == owbInfBits) {
        if (xa == one) return 1.0;
        return (xa > one) != yNeg ? OWB_INF : 0.0;
    }
    int kind = owbIntKind(yb);
    if (!xa || xa == owbInfBits) {
        bool huge = (xa == owbInfBits) != yNeg;
        double r = huge ? OWB_INF : 0.0;
        return xNeg && kind == 1 ? -r : r;
    }
    if (xNeg && !kind) return OWB_NAN;
    return __pow(x, y);
}
double atan2(double y, double x)
{
    static const double pi = 3.141592653589793, halfPi = 1.5707963267948966, quarterPi = 0.7853981633974483;
    unsigned long long xb = owbBits(x), yb = owbBits(y), xa = xb & owbAbsMask, ya = yb & owbAbsMask;
    if (xa > owbInfBits || ya > owbInfBits) return OWB_NAN;
    bool yNeg = (yb & owbSignBit) != 0, xNeg = (xb & owbSignBit) != 0;
    double r;
    if (!ya) {
        if (!xNeg) return y;
        r = pi;
    } else if (!xa) {
        r = halfPi;
    } else if (xa == owbInfBits) {
        if (ya == owbInfBits) r = xNeg ? 3.0 * quarterPi : quarterPi;
        else r = xNeg ? pi : 0.0;
    } else if (ya == owbInfBits) {
        r = halfPi;
    } else {
        r = atan(y / x);
        if (xNeg) r = yNeg ? r - pi : r + pi;
        return r;
    }
    return yNeg ? -r : r;
}
double fmod(double x, double y)
{
    unsigned long long xb = owbBits(x), yb = owbBits(y), xa = xb & owbAbsMask, ya = yb & owbAbsMask;
    unsigned long long sign = xb & owbSignBit;
    if (!ya || ya > owbInfBits || xa >= owbInfBits) return OWB_NAN;
    if (xa < ya) return x;
    if (xa == ya) return owbFromBits(sign);
    int ex = (int)(xa >> 52), ey = (int)(ya >> 52);
    unsigned long long mx, my;
    if (ex) mx = (xa & 0x000fffffffffffffULL) | 0x0010000000000000ULL;
    else { mx = xa; ex = 1; while (!(mx & 0x0010000000000000ULL)) { mx <<= 1; ex--; } }
    if (ey) my = (ya & 0x000fffffffffffffULL) | 0x0010000000000000ULL;
    else { my = ya; ey = 1; while (!(my & 0x0010000000000000ULL)) { my <<= 1; ey--; } }
    for (; ex > ey; ex--) {
        if (mx >= my) { mx -= my; if (!mx) return owbFromBits(sign); }
        mx <<= 1;
    }
    if (mx >= my) { mx -= my; if (!mx) return owbFromBits(sign); }
    while (!(mx & 0x0010000000000000ULL)) { mx <<= 1; ex--; }
    if (ex > 0) mx = (mx & 0x000fffffffffffffULL) | ((unsigned long long)ex << 52);
    else mx >>= 1 - ex;
    return owbFromBits(mx | sign);
}
}

/* CRASH() is abort() on this port, and clib2 turns that into a normal exit: destructors run,
 * libraries close, DOSBase = NULL and the code unloads while other threads are still alive, which
 * then crash (Guru). abort() calls raise(SIGABRT) first, in the calling task, so park that task
 * there instead of tearing the process down. */
static void owbFatalSignal(int sig)
{
	(void)sig;
	Wait(0); /* no exit, no teardown */
}

int main (int argc, char* argv[])
{
	__init_amissl();
	atexit(__exit_amissl);
	_ProgramName = argv[0];
	ensure_progdir_assign(argv[0]);
#if !defined(__AROS__)
	signal(SIGINT, SIG_IGN);
#endif
	atexit(close_libs);
	atexit(destroy_application);
	/* timer.device must be open on every Amiga target, not just AROS: OWB calls
	   GetSysTime() through TimerBase (e.g. the console window timestamps). */
	atexit(aros_timer_close);
	signal(SIGABRT, owbFatalSignal);
	signal(SIGSEGV, owbFatalSignal);
	signal(SIGILL, owbFatalSignal);
	signal(SIGFPE, owbFatalSignal);
	signal(SIGTERM, owbFatalSignal);

	setlocale(LC_TIME, "C");
	setlocale(LC_NUMERIC, "C");

#if defined(__AMIGAOS3__)
	/* ICU data (break iterators, converters, locales) ships as individual files
	   in PROGDIR:Resource/icudt55b/. The library itself carries no data, and its
	   built-in default directory is the cross-compiler's install prefix. Without
	   data, ubrk_open() fails and text editing in web forms crashes. */
	u_setDataDirectory("PROGDIR:Resource");
#endif

#if defined(__AROS__)
   SetVar("FONTCONFIG_PATH", "PROGDIR:Conf/font", -1, LV_VAR | GVF_LOCAL_ONLY);
#endif
   aros_timer_open();

	if (!argc)
	{
		freopen("NIL:", "w", stderr);
		freopen("NIL:", "w", stdout);
	}

	//dosnotify_init();

	if(open_libs())
	{
		if(safety_check())
		{
			app = create_application(argc > 1 ? (char*)argv[1] : (char *)"");

			if(app)
			{
				main_loop();
			}

			destroy_application();
		}
	}
	{ extern void owbStopCurlThreadAtExit(); owbStopCurlThreadAtExit(); }
	close_libs();

	//dosnotify_cleanup();

    return 0;
}
