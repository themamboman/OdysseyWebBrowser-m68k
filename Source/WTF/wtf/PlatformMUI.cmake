list(APPEND WTF_SOURCES
    generic/WorkQueueGeneric.cpp
    generic/RunLoopGeneric.cpp
    text/mui/TextBreakIteratorInternalICUMorphOS.cpp
    mui/CPUTimeAROS.cpp
    mui/LanguageMorphOS.cpp
    generic/MemoryPressureHandlerGeneric.cpp
    generic/MemoryFootprintGeneric.cpp
    generic/MainThreadGeneric.cpp
)

if (WEBKIT_TARGET_AMIGAOS3)
    if (NOT WEBKIT_AMIGA_MULTITASK_LIBRARY_ROOT)
        get_filename_component(WEBKIT_AMIGA_MULTITASK_LIBRARY_ROOT "${CMAKE_SOURCE_DIR}/aos_multithreading_library" ABSOLUTE)
    endif()
    include_directories("${WEBKIT_AMIGA_MULTITASK_LIBRARY_ROOT}")
    set(WTF_AMIGA_ICU_INCLUDE_DIR "${WTF_DIR}/icu")
    list(APPEND WTF_SOURCES
        amiga/OSAllocatorAmiga.cpp
        amiga/ThreadingAmiga.cpp
        amiga/FileSystemAmiga.cpp
        "${WEBKIT_AMIGA_MULTITASK_LIBRARY_ROOT}/multitask_lib.c"
    )
else ()
    list(APPEND WTF_SOURCES
        posix/FileSystemPOSIX.cpp
        mui/execallocator.cpp
        OSAllocatorAROS.cpp
    )
    list(APPEND WTF_SOURCES posix/ThreadingPOSIX.cpp)
endif ()
