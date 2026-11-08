# m68k-amigaos.cmake
# CMake toolchain file for cross-compiling to AmigaOS 3.x on Motorola 68k.
# Built from AmigaPorts/m68k-amigaos-gcc (installed compiler: 16.2.0b).
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=Source/cmake/m68k-amigaos.cmake \
#         -DPORT=MUI -DCMAKE_BUILD_TYPE=Release \
#         -DCMAKE_C_COMPILER=m68k-amigaos-gcc \
#         -DCMAKE_CXX_COMPILER=m68k-amigaos-g++ \
#         ..

# This one is important
SET(CMAKE_SYSTEM_NAME Generic)
# This one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# Toolchain install prefix (AmigaPorts m68k-amigaos-gcc). Pass
# -DWEBKIT_AMIGA_TOOLCHAIN_PREFIX=... or set AMIGA_TOOLCHAIN_PREFIX in the
# environment; the default is the toolchain's own default, /opt/amiga.
if (NOT WEBKIT_AMIGA_TOOLCHAIN_PREFIX)
    if (DEFINED ENV{AMIGA_TOOLCHAIN_PREFIX})
        set(WEBKIT_AMIGA_TOOLCHAIN_PREFIX "$ENV{AMIGA_TOOLCHAIN_PREFIX}")
    else ()
        set(WEBKIT_AMIGA_TOOLCHAIN_PREFIX "/opt/amiga")
    endif ()
endif ()
SET(WEBKIT_AMIGA_TOOLCHAIN_PREFIX "${WEBKIT_AMIGA_TOOLCHAIN_PREFIX}" CACHE PATH "Path to the m68k-amigaos-gcc toolchain install prefix")

# Specify the cross compiler
SET(CMAKE_CROSSCOMPILING ON)
SET(CMAKE_C_COMPILER "${WEBKIT_AMIGA_TOOLCHAIN_PREFIX}/bin/m68k-amigaos-gcc")
SET(CMAKE_CXX_COMPILER "${WEBKIT_AMIGA_TOOLCHAIN_PREFIX}/bin/m68k-amigaos-g++")

# Target architecture
SET(CMAKE_SYSTEM_PROCESSOR m68k)
SET(WEBKIT_TARGET_AMIGAOS3 TRUE)

# The AmigaOS 3.x threading library: the aos_multithreading_library submodule in this
# repository, or a checkout beside this repository if one exists (handy when working on it).
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../../aos_multithreading_library/multitask_lib.c")
    GET_FILENAME_COMPONENT(WEBKIT_AMIGA_MULTITASK_LIBRARY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../aos_multithreading_library" ABSOLUTE)
else ()
    GET_FILENAME_COMPONENT(WEBKIT_AMIGA_MULTITASK_LIBRARY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../aos_multithreading_library" ABSOLUTE)
endif ()
MESSAGE(STATUS "WEBKIT_AMIGA_MULTITASK_LIBRARY_ROOT = ${WEBKIT_AMIGA_MULTITASK_LIBRARY_ROOT}")
SET(WEBKIT_AMIGA_MULTITASK_LIBRARY_ROOT "${WEBKIT_AMIGA_MULTITASK_LIBRARY_ROOT}" CACHE PATH "Path to Amiga multitask library" FORCE)

# The m68k-amigaos target sysroot (headers/libs for cairo, freetype, fontconfig, harfbuzz,
# zlib, png, jpeg, etc.). Used by OptionsMUI.cmake to point WebCore at these libraries
# directly, since find_package()+pkg-config would use the HOST's pkg-config and could pick
# up the host's own cairo/freetype; CMAKE_FIND_ROOT_PATH is intentionally empty (see below).
SET(WEBKIT_AMIGA_SYSROOT "${WEBKIT_AMIGA_TOOLCHAIN_PREFIX}/m68k-amigaos" CACHE PATH "Path to the m68k-amigaos target sysroot")

# expat, sqlite3, libxml2 and libxslt are installed in the toolchain prefix's own
# include/lib rather than under WEBKIT_AMIGA_SYSROOT (see docs/amiga/BUILDING.md).

# Options
# -mcrt=clib2 selects the clib2 C library (cpp defines, startfile, lib search
# path all key off this one flag -- see the GCC spec's *self_spec/*lib/
# *startfile/*cpp rules). This has to be consistent across every object that
# gets linked into one executable: AmiSSL/libxml2/cairo/libamigaposix (all
# built by the other agent) were all built assuming clib2's errno/ctime_r/
# __locale_ctype_ptr/rmdir conventions. CMAKE_EXE_LINKER_FLAGS below used to
# carry a leftover "-noixemul" (forces libnix instead -- a different,
# incompatible C library, confirmed via real link errors: libnix defines its
# own errno/rmdir/open/fclose that collide with or lack what those prebuilt
# deps need). Matching clib2 here too, not just at link time, so WTF/JSC/
# WebCore themselves see the same __CLIB2__ headers.
# -fbaserel32 (32-bit base-relative displacement) instead of -fbaserel
# (16-bit): the whole program's static data (WebKit + JSC + ICU + cairo +
# curl + sqlite3 + everything else sharing one base-relative window) grew
# past what a 16-bit signed displacement can reach from the base register
# (confirmed via a real OWB link: "relocation truncated to fit: DREL16"
# against _edata/_impure_ptr/.data, and "dangerous relocation" in clib2's
# own prebuilt constructor-list startup object, which can't be
# individually recompiled -- see WebCore_status.md). -fbaserel32 requires
# the 68020+ extended addressing modes (not available on bare 68000/68010),
# but -mcpu=68040 already requires that, so this doesn't narrow the real-
# world hardware target any further.
# Update: the build that runs uses -fbaserel -fno-pic (16-bit base-relative), made to link by
# re-building the prefixed libgcc/libstdc++ (see docs/amiga/BUILDING.md). -fbaserel32 fails
# to link against clib2's prebuilt startup code ("dangerous relocation").
SET(CMAKE_C_FLAGS_INIT "-mcpu=68040 -fbaserel -fno-pic -mcrt=clib2")
SET(CMAKE_CXX_FLAGS_INIT "-mcpu=68040 -fbaserel -fno-pic -mcrt=clib2")
SET(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags used by the C compiler during RELEASE builds.")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags used by the CXX compiler during RELEASE builds.")

SET(CMAKE_C_FLAGS_RELWITHDEBINFO "-O1 -gdwarf -g1 -DNDEBUG -fno-exceptions -fno-strict-aliasing" CACHE STRING "Flags used by the C compiler during RELWITHDEBINFO builds.")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O1 -gdwarf -g1 -DNDEBUG -fno-exceptions -fno-strict-aliasing -fno-rtti" CACHE STRING "Flags used by the CXX compiler during RELWITHDEBINFO builds.")

SET(CMAKE_MODULE_LINKER_FLAGS "")

# Where is the target environment
SET(CMAKE_FIND_ROOT_PATH "")

# Search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# For libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# AmigaOS-specific definitions
ADD_DEFINITIONS(-DAMIGA -D__AMIGAOS3__ -D__M68K__ -D__REGISTERS__)
ADD_DEFINITIONS(-D__SASC__)

# This libc's <string.h>/etc. hide strdup() and other non-strict-ANSI declarations
# behind feature-test macros (see sys/features.h) that default to off. Defining this
# globally (rather than per translation unit) avoids depending on include order --
# a file that pulls in <string.h> transitively before defining it locally would
# otherwise lock in the narrower visibility via the header's include guard.
ADD_DEFINITIONS(-D_DEFAULT_SOURCE)

# -noixemul avoided the ixemul.library runtime dependency, but selected
# libnix as the C library -- incompatible with AmiSSL/libxml2/cairo/
# libamigaposix, all built against clib2 (see the CMAKE_C_FLAGS_INIT comment
# above). -mcrt=clib2 keeps the same "no ixemul.library" property (clib2 is
# its own independent, non-ixemul runtime) while matching everything else.
SET(CMAKE_EXE_LINKER_FLAGS "-mcrt=clib2 -lamiga -lm -lgcc" CACHE STRING "Linker flags for AmigaOS executables")

# Static linking for WebKit components (matching Odyssey's existing approach)
SET(CMAKE_POSITION_INDEPENDENT_CODE OFF)