list(APPEND PAL_SOURCES
    crypto/openssl/CryptoDigestOpenSSL.cpp
    text/KillRing.cpp
    system/mui/SoundMorphOS.cpp
    system/ClockGeneric.cpp
)

if (WEBKIT_TARGET_AMIGAOS3)
    # WTF's ThreadingPrimitives.h does #include "multitask_lib.h" (the Amiga threading helper
    # library built alongside this port); every target that pulls in WTF threading headers needs
    # this directory on its include path. WTF's own PlatformMUI.cmake adds it for itself the same
    # way, but that's directory-scoped to Source/WTF/wtf and doesn't reach other targets like PAL.
    if (NOT WEBKIT_AMIGA_MULTITASK_LIBRARY_ROOT)
        get_filename_component(WEBKIT_AMIGA_MULTITASK_LIBRARY_ROOT "${CMAKE_SOURCE_DIR}/aos_multithreading_library" ABSOLUTE)
    endif()
    include_directories("${WEBKIT_AMIGA_MULTITASK_LIBRARY_ROOT}")
endif()
