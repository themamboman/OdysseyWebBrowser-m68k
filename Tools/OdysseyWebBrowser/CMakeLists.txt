# amiga/rtg/rtg_lib.c/.h (the CyberGraphX RTG layer) define CyberGfxBase and
# CyberGfx_Init()/Cleanup() -- built but never wired into any target before
# (see WebCore_status.md). main.cpp now calls CyberGfx_Init()/Cleanup(),
# needed since proto/cybergraphics.h's WritePixelArray etc. (called from
# WebKitLegacy's owbbrowserclass.cpp) dereference CyberGfxBase directly.
set(OdysseyWebBrowser_SOURCES
    main.cpp
    "${CMAKE_SOURCE_DIR}/amiga/rtg/rtg_lib.c"
)

set(OdysseyWebBrowser_INCLUDE_DIRECTORIES
    "${CAIRO_INCLUDE_DIRS}"
    "${WEBKIT_DIR}/mui/Api/MorphOS"
    "${WEBKIT_DIR}/mui/Api/AROS/include"
    "${DERIVED_SOURCES_WEBCORE_DIR}"
    "${CMAKE_SOURCE_DIR}/amiga/rtg"
)

# This link list was never updated for the m68k-amigaos3 port and still had
# flags from whatever platform it was copied from (MorphOS/desktop). Fixed
# against the real sysroot, 2026-10-13/14 (see WebCore_status.md):
#   -lssl -lcrypto     -> no such libraries; AmiSSL's real names, already
#                         used by curl/cairo via openssl.pc's own redirect.
#   -lpixman           -> real file is libpixman-1.a.
#   -lfreetype2         -> real file is libfreetype.a (matches fontconfig.pc's
#                         own -lfreetype, not -lfreetype2).
#   -lxml2 -lxslt       -> real .a files exist but live under
#                         WEBKIT_AMIGA_TOOLCHAIN_PREFIX/lib, not the sysroot
#                         lib dir the linker searches by default -- same
#                         reason OptionsMUI.cmake points LIBXML2_LIBRARIES/
#                         LIBXSLT_LIBRARIES at the absolute path instead of a
#                         bare -l flag; do the same here.
#   -lopenurl -lrexxsyslib -lavformat -lavcodec -lavutil -lswresample
#   -lswscale -lpsl -liconv
#                       -> none of these exist anywhere in the toolchain, and
#                         none are actually needed: zero undefined references
#                         to any of their symbols anywhere in
#                         libWebKitLegacy.a/libWebCoreMUI.a (checked directly
#                         with nm). FFmpeg in particular is only needed by
#                         AcinerellaVideoDecoder.cpp, which PlatformMUI.cmake
#                         only compiles `if (ENABLE_VIDEO)` -- off for this
#                         build. openurl.library has no SDK in this toolchain
#                         either, and its one real call site is already dead
#                         code (OpenURLBase declared but never opened).
# libamigaposix.a (the other agent's POSIX shim -- fchmod/fchown/stat/dup/
# fcntl/gettimeofday/etc.) was never added to this list at all; sqlite3.o
# needs fchmod/fchown from it. Given the archive-ordering trap
# stubbed_to_finished.md already flagged (a shim member is only pulled in
# if something earlier in link order references it first), wrap the whole
# set in --start-group/--end-group so ld re-scans everything until no new
# symbols resolve, rather than depending on a fragile specific ordering.
# CMake's own transitive-dependency flattening for the JavaScriptCore/
# WebKitLegacy/WebCore *targets* appends libPAL.a/libWTFMUI.a (and a few
# duplicated others) AFTER our --end-group closes -- confirmed directly in
# the generated link.txt. That leaves them in ld's final, non-grouped pass,
# so when PAL's CryptoDigestOpenSSL.cpp needs AmiSSL's SHA1_Final etc., or
# WTF's URL.cpp/RandomDevice.cpp need ICU's uidna_* / AmiSSL's RAND_bytes,
# those archives (already closed inside the group) can't be re-searched.
# Adding explicit copies of libPAL.a/libWTFMUI.a inside our own group fixes
# it; CMake's later duplicate copies are harmless (already-resolved symbols
# are simply skipped).
set(OdysseyWebBrowser_LIBRARIES
    -Wl,--start-group
    JavaScriptCore
    WebKitLegacy
    WebCore
    "${CMAKE_BINARY_DIR}/lib/libPAL.a"
    "${CMAKE_BINARY_DIR}/lib/libWTFMUI.a"
    # Explicitly adding libb/libm020/libc.a here (a previous attempt at
    # resolving memalign/ctime_r/__locale_ctype_ptr/errno) was WRONG: this
    # GCC fork's actual default runtime is libnix (nbcrt0.o, pulled in
    # implicitly), which already defines fclose/open/__errno etc. itself.
    # Linking that libc.a on top gave multiple-definition errors on every
    # overlapping symbol, AND still didn't work for memalign/link -- that
    # libc.a's own lib_a-malign.o/lib_a-linkr.o need `_memalign_r`/`_link`,
    # which no library in this toolchain (any flavor, checked via nm across
    # the whole sysroot) actually defines. Removed; see amiga_node_compat.c
    # for the real fix (self-contained memalign()/link() shims that don't
    # depend on either competing C library's internals).
    -lz
    -lcairo
    -lpixman-1
    -lfontconfig
    -lfreetype
    "${WEBKIT_AMIGA_TOOLCHAIN_PREFIX}/lib/libxml2.a"
    -lcurl
    -ljpeg
    -lpng
    -lamisslauto
    -lamisslstubs
    -licui18n
    -licuuc
    -licudata
    -lsqlite3
    -lexpat
    "${WEBKIT_AMIGA_TOOLCHAIN_PREFIX}/lib/libxslt.a"
    -ldebug
    -lbz2
    -latomic
    -lamigaposix
    ${ZLIB_LIBRARIES}
    -Wl,--end-group
)

include_directories(${OdysseyWebBrowser_INCLUDE_DIRECTORIES})
add_executable(OWB ${OdysseyWebBrowser_SOURCES})
target_link_libraries(OWB ${OdysseyWebBrowser_LIBRARIES})

# The GCC driver appends its own implicit "-lm" twice on the real link
# command line (once mid-command, once again inside the trailing
# "-( -lc -ldebug ... -lm -)" fallback group it always adds) -- not
# something this file controls. For 5 comparison functions
# (__gedf2/__gtdf2/__ledf2/__ltdf2/__nedf2), both of those uncoordinated
# passes end up independently pulling in their own copy of the same
# libm.a member, producing a real "multiple definition" error (confirmed
# via a real OWB link -- see WebCore_status.md). Both copies are the
# exact same object code either way, so there's no real ambiguity to
# resolve, just linker bookkeeping across two scans that don't know
# about each other -- --allow-multiple-definition is the standard,
# narrowly-scoped fix for exactly this situation.
target_link_options(OWB PRIVATE -Wl,--allow-multiple-definition)
