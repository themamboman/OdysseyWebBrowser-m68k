/*
 * Out-of-memory handling for AmigaOS 3.x.
 *
 * WebKit assumes fastMalloc() and friends never return NULL. On AmigaOS 3.x they did, and
 * callers then built objects at address 0, overwriting ExecBase and the CPU vectors: a
 * system-wide Guru or a WinUAE HALT3. Every allocator that must not fail now calls
 * owbOutOfMemory() instead, which shows a requester:
 *
 *   Retry     the user frees memory elsewhere and the allocation is tried again
 *   Stop OWB  this task stops for good, without touching anything else. If it is the main
 *             task, the OWB window stops responding; memory stays in use until a reboot.
 *
 * A small emergency reserve is held from start-up and released first, so the requester
 * itself has memory to open.
 *
 * Testing: set the environment variable OWB_SIMULATE_OOM to a number of megabytes, e.g.
 *     setenv OWB_SIMULATE_OOM 60
 * The first fastMalloc-family allocation after that many megabytes in total have been
 * allocated fails as if memory had run out, once. Choosing Retry ends the simulation and the
 * allocation then succeeds normally. Remove the variable with "unsetenv OWB_SIMULATE_OOM".
 */

#include "config.h"

#include <exec/memory.h>
#include <exec/semaphores.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <stdio.h>
#include <stdlib.h>

namespace {

constexpr ULONG reserveSize = 256 * 1024;

struct SignalSemaphore oomLock;
bool initialized;
APTR reserve;

unsigned long simulateLimitBytes;   // 0 = simulation off
unsigned long simulateAllocatedBytes;
bool simulateFired;
bool simulatePending;               // the current failure is a simulated one

void releaseReserve()
{
    if (reserve) {
        FreeMem(reserve, reserveSize);
        reserve = nullptr;
    }
}

void restoreReserve()
{
    if (!reserve)
        reserve = AllocMem(reserveSize, MEMF_ANY);
}

} // namespace

extern "C" {

// Called once from main() before anything else allocates much.
void owbOutOfMemoryInit(void)
{
    if (initialized)
        return;
    InitSemaphore(&oomLock);
    initialized = true;
    restoreReserve();

    if (const char* value = getenv("OWB_SIMULATE_OOM")) {
        long megabytes = atol(value);
        if (megabytes > 0)
            simulateLimitBytes = static_cast<unsigned long>(megabytes) * 1024UL * 1024UL;
    }
}

// True if this allocation should fail because of OWB_SIMULATE_OOM. Fires once per run.
bool owbSimulateOutOfMemory(size_t size)
{
    if (!simulateLimitBytes || simulateFired)
        return false;
    simulateAllocatedBytes += size;
    if (simulateAllocatedBytes < simulateLimitBytes)
        return false;
    simulateFired = true;
    simulatePending = true;
    return true;
}

// An allocation of `size` bytes failed (0 = size unknown). Returns true if the caller should
// try again. Never returns false: "Stop OWB" parks the calling task.
bool owbOutOfMemory(size_t size)
{
    if (!initialized)
        owbOutOfMemoryInit();

    ObtainSemaphore(&oomLock);
    releaseReserve();

    bool simulated = simulatePending;
    simulatePending = false;

    char sizeText[48];
    if (size)
        snprintf(sizeText, sizeof(sizeText), "%lu bytes", static_cast<unsigned long>(size));
    else
        snprintf(sizeText, sizeof(sizeText), "a block");

    char message[640];
    snprintf(message, sizeof(message),
        "OWB could not get %s of memory%s.\n"
        "Free memory now: %lu KB (largest block %lu KB).\n"
        "\n"
        "Retry: first free memory, for example by quitting other\n"
        "programs, then OWB tries again.\n"
        "\n"
        "Stop OWB: OWB stops without crashing the rest of the system.\n"
        "Its window stops responding, and its memory stays in use\n"
        "until you reboot.",
        sizeText, simulated ? " (simulated by OWB_SIMULATE_OOM)" : "",
        AvailMem(MEMF_ANY) / 1024UL, AvailMem(MEMF_ANY | MEMF_LARGEST) / 1024UL);

    // EasyRequest treats '%' in the text as a format character; the message has none.
    struct EasyStruct request;
    request.es_StructSize = sizeof(request);
    request.es_Flags = 0;
    request.es_Title = (CONST_STRPTR)"OWB: out of memory";
    request.es_TextFormat = (CONST_STRPTR)message;
    request.es_GadgetFormat = (CONST_STRPTR)"Retry|Stop OWB";

    LONG choice = IntuitionBase ? EasyRequestArgs(nullptr, &request, nullptr, nullptr) : 0;

    if (choice == 1) {
        restoreReserve();
        ReleaseSemaphore(&oomLock);
        return true;
    }

    // Stop OWB. Keep holding the lock, so any other OWB task that runs out of memory stops
    // here too instead of crashing.
    Wait(0);
    return false;
}

// For C++ operator new (set with std::set_new_handler): returning makes new try again.
void owbOutOfMemoryNewHandler(void)
{
    owbOutOfMemory(0);
}

} // extern "C"
