/*
 * Minimal asyncio.library compat shim -- see proto/asyncio.h for why this exists.
 * Backed directly by synchronous dos.library calls.
 */
#include <proto/asyncio.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/memory.h>
#include <dos/dos.h>

struct AsyncFile {
    BPTR handle;
};

struct AsyncFile *OpenAsync(CONST_STRPTR name, LONG mode, LONG bufferSize)
{
    LONG dosMode;
    BPTR handle;
    struct AsyncFile *file;

    (void)bufferSize; /* no real buffering in this synchronous shim */

    switch (mode) {
    case MODE_READ:
        dosMode = MODE_OLDFILE;
        break;
    case MODE_WRITE:
        dosMode = MODE_NEWFILE;
        break;
    case MODE_APPEND:
        /* MODE_READWRITE opens an existing file for read+write, or creates a new one if it
         * doesn't exist yet -- exactly the "open or create" semantics callers of MODE_APPEND
         * expect (they seek explicitly afterward, to the start or end as needed). */
        dosMode = MODE_READWRITE;
        break;
    default:
        return NULL;
    }

    handle = Open(name, dosMode);
    if (!handle)
        return NULL;

    file = (struct AsyncFile *)AllocVec(sizeof(struct AsyncFile), MEMF_PUBLIC | MEMF_CLEAR);
    if (!file) {
        Close(handle);
        return NULL;
    }
    file->handle = handle;
    return file;
}

void CloseAsync(struct AsyncFile *file)
{
    if (!file)
        return;
    Close(file->handle);
    FreeVec(file);
}

LONG ReadAsync(struct AsyncFile *file, APTR buffer, LONG size)
{
    if (!file)
        return -1;
    return Read(file->handle, buffer, size);
}

LONG WriteAsync(struct AsyncFile *file, APTR buffer, LONG size)
{
    if (!file)
        return -1;
    return Write(file->handle, buffer, size);
}

LONG SeekAsync(struct AsyncFile *file, LONG position, LONG mode)
{
    LONG dosMode;

    if (!file)
        return -1;

    switch (mode) {
    case MODE_START:
        dosMode = OFFSET_BEGINNING;
        break;
    case MODE_CURRENT:
        dosMode = OFFSET_CURRENT;
        break;
    case MODE_END:
        dosMode = OFFSET_END;
        break;
    default:
        return -1;
    }

    return Seek(file->handle, position, dosMode);
}
