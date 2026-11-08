/*
 * Copyright (c) 2005, 2007, Google Inc. All rights reserved.
 * Copyright (C) 2005-2018 Apple Inc. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include <wtf/FastMalloc.h>

#include <limits>
#include <string.h>
#include <wtf/CheckedArithmetic.h>
#include <wtf/DataLog.h>

#if OS(WINDOWS)
#include <windows.h>
#elif OS(AROS)
#else
#include <pthread.h>
#if HAVE(RESOURCE_H)
#include <sys/resource.h>
#endif // HAVE(RESOURCE_H)
#endif

#if OS(DARWIN)
#include <mach/mach_init.h>
#include <malloc/malloc.h>
#endif

namespace WTF {

#if !defined(NDEBUG)
namespace {
// We do not use std::numeric_limits<size_t>::max() here due to the edge case in VC++.
// https://bugs.webkit.org/show_bug.cgi?id=173720
static size_t maxSingleAllocationSize = SIZE_MAX;
};

void fastSetMaxSingleAllocationSize(size_t size)
{
    maxSingleAllocationSize = size;
}

#define ASSERT_IS_WITHIN_LIMIT(size) do { \
        size_t size__ = (size); \
        ASSERT_WITH_MESSAGE((size__) <= maxSingleAllocationSize, "Requested size (%zu) exceeds max single allocation size set for testing (%zu)", (size__), maxSingleAllocationSize); \
    } while (false)

#define FAIL_IF_EXCEEDS_LIMIT(size) do { \
        if (UNLIKELY((size) > maxSingleAllocationSize)) \
            return nullptr; \
    } while (false)

#else // !defined(NDEBUG)

#define ASSERT_IS_WITHIN_LIMIT(size)
#define FAIL_IF_EXCEEDS_LIMIT(size)

#endif // !defined(NDEBUG)

void* fastZeroedMalloc(size_t n) 
{
    void* result = fastMalloc(n);
    if(result)
        memset(result, 0, n);
    return result;
}

char* fastStrDup(const char* src)
{
    size_t len = strlen(src) + 1;
    char* dup = static_cast<char*>(fastMalloc(len));
    memcpy(dup, src, len);
    return dup;
}

TryMallocReturnValue tryFastZeroedMalloc(size_t n) 
{
    void* result;
    if (!tryFastMalloc(n).getValue(result))
        return 0;
    memset(result, 0, n);
    return result;
}

} // namespace WTF

#if defined(USE_SYSTEM_MALLOC) && USE_SYSTEM_MALLOC

#include <wtf/OSAllocator.h>

#if OS(WINDOWS)
#include <malloc.h>
#elif OS(AROS)
#include "mui/arosbailout.h"
#include "mui/execallocator.h"
#elif OS(AMIGA)
#include <malloc.h>
#elif OS(MORPHOS)
#include <clib/debug_protos.h>
#endif

#if defined(__AMIGAOS3__)
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <proto/exec.h>
#endif

#if defined(__AMIGAOS3__)
// Source/WTF/wtf/amiga/OutOfMemoryAmiga.cpp: WebKit never checks these allocations, so a
// failure must not return NULL. owbOutOfMemory() asks the user to retry or stop OWB.
extern "C" bool owbOutOfMemory(size_t size);
extern "C" bool owbSimulateOutOfMemory(size_t size);
extern "C" bool owbMemoryTooLow(size_t size);
// Allocations at least this big check that the system keeps some memory free.
static constexpr size_t amigaLargeAllocation = 64 * 1024;
static inline bool amigaRefuse(size_t size)
{
    return owbSimulateOutOfMemory(size) || (size >= amigaLargeAllocation && owbMemoryTooLow(size));
}
#endif

namespace WTF {

bool isFastMallocEnabled()
{
    return false;
}

size_t fastMallocGoodSize(size_t bytes)
{
#if OS(DARWIN)
    return malloc_good_size(bytes);
#else
    return bytes;
#endif
}

#if OS(WINDOWS)

void* fastAlignedMalloc(size_t alignment, size_t size) 
{
    ASSERT_IS_WITHIN_LIMIT(size);
    void* p = _aligned_malloc(size, alignment);
    if (UNLIKELY(!p))
        CRASH();
    return p;
}

void* tryFastAlignedMalloc(size_t alignment, size_t size) 
{
    FAIL_IF_EXCEEDS_LIMIT(size);
    return _aligned_malloc(size, alignment);
}

void fastAlignedFree(void* p) 
{
    _aligned_free(p);
}

#else

#if defined(__AMIGAOS3__)
/* memalign() is unusable on this toolchain: lib_a-malign.o's memalign()
 * calls through to a reentrant __memalign_r() that no C library variant
 * in this sysroot actually defines (confirmed via nm across every libb/
 * libnix/clib2/ixemul flavor -- see WebCore_status.md). Rather than reuse
 * plain free() (which can't handle an adjusted-for-alignment pointer, the
 * same reason OS(WINDOWS) above uses its own paired _aligned_malloc/
 * _aligned_free instead of malloc/free), implement a small self-contained
 * aligned allocator: over-allocate via plain malloc, round up to the
 * requested alignment, and stash the real malloc() pointer just before the
 * aligned pointer so fastAlignedFree can recover and free it correctly. */
/* JavaScriptCore asks for 16 KB blocks aligned to 16 KB (and 4 KB handle blocks aligned to
 * 4 KB). Over-allocating each one costs about twice its size, so blocks of that shape come from
 * a pool instead: regions of about 1 MB are allocated once, aligned once, and handed out block
 * by block (about 1.5% overhead). A region goes back to the system when all its blocks are free,
 * keeping one empty region per block size so a page that frees and reallocates doesn't churn.
 * If a region can't be had, the per-block fallback below is used. */
struct AmigaAlignedRegion {
    AmigaAlignedRegion* next;
    APTR base;
    ULONG allocationSize;
    uintptr_t start;
    size_t blockSize;
    unsigned blockCount;
    unsigned used;
    uint32_t usedMask[2];
};

static constexpr unsigned amigaMaxBlocksPerRegion = 64;
static struct SignalSemaphore amigaRegionLock;
static volatile bool amigaRegionLockInitialized;
static AmigaAlignedRegion* amigaRegions;

static void amigaEnsureRegionLock()
{
    if (amigaRegionLockInitialized)
        return;
    Forbid();
    if (!amigaRegionLockInitialized) {
        InitSemaphore(&amigaRegionLock);
        amigaRegionLockInitialized = true;
    }
    Permit();
}

static bool amigaIsPooledShape(size_t alignment, size_t size)
{
    return size == alignment && alignment >= 4096 && alignment <= 65536 && !(alignment & (alignment - 1));
}

static void* amigaTakeBlock(AmigaAlignedRegion* region)
{
    for (unsigned i = 0; i < region->blockCount; ++i) {
        uint32_t bit = 1u << (i & 31);
        if (!(region->usedMask[i >> 5] & bit)) {
            region->usedMask[i >> 5] |= bit;
            region->used++;
            return reinterpret_cast<void*>(region->start + i * region->blockSize);
        }
    }
    return nullptr;
}

static void* amigaPoolAlloc(size_t blockSize)
{
    amigaEnsureRegionLock();
    ObtainSemaphore(&amigaRegionLock);

    for (AmigaAlignedRegion* region = amigaRegions; region; region = region->next) {
        if (region->blockSize == blockSize && region->used < region->blockCount) {
            void* block = amigaTakeBlock(region);
            ReleaseSemaphore(&amigaRegionLock);
            return block;
        }
    }

    size_t blockCount = (1024 * 1024) / blockSize;
    if (blockCount > amigaMaxBlocksPerRegion)
        blockCount = amigaMaxBlocksPerRegion;
    ULONG allocationSize = static_cast<ULONG>(blockCount * blockSize + blockSize);
    if (owbMemoryTooLow(allocationSize)) {
        ReleaseSemaphore(&amigaRegionLock);
        return nullptr;
    }
    APTR base = AllocMem(allocationSize, MEMF_ANY);
    auto* region = base ? static_cast<AmigaAlignedRegion*>(AllocMem(sizeof(AmigaAlignedRegion), MEMF_ANY | MEMF_CLEAR)) : nullptr;
    if (!region) {
        if (base)
            FreeMem(base, allocationSize);
        ReleaseSemaphore(&amigaRegionLock);
        return nullptr;
    }
    region->base = base;
    region->allocationSize = allocationSize;
    region->start = (reinterpret_cast<uintptr_t>(base) + blockSize - 1) & ~(static_cast<uintptr_t>(blockSize) - 1);
    region->blockSize = blockSize;
    region->blockCount = static_cast<unsigned>(blockCount);
    region->next = amigaRegions;
    amigaRegions = region;

    void* block = amigaTakeBlock(region);
    ReleaseSemaphore(&amigaRegionLock);
    return block;
}

// Returns false if p is not a pooled block.
static bool amigaPoolFree(void* p)
{
    if (!amigaRegionLockInitialized)
        return false;
    uintptr_t address = reinterpret_cast<uintptr_t>(p);
    ObtainSemaphore(&amigaRegionLock);

    AmigaAlignedRegion* previous = nullptr;
    for (AmigaAlignedRegion* region = amigaRegions; region; previous = region, region = region->next) {
        if (address < region->start || address >= region->start + region->blockCount * region->blockSize)
            continue;
        unsigned index = static_cast<unsigned>((address - region->start) / region->blockSize);
        region->usedMask[index >> 5] &= ~(1u << (index & 31));
        region->used--;

        if (!region->used) {
            bool anotherEmpty = false;
            for (AmigaAlignedRegion* other = amigaRegions; other; other = other->next) {
                if (other != region && other->blockSize == region->blockSize && !other->used) {
                    anotherEmpty = true;
                    break;
                }
            }
            if (anotherEmpty) {
                if (previous)
                    previous->next = region->next;
                else
                    amigaRegions = region->next;
                FreeMem(region->base, region->allocationSize);
                FreeMem(region, sizeof(AmigaAlignedRegion));
            }
        }
        ReleaseSemaphore(&amigaRegionLock);
        return true;
    }

    ReleaseSemaphore(&amigaRegionLock);
    return false;
}

static void* amigaAlignedMalloc(size_t alignment, size_t size)
{
    if (amigaIsPooledShape(alignment, size)) {
        // A pooled block either comes from an existing region or from a new region, which
        // checks the free-memory minimum itself; don't fall back to plain malloc below it.
        if (void* block = amigaPoolAlloc(alignment))
            return block;
        if (owbMemoryTooLow(size))
            return nullptr;
    } else if (size >= amigaLargeAllocation && owbMemoryTooLow(size))
        return nullptr;

    if (alignment < sizeof(void*))
        alignment = sizeof(void*);
    size_t total = size + alignment - 1 + sizeof(void*);
    void* raw = malloc(total);
    if (!raw)
        return nullptr;
    uintptr_t rawAddr = reinterpret_cast<uintptr_t>(raw) + sizeof(void*);
    uintptr_t aligned = (rawAddr + alignment - 1) & ~(static_cast<uintptr_t>(alignment) - 1);
    reinterpret_cast<void**>(aligned)[-1] = raw;
    return reinterpret_cast<void*>(aligned);
}

static void amigaAlignedFree(void* p)
{
    if (!p)
        return;
    if (amigaPoolFree(p))
        return;
    free(reinterpret_cast<void**>(p)[-1]);
}
#endif

void* fastAlignedMalloc(size_t alignment, size_t size)
{
    ASSERT_IS_WITHIN_LIMIT(size);
retry:
    void* p = nullptr;
#if OS(AROS)
    p = allocator_getmem_aligned(size, alignment);
#elif defined(__AMIGAOS3__)
    if (!owbSimulateOutOfMemory(size))
        p = amigaAlignedMalloc(alignment, size ? size : 2);
#elif OS(AMIGA)
    p = memalign(alignment, size ? size : 2);
#else
    posix_memalign(&p, alignment, size ? size : 2);
#endif

    if (UNLIKELY(!p))
    {
#if OS(AROS)
        if (aros_memory_allocation_error(size ? size : 2, alignment) == 1)
            goto retry;

        if (aros_is_memory_bailout())
            aros_bailout_jump();
#endif
#if OS(MORPHOS)
        kprintf("fastAlignedMalloc: Failed to allocate %lu bytes. Happy crash sponsored by WebKit will follow.\n", n ? n : 2);
        if(morphos_crash(size ? size : 2))
            goto retry;
#endif
#if defined(__AMIGAOS3__)
        if (owbOutOfMemory(size ? size : 2))
            goto retry;
#endif
    }

    return p;
}

void* tryFastAlignedMalloc(size_t alignment, size_t size)
{
    FAIL_IF_EXCEEDS_LIMIT(size);
    void* p = nullptr;
#if OS(AROS)
    p = allocator_getmem_aligned(size, alignment);
#elif defined(__AMIGAOS3__)
    p = amigaAlignedMalloc(alignment, size);
#elif OS(AMIGA)
    p = memalign(alignment, size);
#else
    posix_memalign(&p, alignment, size);
#endif
    return p;
}

void fastAlignedFree(void* p)
{
#if OS(AROS)
    allocator_freemem(p);
#elif defined(__AMIGAOS3__)
    amigaAlignedFree(p);
#else
    free(p);
#endif
}

#endif // OS(WINDOWS)

TryMallocReturnValue tryFastMalloc(size_t n) 
{
    FAIL_IF_EXCEEDS_LIMIT(n);
#if defined(__AMIGAOS3__)
    if (n >= amigaLargeAllocation && owbMemoryTooLow(n))
        return nullptr;
#endif
    return malloc(n ? n : 2);
}

void* fastMalloc(size_t n)
{
    ASSERT_IS_WITHIN_LIMIT(n);
retry:
#if defined(__AMIGAOS3__)
    void* result = amigaRefuse(n) ? nullptr : malloc(n ? n : 2);
#else
    void* result = malloc(n ? n : 2);
#endif
    if (!result)
    {
#if OS(AROS)
        if (aros_memory_allocation_error(n ? n : 2, 4) == 1)
            goto retry;

        if (aros_is_memory_bailout())
            aros_bailout_jump();
#endif
#if OS(MORPHOS)
        kprintf("fastMalloc: Failed to allocate %lu bytes. Happy crash sponsored by WebKit will follow.\n", n ? n : 2);
        if(morphos_crash(n ? n : 2)) 
            goto retry;
#endif
#if defined(__AMIGAOS3__)
        if (owbOutOfMemory(n ? n : 2))
            goto retry;
#endif
    }

    return result;
}

TryMallocReturnValue tryFastCalloc(size_t n_elements, size_t element_size)
{
    FAIL_IF_EXCEEDS_LIMIT(n_elements * element_size);
#if defined(__AMIGAOS3__)
    if (n_elements * element_size >= amigaLargeAllocation && owbMemoryTooLow(n_elements * element_size))
        return nullptr;
#endif
    return calloc(n_elements ? n_elements : 1, element_size ? element_size : 2);
}

void* fastCalloc(size_t n_elements, size_t element_size)
{
    ASSERT_IS_WITHIN_LIMIT(n_elements * element_size);
retry:
#if defined(__AMIGAOS3__)
    void* result = amigaRefuse(n_elements * element_size) ? nullptr : calloc(n_elements ? n_elements : 1, element_size ? element_size : 2);
#else
    void* result = calloc(n_elements ? n_elements : 1, element_size ? element_size : 2);
#endif
    if (!result)
    {
#if OS(AROS)
        if (aros_memory_allocation_error((n_elements ? n_elements : 1)*(element_size ? element_size : 2), 4) == 1)
            goto retry;

        if (aros_is_memory_bailout())
            aros_bailout_jump();
#endif
#if OS(MORPHOS)
        kprintf("fastCalloc: Failed to allocate %lu x %lu bytes. Happy crash sponsored by WebKit will follow.\n", n_elements ? n_elements : 1, element_size ? element_size : 2);
        if(morphos_crash((n_elements ? n_elements : 1)*(element_size ? element_size : 2))) 
            goto retry;
#endif
#if defined(__AMIGAOS3__)
        if (owbOutOfMemory((n_elements ? n_elements : 1) * (element_size ? element_size : 2)))
            goto retry;
#endif
    }

    return result;
}

void fastFree(void* p)
{
    free(p);
}

void* fastRealloc(void* p, size_t n)
{
    ASSERT_IS_WITHIN_LIMIT(n);
retry:
#if defined(__AMIGAOS3__)
    void* result = amigaRefuse(n) ? nullptr : realloc(p, n ? n : 2);
#else
    void* result = realloc(p, n ? n : 2);
#endif
    if (!result)
    {
#if OS(AROS)
        if (aros_memory_allocation_error(n ? n : 2, 4) == 1)
            goto retry;

        if (aros_is_memory_bailout())
            aros_bailout_jump();
#endif
#if OS(MORPHOS)
        kprintf("fastRealloc: Failed to allocate %lu bytes. Happy crash sponsored by WebKit will follow.\n", n ? n : 2);
        if(morphos_crash(n ? n : 2))
            goto retry;
#endif
#if defined(__AMIGAOS3__)
        if (owbOutOfMemory(n ? n : 2))
            goto retry;
#endif
    }
    return result;
}

TryMallocReturnValue tryFastRealloc(void* p, size_t n)
{
    FAIL_IF_EXCEEDS_LIMIT(n);
#if defined(__AMIGAOS3__)
    if (n >= amigaLargeAllocation && owbMemoryTooLow(n))
        return nullptr;
#endif
    return realloc(p, n);
}

void releaseFastMallocFreeMemory() { }
void releaseFastMallocFreeMemoryForThisThread() { }
    
FastMallocStatistics fastMallocStatistics()
{
    FastMallocStatistics statistics = { 0, 0, 0 };
    return statistics;
}

size_t fastMallocSize(const void* p)
{
#if OS(DARWIN)
    return malloc_size(p);
#elif OS(WINDOWS)
    return _msize(const_cast<void*>(p));
#else
    UNUSED_PARAM(p);
    return 1;
#endif
}

void fastCommitAlignedMemory(void* ptr, size_t size)
{
    OSAllocator::commit(ptr, size, true, false);
}

void fastDecommitAlignedMemory(void* ptr, size_t size)
{
    OSAllocator::decommit(ptr, size);
}

void fastEnableMiniMode() { }

} // namespace WTF

#else // defined(USE_SYSTEM_MALLOC) && USE_SYSTEM_MALLOC

#include <bmalloc/bmalloc.h>

namespace WTF {

bool isFastMallocEnabled()
{
    return bmalloc::api::isEnabled();
}

void* fastMalloc(size_t size)
{
    ASSERT_IS_WITHIN_LIMIT(size);
    return bmalloc::api::malloc(size);
}

void* fastCalloc(size_t numElements, size_t elementSize)
{
    ASSERT_IS_WITHIN_LIMIT(numElements * elementSize);
    Checked<size_t> checkedSize = elementSize;
    checkedSize *= numElements;
    void* result = fastZeroedMalloc(checkedSize.unsafeGet());
    if (!result)
        CRASH();
    return result;
}

void* fastRealloc(void* object, size_t size)
{
    ASSERT_IS_WITHIN_LIMIT(size);
    return bmalloc::api::realloc(object, size);
}

void fastFree(void* object)
{
    bmalloc::api::free(object);
}

size_t fastMallocSize(const void*)
{
    // FIXME: This is incorrect; best fix is probably to remove this function.
    // Caller currently are all using this for assertion, not to actually check
    // the size of the allocation, so maybe we can come up with something for that.
    return 1;
}

size_t fastMallocGoodSize(size_t size)
{
    return size;
}

void* fastAlignedMalloc(size_t alignment, size_t size) 
{
    ASSERT_IS_WITHIN_LIMIT(size);
    return bmalloc::api::memalign(alignment, size);
}

void* tryFastAlignedMalloc(size_t alignment, size_t size) 
{
    FAIL_IF_EXCEEDS_LIMIT(size);
    return bmalloc::api::tryMemalign(alignment, size);
}

void fastAlignedFree(void* p) 
{
    bmalloc::api::free(p);
}

TryMallocReturnValue tryFastMalloc(size_t size)
{
    FAIL_IF_EXCEEDS_LIMIT(size);
    return bmalloc::api::tryMalloc(size);
}
    
TryMallocReturnValue tryFastCalloc(size_t numElements, size_t elementSize)
{
    FAIL_IF_EXCEEDS_LIMIT(numElements * elementSize);
    Checked<size_t, RecordOverflow> checkedSize = elementSize;
    checkedSize *= numElements;
    if (checkedSize.hasOverflowed())
        return nullptr;
    return tryFastZeroedMalloc(checkedSize.unsafeGet());
}
    
TryMallocReturnValue tryFastRealloc(void* object, size_t newSize)
{
    FAIL_IF_EXCEEDS_LIMIT(newSize);
    return bmalloc::api::tryRealloc(object, newSize);
}

void releaseFastMallocFreeMemoryForThisThread()
{
    bmalloc::api::scavengeThisThread();
}

void releaseFastMallocFreeMemory()
{
    bmalloc::api::scavenge();
}

FastMallocStatistics fastMallocStatistics()
{

    // FIXME: Can bmalloc itself report the stats instead of relying on the OS?
    FastMallocStatistics statistics;
    statistics.freeListBytes = 0;
    statistics.reservedVMBytes = 0;

#if OS(WINDOWS)
    PROCESS_MEMORY_COUNTERS resourceUsage;
    GetProcessMemoryInfo(GetCurrentProcess(), &resourceUsage, sizeof(resourceUsage));
    statistics.committedVMBytes = resourceUsage.PeakWorkingSetSize;
#elif HAVE(RESOURCE_H)
    struct rusage resourceUsage;
    getrusage(RUSAGE_SELF, &resourceUsage);

#if OS(DARWIN)
    statistics.committedVMBytes = resourceUsage.ru_maxrss;
#else
    statistics.committedVMBytes = resourceUsage.ru_maxrss * 1024;
#endif // OS(DARWIN)

#endif // OS(WINDOWS)
    return statistics;
}

void fastCommitAlignedMemory(void* ptr, size_t size)
{
    bmalloc::api::commitAlignedPhysical(ptr, size);
}

void fastDecommitAlignedMemory(void* ptr, size_t size)
{
    bmalloc::api::decommitAlignedPhysical(ptr, size);
}

void fastEnableMiniMode()
{
    bmalloc::api::enableMiniMode();
}

} // namespace WTF

#endif // defined(USE_SYSTEM_MALLOC) && USE_SYSTEM_MALLOC
