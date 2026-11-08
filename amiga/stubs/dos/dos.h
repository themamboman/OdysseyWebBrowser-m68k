#ifndef DOS_DOS_H
#define DOS_DOS_H

#include <stdint.h>
#include "exec/exec.h"

#define DOS_DOS_H

/* DOS types */
typedef int32_t BPTR;
#define BNULL ((BPTR)0)
#define BADDR(x) ((BPTR)(x))
#define MKBADDR(x) ((BPTR)(x))

typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int16_t WORD;
typedef uint16_t UWORD;
typedef int8_t BYTE;
typedef uint8_t UBYTE;

typedef struct FileLock {
    struct Node fl_Link;
    LONG fl_Key;
    LONG fl_Access;
    struct FileLock* fl_Parent;
    LONG fl_Volume;
} FileLock;

typedef struct FileHandle {
    struct Node fh_Link;
    LONG fh_Type;
    LONG fh_End;
    LONG fh_Pos;
    LONG fh_Funcs;
    LONG fh_Func2;
    LONG fh_Func3;
    LONG fh_Args;
    LONG fh_Buf;
    LONG fh_BufSize;
    LONG fh_Pos2;
    LONG fh_End2;
    LONG fh_Funcs2;
    LONG fh_Func2_2;
    LONG fh_Func3_2;
} FileHandle;

typedef struct DosInfo {
    struct Node di_Link;
    LONG di_DevInfo;
    LONG di_RootLock;
    LONG di_DevList;
    LONG di_DosType;
    LONG di_Flags;
    LONG di_SoftBack;
} DosInfo;

typedef struct DateStamp {
    LONG ds_Days;
    LONG ds_Minute;
    LONG ds_Tick;
} DateStamp;

typedef struct InfoData {
    LONG id_NumSoftErrors;
    LONG id_Units;
    LONG id_SoftErrors;
    LONG id_FreeBlocks;
    LONG id_DiskBlocks;
    LONG id_DiskType;
    LONG id_VolumeNode;
    LONG id_InUse;
} InfoData;

/* Access modes */
#define ACCESS_READ  -2
#define ACCESS_WRITE -1

/* File modes */
#define MODE_OLDFILE 1000
#define MODE_NEWFILE 1001
#define MODE_READWRITE 1002

/* Seek modes */
#define OFFSET_BEGINNING 0
#define OFFSET_CURRENT   1
#define OFFSET_END       2

/* Lock modes */
#define SHARED_LOCK     -2
#define EXCLUSIVE_LOCK  -1

/* Function prototypes */
BPTR Lock(const char* name, LONG accessMode);
void UnLock(BPTR lock);
BPTR DupLock(BPTR lock);
BPTR ParentDir(BPTR lock);

BPTR Open(const char* name, LONG mode);
LONG Close(BPTR file);
LONG Read(BPTR file, APTR buffer, LONG length);
LONG Write(BPTR file, APTR buffer, LONG length);
LONG Seek(BPTR file, LONG position, LONG mode);

BPTR CreateDir(const char* name);
BOOL DeleteFile(const char* name);
BOOL Rename(const char* oldName, const char* newName);

LONG Examine(BPTR lock, struct FileInfoBlock* fib);
LONG ExNext(BPTR lock, struct FileInfoBlock* fib);

BPTR CurrentDir(BPTR lock);
BPTR DupLockFromFH(BPTR fileHandle);

LONG Info(BPTR lock, struct InfoData* parameterBlock);
LONG SetProtection(const char* name, LONG protect);

BPTR CreateProc(char* name, LONG pri, BPTR segList, ULONG stackSize);
void WaitForChar(BPTR file, LONG timeout);

struct Process* FindTask(APTR task);
struct Process* CreateNewProcTags(ULONG tag1, ...);

void DateStamp(struct DateStamp* ds);
LONG CompareDates(struct DateStamp* ds1, struct DateStamp* ds2);

typedef struct FileInfoBlock {
    LONG fib_DiskKey;
    LONG fib_DirEntryType;
    char fib_FileName[256];
    LONG fib_Protection;
    LONG fib_EntryType;
    LONG fib_Size;
    LONG fib_NumBlocks;
    struct DateStamp fib_Date;
    char fib_Comment[80];
    char fib_Reserved[36];
} FileInfoBlock;

#define ST_USERDIR 0
#define ST_ROOT    1
#define ST_PIPE    2
#define ST_USER    3
#define ST_LINK    4

#define FIBF_DELETE    0
#define FIBF_READ      1
#define FIBF_WRITE     2
#define FIBF_EXECUTE   3
#define FIBF_ARCHIVE   4
#define FIBF_PURE      5
#define FIBF_SCRIPT    6
#define FIBF_HIDDEN    7

#endif /* DOS_DOS_H */