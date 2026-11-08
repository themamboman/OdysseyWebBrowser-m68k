#include "config.h"
#include <wtf/OSAllocator.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <limits>
#include <stdint.h>
#include <string.h>

namespace WTF {

namespace {

constexpr size_t pageSize = 4096;

struct AllocationHeader {
    APTR base;
    ULONG size;
};

void* allocatePages(size_t bytes)
{
    constexpr size_t overhead = pageSize + sizeof(AllocationHeader);
    if (bytes > std::numeric_limits<ULONG>::max() - overhead)
        return nullptr;

    ULONG allocationSize = static_cast<ULONG>(bytes + overhead);
    APTR base = AllocMem(allocationSize, MEMF_ANY | MEMF_CLEAR);
    if (!base)
        return nullptr;

    uintptr_t alignedAddress = (reinterpret_cast<uintptr_t>(base) + sizeof(AllocationHeader) + pageSize - 1)
        & ~(static_cast<uintptr_t>(pageSize) - 1);
    auto* header = reinterpret_cast<AllocationHeader*>(alignedAddress) - 1;
    header->base = base;
    header->size = allocationSize;
    return reinterpret_cast<void*>(alignedAddress);
}

}

void* OSAllocator::reserveUncommitted(size_t bytes, Usage, bool, bool, bool)
{
    return allocatePages(bytes);
}

void* OSAllocator::reserveAndCommit(size_t bytes, Usage, bool, bool, bool)
{
    return allocatePages(bytes);
}

void OSAllocator::commit(void* address, size_t bytes, bool, bool)
{
    memset(address, 0, bytes);
}

void OSAllocator::decommit(void* address, size_t bytes)
{
    memset(address, 0, bytes);
}

void OSAllocator::releaseDecommitted(void* address, size_t)
{
    auto* header = static_cast<AllocationHeader*>(address) - 1;
    FreeMem(header->base, header->size);
}

void OSAllocator::hintMemoryNotNeededSoon(void*, size_t)
{
}

}
