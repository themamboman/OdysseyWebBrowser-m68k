#ifndef PROTO_DOS_H
#define PROTO_DOS_H

#include "dos/dos.h"

#define PROTO_DOS_H

/* Function prototypes - these are typically just the same as in dos.h */
#define Lock(name, accessMode) Lock(name, accessMode)
#define UnLock(lock) UnLock(lock)
#define DupLock(lock) DupLock(lock)
#define ParentDir(lock) ParentDir(lock)
#define Open(name, mode) Open(name, mode)
#define Close(file) Close(file)
#define Read(file, buffer, length) Read(file, buffer, length)
#define Write(file, buffer, length) Write(file, buffer, length)
#define Seek(file, position, mode) Seek(file, position, mode)
#define CreateDir(name) CreateDir(name)
#define DeleteFile(name) DeleteFile(name)
#define Rename(oldName, newName) Rename(oldName, newName)
#define Examine(lock, fib) Examine(lock, fib)
#define ExNext(lock, fib) ExNext(lock, fib)
#define CurrentDir(lock) CurrentDir(lock)
#define DupLockFromFH(fh) DupLockFromFH(fh)
#define Info(lock, info) Info(lock, info)
#define SetProtection(name, protect) SetProtection(name, protect)
#define DateStamp(ds) DateStamp(ds)
#define CompareDates(ds1, ds2) CompareDates(ds1, ds2)

#endif /* PROTO_DOS_H */