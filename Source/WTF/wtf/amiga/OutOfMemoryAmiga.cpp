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
 * itself has memory to open. If even that is not enough, the screen title says so and the
 * allocation is retried every 2 seconds until memory frees up; OWB never stops silently.
 *
 * Keeping memory free: large allocations (64 KB and up, and new JavaScript heap regions) are
 * refused while free memory is below 3 MB (OWB_MIN_FREE_KB overrides it). JavaScript then
 * gets a normal "Out of memory" error, and the system keeps enough memory to work.
 *
 * Getting memory back: when free memory drops below a tenth of the total (at least 16 MB),
 * or a large allocation is refused, WebKit's low-memory release runs on the main thread
 * (prunes caches and the page cache, full garbage collection), at most every 10 seconds.
 * A 5-second watch checks for this, so a heavy page that was left behind is freed even if
 * the next page is small.
 *
 * Testing: set the environment variable OWB_SIMULATE_OOM to a number of megabytes, e.g.
 *     setenv OWB_SIMULATE_OOM 60
 * The first fastMalloc-family allocation after that many megabytes in total have been
 * allocated fails as if memory had run out, once. Choosing Retry ends the simulation and the
 * allocation then succeeds normally. Remove the variable with "unsetenv OWB_SIMULATE_OOM".
 */

#include "config.h"

// WTF headers before the AmigaOS ones: proto/dos.h defines a Lock() macro.
#include <wtf/MainThread.h>
#include <wtf/MemoryPressureHandler.h>
#include <wtf/RunLoop.h>

#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
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

// Below this much free memory the requester is unlikely to open.
constexpr ULONG requesterMinimumFree = 48 * 1024;

// Large allocations are refused while free memory is below this, so OWB never takes the very
// last memory: the requester can still open, JavaScript gets a normal "Out of memory" error,
// and the rest of the system keeps working. OWB_MIN_FREE_KB overrides it.
ULONG minimumFreeBytes = 3 * 1024 * 1024;

// Sets the screen title of OWB's window (if it is the active window and belongs to this
// task). Unlike a requester this needs no memory. `title` must stay valid.
void showInScreenTitle(const char* title)
{
    if (!IntuitionBase)
        return;
    struct Task* self = FindTask(nullptr);
    Forbid();
    struct Window* window = IntuitionBase->ActiveWindow;
    if (window && window->UserPort && window->UserPort->mp_SigTask == self)
        SetWindowTitles(window, (CONST_STRPTR)~0, (CONST_STRPTR)title);
    Permit();
}

// While stuck in the retry loop, say so in the screen title.
void showRetryingInTitle()
{
    showInScreenTitle("OWB: out of memory, retrying every 2 seconds. Quit other programs to continue.");
}

LONG elapsedTicks(const struct DateStamp& start)
{
    struct DateStamp now;
    DateStamp(&now);
    return (now.ds_Days - start.ds_Days) * 24L * 60L * TICKS_PER_SECOND * 60L
        + (now.ds_Minute - start.ds_Minute) * TICKS_PER_SECOND * 60L
        + (now.ds_Tick - start.ds_Tick);
}

// Returns 1 for Retry, 0 for Stop OWB, -1 if the requester could not be shown.
LONG askWithRequester(const char* message)
{
    if (!IntuitionBase || AvailMem(MEMF_ANY | MEMF_LARGEST) < requesterMinimumFree)
        return -1;

    // EasyRequest treats '%' in the text as a format character; the message has none.
    struct EasyStruct request;
    request.es_StructSize = sizeof(request);
    request.es_Flags = 0;
    request.es_Title = (CONST_STRPTR)"OWB: out of memory";
    request.es_TextFormat = (CONST_STRPTR)message;
    request.es_GadgetFormat = (CONST_STRPTR)"Retry|Stop OWB";

    struct DateStamp start;
    DateStamp(&start);
    LONG choice = EasyRequestArgs(nullptr, &request, nullptr, nullptr);
    // EasyRequestArgs also returns 0 when the requester could not open. Nobody clicks Stop
    // within a tenth of a second, so treat such a fast 0 as "could not be shown".
    if (choice == 0 && elapsedTicks(start) < TICKS_PER_SECOND / 10)
        return -1;
    return choice == 1 ? 1 : 0;
}

// When memory gets tight, ask WebKit to prune its caches and collect garbage, on the main
// thread and outside any running JavaScript. At most once every 10 seconds.
volatile bool releaseScheduled;
struct DateStamp lastRelease;
bool haveLastRelease;

void requestMemoryRelease()
{
    if (releaseScheduled)
        return;
    if (haveLastRelease && elapsedTicks(lastRelease) < 10 * TICKS_PER_SECOND)
        return;
    releaseScheduled = true;
    WTF::callOnMainThread([] {
        WTF::MemoryPressureHandler::singleton().releaseMemory(WTF::Critical::Yes, WTF::Synchronous::Yes);
        DateStamp(&lastRelease);
        haveLastRelease = true;
        releaseScheduled = false;
    });
}

// Every 5 seconds on the main thread: if free memory is getting low, release memory. This
// frees a page left behind (and WebKit's page cache) even when no allocation hits the limit.
// Release below 10% of total memory, but at least below 16 MB.
ULONG watchThresholdBytes = 16 * 1024 * 1024;

void memoryWatchTick()
{
    if (AvailMem(MEMF_ANY) < watchThresholdBytes)
        requestMemoryRelease();
    WTF::RunLoop::main().dispatchAfter(WTF::Seconds(5), memoryWatchTick);
}

} // namespace

extern "C" {

// Called once on the main thread after WebKit is set up.
void owbStartMemoryWatch(void)
{
    static bool started;
    if (started)
        return;
    started = true;
    ULONG tenth = AvailMem(MEMF_ANY | MEMF_TOTAL) / 10;
    if (tenth > watchThresholdBytes)
        watchThresholdBytes = tenth;
    WTF::RunLoop::main().dispatchAfter(WTF::Seconds(5), memoryWatchTick);
}

// Readable from the WinUAE debugger: number of out-of-memory events, the last requested size,
// how the last one was answered (1 Retry, 0 Stop, -1 requester could not open) and how often
// the requester could not open.
struct OwbOutOfMemoryRecord {
    unsigned long count;
    unsigned long lastSize;
    long lastChoice;
    unsigned long requesterFailures;
} owbOutOfMemoryRecord;

// Called once from main() before anything else allocates much.
void owbOutOfMemoryInit(void)
{
    if (initialized)
        return;
    InitSemaphore(&oomLock);
    initialized = true;
    restoreReserve();

    if (const char* value = getenv("OWB_MIN_FREE_KB")) {
        long kilobytes = atol(value);
        if (kilobytes >= 0)
            minimumFreeBytes = static_cast<ULONG>(kilobytes) * 1024UL;
    }

    if (const char* value = getenv("OWB_SIMULATE_OOM")) {
        long megabytes = atol(value);
        if (megabytes > 0)
            simulateLimitBytes = static_cast<unsigned long>(megabytes) * 1024UL * 1024UL;
    }
}

// True if allocating `size` more bytes would leave less than the minimum free memory. Only
// called for large allocations, because AvailMem() walks the free-memory list.
bool owbMemoryTooLow(size_t size)
{
    if (AvailMem(MEMF_ANY) >= size + minimumFreeBytes)
        return false;
    requestMemoryRelease();
    return true;
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

    owbOutOfMemoryRecord.count++;
    owbOutOfMemoryRecord.lastSize = size;

    LONG choice = askWithRequester(message);
    owbOutOfMemoryRecord.lastChoice = choice;
    if (choice < 0) {
        // Too little memory even for the requester. Never stop silently: flash the screen
        // once, wait a little and let the caller try again. As soon as anything frees memory
        // (the user quits another program, another task finishes), OWB carries on or the
        // requester can open.
        owbOutOfMemoryRecord.requesterFailures++;
        showRetryingInTitle();
        ReleaseSemaphore(&oomLock);
        Delay(2 * TICKS_PER_SECOND);
        restoreReserve();
        return true;
    }

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

// Frees the emergency reserve, e.g. so a fatal-error requester has room to open.
void owbOutOfMemoryReleaseReserve(void)
{
    releaseReserve();
}

// For C++ operator new (set with std::set_new_handler): returning makes new try again.
void owbOutOfMemoryNewHandler(void)
{
    owbOutOfMemory(0);
}

} // extern "C"
