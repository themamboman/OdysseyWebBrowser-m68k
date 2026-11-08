#ifndef EXEC_SEMAPHORES_H
#define EXEC_SEMAPHORES_H

#include "exec/exec.h"

#define EXEC_SEMAPHORES_H

/* SignalSemaphore is already defined in exec.h, functions are declared there */
/* Additional semaphore functions */
void ObtainSemaphoreShared(struct SignalSemaphore* semaphore);
void ReleaseSemaphoreShared(struct SignalSemaphore* semaphore);
void ObtainSemaphoreList(struct SignalSemaphore** semaphoreArray, ULONG count);
void ReleaseSemaphoreList(struct SignalSemaphore** semaphoreArray, ULONG count);

typedef struct SemaphoreRequest {
    struct Node sr_Link;
    struct Task* sr_Waiter;
    struct SignalSemaphore* sr_Semaphore;
} SemaphoreRequest;

#endif /* EXEC_SEMAPHORES_H */