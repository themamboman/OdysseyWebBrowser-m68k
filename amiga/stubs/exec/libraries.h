#ifndef EXEC_LIBRARIES_H
#define EXEC_LIBRARIES_H

#include "exec/exec.h"

#define EXEC_LIBRARIES_H

/* Library is already defined in exec.h, functions are declared there */
/* Additional library management functions */
void SetFunction(struct Library* library, APTR func, LONG funcOffset);
APTR MakeLibrary(APTR vectors, ULONG structure, UBYTE flags, UBYTE pad);
void AddLibrary(struct Library* library);
struct Library* RemLibrary(struct Library* library);

#endif /* EXEC_LIBRARIES_H */