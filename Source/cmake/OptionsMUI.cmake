include(GNUInstallDirs)

# FIXME: We want to expose fewer options to downstream, but for now everything is public.

WEBKIT_OPTION_BEGIN()

#Enabled
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_VIDEO PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(USE_SYSTEM_MALLOC PUBLIC ON)
if (WTF_CPU_X86_64)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_JIT PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FTL_JIT PUBLIC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_C_LOOP PUBLIC OFF)
else ()
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_JIT PUBLIC OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FTL_JIT PUBLIC OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_C_LOOP PUBLIC ON)
endif ()
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_MEDIA_SOURCE PUBLIC ON)

WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_BOX_DECORATION_BREAK PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_CONIC_GRADIENTS PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_PAINTING_API PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_TYPED_OM PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_LEGACY_CSS_VENDOR_PREFIXES PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_DRAG_SUPPORT PRIVATE ON)

WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_INPUT_TYPE_COLOR PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_INPUT_TYPE_DATE PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_INPUT_TYPE_DATETIMELOCAL PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_INPUT_TYPE_MONTH PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_INPUT_TYPE_TIME PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_INPUT_TYPE_WEEK PRIVATE ON)

WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_MHTML PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_MEDIA_STATISTICS PRIVATE ON)

#Disabled

# WebAssembly not compiling
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_WEBASSEMBLY PUBLIC OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_WEB_AUDIO PUBLIC OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_WEB_CRYPTO PUBLIC OFF)

WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_GEOLOCATION PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_SAMPLING_PROFILER PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_3D_TRANSFORMS PRIVATE OFF)
# Doesn't work with curl backend
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FTPDIR PRIVATE OFF)
# Missing dependencies
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_REMOTE_INSPECTOR PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_SMOOTH_SCROLLING PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_WEBGL PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_SERVICE_WORKER PRIVATE OFF)
# libpsl was never cross-built for m68k-amigaos (not in any agent's dependency list); cookie
# domain/public-suffix validation just falls back to WebCore's simpler built-in heuristic.
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_PUBLIC_SUFFIX_LIST PRIVATE OFF)
# libxml2 on this port is Agent B's hand-written stub (agent-b-stubs/libxml2/libxml_stub.c), not a
# real parser -- it has no parserInternals.h and couldn't actually run an XSLT transform even if it
# did. Disabling XSLT avoids compiling against fake libxml2 internals for a feature that would
# silently misbehave at runtime anyway.
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_XSLT PRIVATE OFF)

#Candidates
#ENABLE_CONTENT_EXTENSIONS
#ENABLE_CURSOR_VISIBILITY
#ENABLE_DOWNLOAD_ATTRIBUTE

# FIXME: Perhaps we need a more generic way of defining dependencies between features.
# VIDEO_TRACK depends on VIDEO.
if (NOT ENABLE_VIDEO AND ENABLE_VIDEO_TRACK)
    message(STATUS "Disabling VIDEO_TRACK since VIDEO support is disabled.")
    set(ENABLE_VIDEO_TRACK OFF)
endif ()
WEBKIT_OPTION_END()

set(PROJECT_VERSION_MAJOR 2)
set(PROJECT_VERSION_MINOR 9)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

set(WEBKIT_MICRO_VERSION ${PROJECT_VERSION_PATCH})
set(WEBKIT_MINOR_VERSION ${PROJECT_VERSION_MINOR})
set(WEBKIT_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})

set(ENABLE_WEBCORE ON)
set(ENABLE_INSPECTOR ON)
set(ENABLE_PLUGIN_PROCESS ON)
set(ENABLE_WEBKIT OFF)
set(ENABLE_WEBKIT_LEGACY ON)

set(WTF_USE_ICU_UNICODE 0)
set(WTF_USE_CURL 1)

set(WTF_OUTPUT_NAME WTFMUI)
set(JavaScriptCore_OUTPUT_NAME javascriptcoremui)
set(WebCore_OUTPUT_NAME WebCoreMUI)
set(WebKit_OUTPUT_NAME webkitmui)
set(WebKit2_OUTPUT_NAME webkit2mui)
set(WebKit2_WebProcess_OUTPUT_NAME WebKitWebProcess)

add_definitions(-DUSER_AGENT_GTK_MAJOR_VERSION=601)
add_definitions(-DUSER_AGENT_GTK_MINOR_VERSION=1)

# FIXME: These need to be configurable.

#find_package(Cairo 1.10.2 REQUIRED)
#find_package(Fontconfig 2.8.0 REQUIRED)
#find_package(Freetype 2.4.2 REQUIRED)
#find_package(ICU REQUIRED)
#find_package(JPEG REQUIRED)
#find_package(LibXml2 2.8.0 REQUIRED)
#find_package(LibXslt 1.1.7 REQUIRED)
#find_package(PNG REQUIRED)
#find_package(Sqlite REQUIRED)
#find_package(ZLIB REQUIRED)

if (WEBKIT_TARGET_AMIGAOS3)
    # Cairo/FreeType/Fontconfig/HarfBuzz were cross-built for m68k and installed to the target
    # sysroot (${WEBKIT_AMIGA_SYSROOT}/include, lib), but find_package() above is commented out
    # project-wide, so WebCore's platform/Cairo.cmake and platform/FreeType.cmake see empty
    # CAIRO_INCLUDE_DIRS etc. Set them directly from the known sysroot layout instead of
    # uncommenting find_package(): that goes through pkg-config, which on this dev machine would
    # happily resolve to the HOST's own (x86_64) cairo/freetype if pkg-config isn't explicitly
    # redirected at the m68k .pc files -- and CMAKE_FIND_ROOT_PATH is deliberately empty in
    # m68k-amigaos.cmake, so there's no sysroot isolation to fall back on.
    set(CAIRO_INCLUDE_DIRS "${WEBKIT_AMIGA_SYSROOT}/include/cairo")
    set(CAIRO_LIBRARIES cairo)
    set(FREETYPE_INCLUDE_DIRS "${WEBKIT_AMIGA_SYSROOT}/include/freetype2")
    set(FREETYPE_LIBRARIES freetype)
    set(FONTCONFIG_INCLUDE_DIRS "${WEBKIT_AMIGA_SYSROOT}/include")
    set(FONTCONFIG_LIBRARIES fontconfig)
    set(HARFBUZZ_INCLUDE_DIRS "${WEBKIT_AMIGA_SYSROOT}/include/harfbuzz")
    set(HARFBUZZ_LIBRARIES harfbuzz)
    set(PNG_INCLUDE_DIRS "${WEBKIT_AMIGA_SYSROOT}/include/libpng16")
    set(PNG_LIBRARIES png16)

    # Agent B's stub libraries live directly under the toolchain prefix's include/lib (see the
    # comment by WEBKIT_AMIGA_TOOLCHAIN_PREFIX in m68k-amigaos.cmake), not WEBKIT_AMIGA_SYSROOT.
    # Pointing *_LIBRARIES straight at the .a files (rather than a bare name + -L) sidesteps the
    # fact that this directory isn't on the linker's default search path the way the main sysroot's
    # lib dir is.
    set(SQLITE_INCLUDE_DIR "${WEBKIT_AMIGA_TOOLCHAIN_PREFIX}/include")
    set(SQLITE_LIBRARIES "${WEBKIT_AMIGA_TOOLCHAIN_PREFIX}/lib/libsqlite3.a")
    set(LIBXML2_INCLUDE_DIR "${WEBKIT_AMIGA_TOOLCHAIN_PREFIX}/include")
    set(LIBXML2_LIBRARIES "${WEBKIT_AMIGA_TOOLCHAIN_PREFIX}/lib/libxml2.a")
    set(LIBXSLT_INCLUDE_DIR "${WEBKIT_AMIGA_TOOLCHAIN_PREFIX}/include")
    set(LIBXSLT_LIBRARIES "${WEBKIT_AMIGA_TOOLCHAIN_PREFIX}/lib/libxslt.a")
endif ()

# We don't use find_package for GLX because it is part of -lGL, unlike EGL.
#find_package(OpenGL)
#check_include_files("GL/glx.h" GLX_FOUND)
#find_package(EGL)

if (EGL_FOUND)
    set(WTF_USE_EGL 1)
endif ()

if (ENABLE_SPELLCHECK)
    find_package(Enchant REQUIRED)
endif ()

if (ENABLE_INDEXED_DATABASE)
    set(WTF_USE_LEVELDB 1)
    add_definitions(-DWTF_USE_LEVELDB=1)
endif ()

set(CPACK_SOURCE_GENERATOR TBZ2)
