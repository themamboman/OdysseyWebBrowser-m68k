/*
 * ThreadingAmiga.cpp
 * AmigaOS 3.x / m68k threading implementation for WebKit WTF layer.
 * Maps the pthreads-style API onto AmigaOS Exec via the aos_multithreading library.
 */

#include "config.h"
#include <wtf/Threading.h>

#if OS(AMIGA)

#include <wtf/DataLog.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/RawPointer.h>
#include <wtf/StdLibExtras.h>
#include <wtf/ThreadGroup.h>
#include <wtf/ThreadingPrimitives.h>
#include <wtf/WordLock.h>

#include <exec/exec.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <string.h>
#include <limits>
#include <utility>

extern "C" {
#include "multitask_lib.h"
}

namespace WTF {

Thread::~Thread()
{
}

void Thread::initializePlatformThreading()
{
}

void Thread::initializeCurrentThreadEvenIfNonWTFCreated()
{
}

static void wtfThreadEntryPoint(void* context)
{
    Thread::entryPoint(reinterpret_cast<Thread::NewThreadContext*>(context));
}

bool Thread::establishHandle(NewThreadContext* context)
{
    /* Use AmigaOS Exec task via aos_multithreading library */
    struct MyThread* thread = MyThreadCreate(wtfThreadEntryPoint, context);
    if (!thread) {
        LOG_ERROR("Failed to create AmigaOS task at entry point %p with context %p", wtfThreadEntryPoint, context);
        return false;
    }
    establishPlatformSpecificHandle(thread);
    return true;
}


void Thread::initializeCurrentThreadInternal(const char* threadName)
{
    struct Task* self = FindTask(NULL);
    /* Every thread is created as "MyThread"; give the task WTF's name so
       crash requesters and task lists say which thread it is. DOS keeps the
       original name buffer on the task's MemList, so it is still freed. */
    if (threadName && *threadName) {
        size_t length = strlen(threadName) + 1;
        char* name = static_cast<char*>(AllocVec(length, MEMF_ANY));
        if (name) {
            memcpy(name, threadName, length);
            self->tc_Node.ln_Name = name; /* lives until exit; a few bytes per thread */
        }
    }
    initializeCurrentThreadEvenIfNonWTFCreated();
}

void Thread::changePriority(int delta)
{
    UNUSED_PARAM(delta);
}

int Thread::waitForCompletion()
{
    struct MyThread* thread;
    {
        auto locker = holdLock(m_mutex);
        thread = static_cast<struct MyThread*>(m_handle);
        m_handle = nullptr;
        if (thread)
            didJoin();
    }

    if (!thread)
        return 0;

    MyThreadWait(thread);
    MyThreadDestroy(thread);
    return 0;
}

void Thread::detach()
{
    struct MyThread* thread;
    {
        auto locker = holdLock(m_mutex);
        thread = static_cast<struct MyThread*>(m_handle);
        m_handle = nullptr;
        didBecomeDetached();
    }
    MyThreadDetach(thread);
}

Thread& Thread::initializeCurrentTLS()
{
    Ref<Thread> thread = adoptRef(*new Thread());
    /* A thread WTF didn't create (the main task) still needs its Exec task
       recorded, so the garbage collector can suspend it and scan its stack. */
    struct MyThread* handle = static_cast<struct MyThread*>(AllocMem(sizeof(struct MyThread), MEMF_CLEAR));
    if (handle) {
        handle->os_task = FindTask(NULL);
        handle->detached = TRUE;
        thread->establishPlatformSpecificHandle(handle);
    }
    thread->initializeInThread();
    return initializeTLS(WTFMove(thread));
}

/* JavaScriptCore's garbage collector runs on its own thread and must scan
   the stacks and registers of every thread that uses the VM (the main task
   above all). Previously suspend() always failed, so those stacks were
   silently skipped: objects referenced only from the main task's stack were
   freed while still in use, and the collector later called through their
   garbage method tables.

   Classic AmigaOS has one CPU, so Forbid() stops every other task until
   Permit(). A task that is not running had its registers pushed onto its
   own stack by Exec, with tc_SPReg pointing at them, so scanning from
   tc_SPReg to tc_SPUpper covers both its registers and its stack.
   JavaScriptCore does not allocate or wait between suspend() and resume(),
   so the Forbid() is not broken. */
auto Thread::suspend() -> Expected<void, PlatformSuspendError>
{
    struct MyThread* handle = static_cast<struct MyThread*>(m_handle);
    if (!handle || !handle->os_task)
        return makeUnexpected(1);
    Forbid();
    return { };
}

void Thread::resume()
{
    Permit();
}

size_t Thread::getRegisters(PlatformRegisters& registers)
{
    struct MyThread* handle = static_cast<struct MyThread*>(m_handle);
    registers.stackPointer = handle && handle->os_task ? handle->os_task->tc_SPReg : nullptr;
    return sizeof(registers);
}

void Thread::establishPlatformSpecificHandle(PlatformThreadHandle handle)
{
    auto locker = holdLock(m_mutex);
    m_handle = handle;
}

#if !HAVE(FAST_TLS)
void Thread::initializeTLSKey()
{
    threadSpecificKeyCreate(&s_key, destructTLS);
}
#endif

Thread& Thread::initializeTLS(Ref<Thread>&& thread)
{
    auto& threadInTLS = thread.leakRef();
#if !HAVE(FAST_TLS)
    ASSERT(s_key != InvalidThreadSpecificKey);
    threadSpecificSet(s_key, &threadInTLS);
#endif
    return threadInTLS;
}

void Thread::destructTLS(void* data)
{
    Thread* thread = static_cast<Thread*>(data);
    ASSERT(thread);

    if (thread->m_isDestroyedOnce) {
        thread->didExit();
        thread->deref();
        return;
    }

    thread->m_isDestroyedOnce = true;
#if !HAVE(FAST_TLS)
    ASSERT(s_key != InvalidThreadSpecificKey);
    threadSpecificSet(s_key, thread);
#endif
}

Mutex::~Mutex()
{
    MyMutexDestroy(&m_mutex);
}

Mutex::Mutex()
{
    MyMutexInit(&m_mutex);
}

void Mutex::lock()
{
    /* Use SignalSemaphore via MyMutex */
    struct MyMutex* m = static_cast<struct MyMutex*>(&m_mutex);
    MyMutexLock(m);
}

bool Mutex::tryLock()
{
    return AttemptSemaphore(&m_mutex.sem);
}

void Mutex::unlock()
{
    struct MyMutex* m = static_cast<struct MyMutex*>(&m_mutex);
    MyMutexUnlock(m);
}

ThreadCondition::ThreadCondition()
{
    MyConditionInit(&m_condition);
}

ThreadCondition::~ThreadCondition()
{
    MyConditionDestroy(&m_condition);
}

void ThreadCondition::wait(Mutex& mutex)
{
    MyConditionWait(&mutex.impl(), &m_condition);
}

bool ThreadCondition::timedWait(Mutex& mutex, WallTime absoluteTime)
{
    auto remainingTime = absoluteTime - WallTime::now();
    auto timeoutMilliseconds = remainingTime.milliseconds();
    if (timeoutMilliseconds <= 0)
        return false;
    if (timeoutMilliseconds > std::numeric_limits<LONG>::max()) {
        wait(mutex);
        return true;
    }
    return MyConditionTimedWait(&mutex.impl(), &m_condition, static_cast<ULONG>(timeoutMilliseconds));
}

void ThreadCondition::signal()
{
    MyConditionSignal(&m_condition);
}

void ThreadCondition::broadcast()
{
    MyConditionBroadcast(&m_condition);
}

void Thread::yield()
{
    MyThreadYield();
}

} // namespace WTF

#endif // OS(AMIGA)
