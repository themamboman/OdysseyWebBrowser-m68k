/*
 * Copyright (C) 2013 Apple Inc.  All rights reserved.
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * Copyright (C) 2017 NAVER Corp.
 *
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
#include "CurlRequestScheduler.h"

#if USE(CURL)

#include "CurlRequestSchedulerClient.h"

#if PLATFORM(MUI)
#include <proto/exec.h>
#include <proto/dos.h>
// <unistd.h> must come before <proto/bsdsocket.h>: the latter #defines getdtablesize() as a
// zero-arg function-like macro (an inline stub calling the library function), and if it's already
// in effect when <unistd.h>'s own `int getdtablesize(void);` declaration is parsed, the literal
// token `void` inside the parens is mistaken for a macro argument.
#include <unistd.h>
/* AmiSSL's libamisslauto.a (autoinit_assl.o) also defines a plain global
 * named SocketBase for its own internal bsdsocket.library use. Since C
 * gives external-linkage globals of the same name the same storage, that
 * collided with this file's own SocketBase -- confirmed via nm: both
 * objects define `D _SocketBase` at link time. Not cosmetic: whichever
 * side's CloseLibrary()+NULL-out runs "last" would silently invalidate the
 * other side's handle even though its own OpenLibrary() reference is still
 * live, since they're actually the same variable. Renaming this file's own
 * copy (and retargeting bsdsocket.h's inline call-macros at it via
 * BSDSOCKET_BASE_NAME, so this file's own socket calls keep using the
 * right base) makes the two fully independent, which is what was already
 * intended -- two separate OpenLibrary/CloseLibrary pairs on a reference-
 * counted system library is normal and fine; only the accidental shared
 * storage was the bug. */
#define BSDSOCKET_BASE_NAME WebCoreCurlSocketBase
#include <proto/bsdsocket.h>
#include <bsdsocket/socketbasetags.h>
#undef send
struct Library *WebCoreCurlSocketBase;
/* libcurl makes every socket call through the global SocketBase (defined by
 * libamisslauto and opened by the main task). A bsdsocket.library base must
 * only be used by the task that opened it, so while the curl thread runs,
 * SocketBase is pointed at the thread's own base and restored afterwards. */
static struct Library *savedSocketBase;

/* Likewise for AmiSSL: libamisslauto opens it once from the main task, bound to
 * the main task's SocketBase, and curl's TLS calls go through the global
 * AmiSSLBase/AmiSSLExtBase. AmiSSL bases are per task too, so the curl thread
 * opens its own (bound to its own SocketBase) and swaps the globals while it runs. */
extern "C" {
extern struct Library *AmiSSLMasterBase;
extern LONG AmiSSLAPIVersion, UsesOpenSSLStructs;
LONG OpenAmiSSLTags(LONG APIVersion, ULONG tag, ...);
}
#define OWB_AmiSSL_SocketBase (TAG_USER + 0x01)
#define OWB_AmiSSL_ErrNoPtr (TAG_USER + 0x0b)
#define OWB_AmiSSL_UsesOpenSSLStructs (TAG_USER + 0x0c)
#define OWB_AmiSSL_GetAmiSSLBase (TAG_USER + 0x0d)
#define OWB_AmiSSL_GetAmiSSLExtBase (TAG_USER + 0x0e)
static struct Library *savedAmiSSLMasterBase, *savedAmiSSLBase, *savedAmiSSLExtBase;
static struct Library *threadAmiSSLMasterBase;
static bool threadAmiSSLOpen;

static void closeAmiSSLWith(struct Library *masterBase)
{
    register struct Library *a6 asm("a6") = masterBase;
    asm volatile("jsr -42(%%a6)" : : "r"(a6) : "d0", "d1", "a0", "a1", "memory", "cc"); /* CloseAmiSSL() */
}

static void init_AmiSSLForThread()
{
    savedAmiSSLMasterBase = AmiSSLMasterBase;
    savedAmiSSLBase = AmiSSLBase;
    savedAmiSSLExtBase = AmiSSLExtBase;
    threadAmiSSLOpen = false;

    threadAmiSSLMasterBase = OpenLibrary("amisslmaster.library", 5);
    if (!threadAmiSSLMasterBase)
        return;

    struct Library *base = nullptr, *extBase = nullptr;
    AmiSSLMasterBase = threadAmiSSLMasterBase;
    if (!OpenAmiSSLTags(AmiSSLAPIVersion,
        OWB_AmiSSL_UsesOpenSSLStructs, (ULONG)UsesOpenSSLStructs,
        OWB_AmiSSL_GetAmiSSLBase, (ULONG)&base,
        OWB_AmiSSL_GetAmiSSLExtBase, (ULONG)&extBase,
        OWB_AmiSSL_SocketBase, (ULONG)WebCoreCurlSocketBase,
        OWB_AmiSSL_ErrNoPtr, (ULONG)&errno,
        TAG_DONE) && base) {
        AmiSSLBase = base;
        AmiSSLExtBase = extBase;
        threadAmiSSLOpen = true;
        return;
    }

    AmiSSLMasterBase = savedAmiSSLMasterBase;
    CloseLibrary(threadAmiSSLMasterBase);
    threadAmiSSLMasterBase = nullptr;
}

static void close_AmiSSLForThread()
{
    if (threadAmiSSLOpen)
        closeAmiSSLWith(threadAmiSSLMasterBase);
    if (threadAmiSSLMasterBase)
        CloseLibrary(threadAmiSSLMasterBase);
    threadAmiSSLMasterBase = nullptr;
    threadAmiSSLOpen = false;
    AmiSSLMasterBase = savedAmiSSLMasterBase;
    AmiSSLBase = savedAmiSSLBase;
    AmiSSLExtBase = savedAmiSSLExtBase;
}

/* AmiSSL library bases are per task. The curl thread opens its own and swaps
   them into the global AmiSSLBase/AmiSSLExtBase/AmiSSLMasterBase that every
   AmiSSL call goes through. Other tasks also use AmiSSL (WTF::RandomDevice's
   RAND_bytes() for JavaScript, PAL's SHA digests), and while the curl thread
   ran they went through the curl thread's bases: two tasks inside one task's
   AmiSSL state at once, corrupting the curl thread (crashes on JavaScript-heavy
   pages). Now AmiSSL use is serialised by owbAmiSSLLock: the curl thread holds
   it while it works and releases it while it sleeps, and any other task takes
   it and swaps in the main task's own bases for the duration of its call. */
static struct SignalSemaphore owbAmiSSLLock;
static struct Task* owbAmiSSLCurlTask;
static int owbAmiSSLForeignDepth;
static struct Library* owbAmiSSLSwappedMaster;
static struct Library* owbAmiSSLSwappedBase;
static struct Library* owbAmiSSLSwappedExt;
extern "C" void (*owbAmiSSLUseHook)(int begin);

static void owbAmiSSLUse(int begin)
{
    bool foreign = FindTask(NULL) != owbAmiSSLCurlTask;
    if (begin) {
        ObtainSemaphore(&owbAmiSSLLock);
        if (foreign && owbAmiSSLForeignDepth++ == 0 && threadAmiSSLOpen) {
            owbAmiSSLSwappedMaster = AmiSSLMasterBase;
            owbAmiSSLSwappedBase = AmiSSLBase;
            owbAmiSSLSwappedExt = AmiSSLExtBase;
            AmiSSLMasterBase = savedAmiSSLMasterBase;
            AmiSSLBase = savedAmiSSLBase;
            AmiSSLExtBase = savedAmiSSLExtBase;
        }
    } else {
        if (foreign && --owbAmiSSLForeignDepth == 0 && owbAmiSSLSwappedBase) {
            AmiSSLMasterBase = owbAmiSSLSwappedMaster;
            AmiSSLBase = owbAmiSSLSwappedBase;
            AmiSSLExtBase = owbAmiSSLSwappedExt;
            owbAmiSSLSwappedBase = nullptr;
        }
        ReleaseSemaphore(&owbAmiSSLLock);
    }
}

static void owbAmiSSLLockInstall()
{
    if (owbAmiSSLUseHook)
        return;
    InitSemaphore(&owbAmiSSLLock);
    owbAmiSSLUseHook = owbAmiSSLUse;
}

void init_SocketBase()
{
    WebCoreCurlSocketBase = OpenLibrary("bsdsocket.library", 4L);
    savedSocketBase = SocketBase;
    SocketBase = WebCoreCurlSocketBase;
    SocketBaseTags(
        SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(errno))), (IPTR) &errno,
        SBTM_SETVAL(SBTC_LOGTAGPTR),       (IPTR) "cURL",
        TAG_DONE);
    init_AmiSSLForThread();
}
void close_SocketBase()
{
    close_AmiSSLForThread();
    SocketBase = savedSocketBase;
    CloseLibrary(WebCoreCurlSocketBase);
    WebCoreCurlSocketBase = NULL;
}
#endif

namespace WebCore {

#if PLATFORM(MUI)
/* The curl thread is kept running until the browser quits; main() must stop it
 * (owbStopCurlThreadAtExit) before OWB's code is unloaded, or the still-running
 * thread process executes freed memory. */
static CurlRequestScheduler* s_curlScheduler;
#endif

CurlRequestScheduler::CurlRequestScheduler(long maxConnects, long maxTotalConnections, long maxHostConnections)
    : m_maxConnects(maxConnects)
    , m_maxTotalConnections(maxTotalConnections)
    , m_maxHostConnections(maxHostConnections)
{
#if PLATFORM(MUI)
    s_curlScheduler = this;
    owbAmiSSLLockInstall(); /* on the main task, before the curl thread exists */
#endif
}

bool CurlRequestScheduler::add(CurlRequestSchedulerClient* client)
{
    ASSERT(isMainThread());

    if (!client)
        return false;

    startTransfer(client);
    startThreadIfNeeded();

    return true;
}

void CurlRequestScheduler::cancel(CurlRequestSchedulerClient* client)
{
    ASSERT(isMainThread());

    if (!client)
        return;

    cancelTransfer(client);
}

void CurlRequestScheduler::callOnWorkerThread(WTF::Function<void()>&& task)
{
    {
        auto locker = holdLock(m_mutex);
        m_taskQueue.append(WTFMove(task));
    }

    startThreadIfNeeded();
}

void CurlRequestScheduler::startThreadIfNeeded()
{
    ASSERT(isMainThread());

    {
        auto locker = holdLock(m_mutex);
        if (m_runThread)
            return;
    }

    if (m_thread)
        m_thread->waitForCompletion();

    {
        auto locker = holdLock(m_mutex);
        m_runThread = true;
    }

    m_thread = Thread::create("curlThread", [this] {
#if PLATFORM(MUI)
        ObtainSemaphore(&owbAmiSSLLock);
        owbAmiSSLCurlTask = FindTask(NULL);
        init_SocketBase();
        ReleaseSemaphore(&owbAmiSSLLock);
        /* Increase priority so that network data is transported immediatelly */
        SetTaskPri(FindTask(NULL), 1);
#endif
        workerThread();

        auto locker = holdLock(m_mutex);
        m_runThread = false;
#if PLATFORM(MUI)
        ObtainSemaphore(&owbAmiSSLLock);
        close_SocketBase();
        owbAmiSSLCurlTask = nullptr;
        ReleaseSemaphore(&owbAmiSSLLock);
#endif
    });
}

void CurlRequestScheduler::stopThreadIfNoMoreJobRunning()
{
    ASSERT(!isMainThread());

#if !PLATFORM(MUI)
    /* Keep the original curlThread running until browser quits */
    auto locker = holdLock(m_mutex);
    if (m_activeJobs.size() || m_taskQueue.size())
        return;

    m_runThread = false;
#endif
}

#if PLATFORM(MUI)
void CurlRequestScheduler::stopCurlThread()
{
    stopThread();
}

} // namespace WebCore

void owbStopCurlThreadAtExit()
{
    if (WebCore::s_curlScheduler)
        WebCore::s_curlScheduler->stopCurlThread();
}

namespace WebCore {

CurlStreamID CurlRequestScheduler::createStream(const URL& url, CurlStream::Client& client)
{
    ASSERT(isMainThread());

    do {
        m_currentStreamID = (m_currentStreamID + 1 != invalidCurlStreamID) ? m_currentStreamID + 1 : 1;
    } while (m_clientList.contains(m_currentStreamID));

    auto streamID = m_currentStreamID;
    m_clientList.add(streamID, &client);

    callOnWorkerThread([this, streamID, url = url.isolatedCopy()]() mutable {
        m_streamList.add(streamID, CurlStream::create(*this, streamID, WTFMove(url)));
    });

    return streamID;
}

void CurlRequestScheduler::destroyStream(CurlStreamID streamID)
{
    ASSERT(isMainThread());

    if (m_clientList.contains(streamID))
        m_clientList.remove(streamID);

    callOnWorkerThread([this, streamID]() {
        if (m_streamList.contains(streamID))
            m_streamList.remove(streamID);
    });
}

void CurlRequestScheduler::send(CurlStreamID streamID, UniqueArray<uint8_t>&& data, size_t length)
{
    ASSERT(isMainThread());

    callOnWorkerThread([this, streamID, data = WTFMove(data), length]() mutable {
        if (auto stream = m_streamList.get(streamID))
            stream->send(WTFMove(data), length);
    });
}

void CurlRequestScheduler::callClientOnMainThread(CurlStreamID streamID, WTF::Function<void(CurlStream::Client&)>&& task)
{
    ASSERT(!isMainThread());

    callOnMainThread([this, streamID, task = WTFMove(task)]() {
        if (auto client = m_clientList.get(streamID))
            task(*client);
    });
}
#endif

void CurlRequestScheduler::stopThread()
{
    {
        auto locker = holdLock(m_mutex);
        m_runThread = false;
    }

    if (m_thread) {
        m_thread->waitForCompletion();
        m_thread = nullptr;
    }
}

void CurlRequestScheduler::executeTasks()
{
    ASSERT(!isMainThread());

    Vector<WTF::Function<void()>> taskQueue;

    {
        auto locker = holdLock(m_mutex);
        taskQueue = WTFMove(m_taskQueue);
    }

    for (auto& task : taskQueue)
        task();
}

} // namespace WebCore

/* Makes the curl thread's bsdsocket calls 64 KB further down its 2 MB stack. Kept because
   the verified builds ran this way; whether it is strictly needed has not been established. */
static LONG __attribute__((noinline)) owbDeepWaitSelect(LONG nfds, fd_set* readSet, fd_set* writeSet, fd_set* exceptSet)
{
    volatile char pad[64 * 1024];
    pad[0] = 0;
    struct timeval zero = { 0, 0 };
    return WaitSelect(nfds, readSet, writeSet, exceptSet, &zero, nullptr);
}

namespace WebCore {

void CurlRequestScheduler::workerThread()
{
    ASSERT(!isMainThread());

    m_curlMultiHandle = std::make_unique<CurlMultiHandle>();
    m_curlMultiHandle->setMaxConnects(m_maxConnects);
    m_curlMultiHandle->setMaxTotalConnections(m_maxTotalConnections);
    m_curlMultiHandle->setMaxHostConnections(m_maxHostConnections);

    while (true) {
        {
            auto locker = holdLock(m_mutex);
            if (!m_runThread)
                break;
        }

        ObtainSemaphore(&owbAmiSSLLock); /* working: AmiSSL is ours */
        executeTasks();
        ReleaseSemaphore(&owbAmiSSLLock); /* about to sleep */

        // Retry 'select' if it was interrupted by a process signal.
        int rc = 0;
#if PLATFORM(MUI)
        fd_set fdread;
        fd_set fdwrite;
        fd_set fdexcep;
#endif
        do {
#if PLATFORM(MUI)
            FD_ZERO(&fdread);
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdexcep);
            int maxfd = 0;

            // Wait as long as curl allows (socket activity wakes WaitSelect early anyway),
            // capped at 50ms so newly queued tasks are picked up promptly. A fixed 500us
            // timeout made this thread (priority 1) poll ~2000 times/s and starve the
            // main task on 68k.
            long waitMs = m_curlMultiHandle->timeoutMilliseconds();
            if (waitMs < 0 || waitMs > 50)
                waitMs = 50;
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = waitMs * 1000;
#else
            fd_set fdread;
            fd_set fdwrite;
            fd_set fdexcep;
            int maxfd = 0;

            const int selectTimeoutMS = 5;

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = selectTimeoutMS * 1000; // select waits microseconds
#endif

            m_curlMultiHandle->getFdSet(fdread, fdwrite, fdexcep, maxfd);
#if PLATFORM(MUI)
            for (auto& stream : m_streamList.values())
                stream->appendMonitoringFd(fdread, fdwrite, fdexcep, maxfd);
#endif

            // When the 3 file descriptors are empty, winsock will return -1
            // and bail out, stopping the file download. So make sure we
            // have valid file descriptors before calling select.
            if (maxfd >= 0)
#if PLATFORM(MUI)
            {
                /* Poll with a zero timeout and do the waiting here. This is the path every
                   verified build used; a blocking WaitSelect with a timeout is untested. */
                fd_set readSet = fdread, writeSet = fdwrite, exceptSet = fdexcep;
                long waitedMs = 0;
                while (true) {
                    fdread = readSet;
                    fdwrite = writeSet;
                    fdexcep = exceptSet;
                    rc = owbDeepWaitSelect(maxfd + 1, &fdread, &fdwrite, &fdexcep);
                    if (rc != 0 || waitedMs >= waitMs)
                        break;
                    Delay(1); /* 20 ms */
                    waitedMs += 20;
                }
            }
            else {
                // usleep() isn't implemented by this target's libc. Delay() is dos.library's
                // native equivalent; ticks are 1/50 sec, so 5 ticks ~= the 100ms (100 * 1000us)
                // this was waiting for.
                owbDeepWaitSelect(0, nullptr, nullptr, nullptr);
                Delay(5);
            }
#else
                rc = ::select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
#endif
        } while (rc == -1 && errno == EINTR);

        int activeCount = 0;
        ObtainSemaphore(&owbAmiSSLLock);
        while (m_curlMultiHandle->perform(activeCount) == CURLM_CALL_MULTI_PERFORM) { }

        // check the curl messages indicating completed transfers
        // and free their resources
        while (true) {
            int messagesInQueue = 0;
            CURLMsg* msg = m_curlMultiHandle->readInfo(messagesInQueue);
            if (!msg)
                break;

            ASSERT(msg->msg == CURLMSG_DONE);
            if (auto client = m_clientMaps.inlineGet(msg->easy_handle))
                completeTransfer(client, msg->data.result);
        }

#if PLATFORM(MUI)
        for (auto& stream : m_streamList.values())
            stream->tryToTransfer(fdread, fdwrite, fdexcep);
#endif

        stopThreadIfNoMoreJobRunning();
        ReleaseSemaphore(&owbAmiSSLLock);
    }

    m_curlMultiHandle = nullptr;
}

void CurlRequestScheduler::startTransfer(CurlRequestSchedulerClient* client)
{
    client->retain();

    auto task = [this, client]() {
        CURL* handle = client->setupTransfer();
        if (!handle) {
            completeTransfer(client, CURLE_FAILED_INIT);
            return;
        }

        m_curlMultiHandle->addHandle(handle);

        ASSERT(!m_clientMaps.contains(handle));
        m_clientMaps.set(handle, client);
    };

    auto locker = holdLock(m_mutex);
    m_activeJobs.add(client);
    m_taskQueue.append(WTFMove(task));
}

void CurlRequestScheduler::completeTransfer(CurlRequestSchedulerClient* client, CURLcode result)
{
    finalizeTransfer(client, [client, result]() {
        client->didCompleteTransfer(result);
    });
}

void CurlRequestScheduler::cancelTransfer(CurlRequestSchedulerClient* client)
{
    finalizeTransfer(client, [client]() {
        client->didCancelTransfer();
    });
}

void CurlRequestScheduler::finalizeTransfer(CurlRequestSchedulerClient* client, Function<void()> completionHandler)
{
    auto locker = holdLock(m_mutex);

    if (!m_activeJobs.contains(client))
        return;

    m_activeJobs.remove(client);

    auto task = [this, client, completionHandler = WTFMove(completionHandler)]() {
        if (client->handle()) {
            ASSERT(m_clientMaps.contains(client->handle()));
            m_clientMaps.remove(client->handle());
            m_curlMultiHandle->removeHandle(client->handle());
        }

        completionHandler();

        callOnMainThread([client]() {
            client->release();
        });
    };

    m_taskQueue.append(WTFMove(task));
}

}

#endif
