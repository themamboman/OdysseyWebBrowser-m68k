#ifndef EXEC_EXEC_H
#define EXEC_EXEC_H

#include <stdint.h>
#include <stddef.h>

#define EXEC_EXEC_H

/* Basic types */
typedef int8_t   BYTE;
typedef uint8_t  UBYTE;
typedef int16_t  WORD;
typedef uint16_t UWORD;
typedef int32_t  LONG;
typedef uint32_t ULONG;
typedef int64_t  QUAD;
typedef uint64_t UQUAD;

typedef int      BOOL;
#define TRUE  1
#define FALSE 0

typedef void*    APTR;
typedef const void* CONST_APTR;
typedef APTR*    APTR_PTR;

struct MinNode {
    struct MinNode* mln_Succ;
    struct MinNode* mln_Pred;
};

struct MinList {
    struct MinNode* mlh_Head;
    struct MinNode* mlh_Tail;
    struct MinNode* mlh_TailPred;
    UBYTE mlh_Type;
    UBYTE mlh_pad;
};

typedef struct Node {
    struct Node* ln_Succ;
    struct Node* ln_Pred;
    UBYTE ln_Type;
    UBYTE ln_Pri;
    char* ln_Name;
} Node;

typedef struct List {
    struct Node* lh_Head;
    struct Node* lh_Tail;
    struct Node* lh_TailPred;
    UBYTE lh_Type;
    UBYTE lh_pad;
} List;

typedef struct Library {
    struct Node lib_Node;
    UBYTE lib_Flags;
    UBYTE lib_pad;
    UWORD lib_NegSize;
    UWORD lib_PosSize;
    UWORD lib_Version;
    UWORD lib_Revision;
    APTR lib_IdString;
    ULONG lib_Sum;
    UWORD lib_OpenCnt;
} Library;

typedef struct Task {
    struct Node tc_Node;
    UBYTE tc_Flags;
    UBYTE tc_State;
    UBYTE tc_IDNestCnt;
    UBYTE tc_TDNestCnt;
    APTR tc_SigExcept;
    APTR tc_SigRecvd;
    APTR tc_SigAlloc;
    APTR tc_MemEntry;
    APTR tc_UserData;
    APTR tc_SPReg;
    APTR tc_SPLower;
    APTR tc_SPUpper;
    struct List tc_MemList;
    APTR tc_TrapCode;
    APTR tc_TrapData;
    APTR tc_ExceptCode;
    APTR tc_ExceptData;
    APTR tc_ExitCode;
    APTR tc_ExitData;
    APTR tc_Switch;
    APTR tc_Launch;
    APTR tc_FinalPC;
    ULONG tc_FinalCode;
    struct List tc_ExecData;
} Task;

typedef struct Process {
    struct Task pr_Task;
    APTR pr_SegList;
    APTR pr_StackSize;
    APTR pr_StackBase;
    APTR pr_GlobVec;
    APTR pr_TaskArray;
    APTR pr_CLI;
    APTR pr_Result2;
    APTR pr_ConsoleTask;
    APTR pr_CIS;
    APTR pr_COS;
    APTR pr_Arguments;
    APTR pr_FreeVec;
    APTR pr_WindowPtr;
    APTR pr_HomeDir;
    APTR pr_CurrentDir;
    APTR pr_CISName;
    APTR pr_COSName;
    APTR pr_Module;
} Process;

/* Signal bits */
#define SIGB_SINGLE   0
#define SIGB_BREAK    1
#define SIGB_CHILD    2
#define SIGB_KILL     3
#define SIGB_TERM     4
#define SIGB_USR1     5
#define SIGB_USR2     6
#define SIGB_USR3     7
#define SIGB_SUPER    8
#define SIGB_MEMERR   9
#define SIGB_BUSERR  10
#define SIGB_FPEERR  11
#define SIGB_ILLEGAL 12
#define SIGB_DIVZERO 13
#define SIGB_TRAP    14
#define SIGB_PRIV    15

#define SIGF_SINGLE   (1L << SIGB_SINGLE)
#define SIGF_BREAK    (1L << SIGB_BREAK)
#define SIGF_CHILD    (1L << SIGB_CHILD)
#define SIGF_KILL     (1L << SIGB_KILL)
#define SIGF_TERM     (1L << SIGB_TERM)
#define SIGF_USR1     (1L << SIGB_USR1)
#define SIGF_USR2     (1L << SIGB_USR2)
#define SIGF_USR3     (1L << SIGB_USR3)
#define SIGF_SUPER    (1L << SIGB_SUPER)
#define SIGF_MEMERR   (1L << SIGB_MEMERR)
#define SIGF_BUSERR   (1L << SIGB_BUSERR)
#define SIGF_FPEERR   (1L << SIGB_FPEERR)
#define SIGF_ILLEGAL  (1L << SIGB_ILLEGAL)
#define SIGF_DIVZERO  (1L << SIGB_DIVZERO)
#define SIGF_TRAP     (1L << SIGB_TRAP)
#define SIGF_PRIV     (1L << SIGB_PRIV)

/* Memory flags */
#define MEMF_ANY         0x00000000
#define MEMF_PUBLIC      0x00000001
#define MEMF_CHIP        0x00000002
#define MEMF_FAST        0x00000004
#define MEMF_CLEAR       0x00000010
#define MEMF_LOCAL       0x00000020
#define MEMF_24BITDMA    0x00000040
#define MEMF_REVERSE     0x00000080
#define MEMF_KICK        0x00000100
#define MEMF_EXEC        0x00000200

/* Function prototypes */
APTR AllocMem(ULONG size, ULONG flags);
void FreeMem(APTR memory, ULONG size);
APTR AllocVec(ULONG size, ULONG flags);
void FreeVec(APTR memory);

void* AllocPooled(APTR poolHeader, ULONG size);
void FreePooled(APTR poolHeader, APTR memory, ULONG size);
APTR CreatePool(ULONG flags, ULONG puddleSize, ULONG threshSize);
void DeletePool(APTR poolHeader);

struct Task* FindTask(APTR task);
struct Task* SetTaskPri(struct Task* task, int16_t priority);
ULONG Signal(struct Task* task, ULONG signals);
ULONG Wait(ULONG signalMask);
void Forbid(void);
void Permit(void);
void Disable(void);
void Enable(void);

void AddHead(struct List* list, struct Node* node);
void AddTail(struct List* list, struct Node* node);
struct Node* RemHead(struct List* list);
struct Node* RemTail(struct List* list);
void Remove(struct Node* node);
void Enqueue(struct List* list, struct Node* node);
struct Node* FindName(struct List* list, const char* name);

struct Library* OpenLibrary(const char* name, ULONG version);
void CloseLibrary(struct Library* library);
APTR OpenDevice(const char* name, LONG unit, struct IORequest* ioRequest, ULONG flags);
void CloseDevice(struct IORequest* ioRequest);

struct Interrupt {
    struct Node is_Node;
    APTR is_Data;
    APTR (*is_Code)(APTR);
};

void AddIntServer(UBYTE intNum, struct Interrupt* interrupt);
void RemIntServer(UBYTE intNum, struct Interrupt* interrupt);

struct SignalSemaphore {
    struct Node ss_Link;
    struct Task* ss_Waiter;
    struct List ss_Queue;
    WORD ss_NestCount;
};

void InitSemaphore(struct SignalSemaphore* semaphore);
void ObtainSemaphore(struct SignalSemaphore* semaphore);
void ReleaseSemaphore(struct SignalSemaphore* semaphore);
BOOL AttemptSemaphore(struct SignalSemaphore* semaphore);

typedef struct MsgPort {
    struct Node mp_Node;
    UBYTE mp_Flags;
    UBYTE mp_SigBit;
    struct Task* mp_SigTask;
    struct List mp_MsgList;
} MsgPort;

typedef struct Message {
    struct Node mn_Node;
    struct MsgPort* mn_ReplyPort;
    UWORD mn_Length;
} Message;

struct MsgPort* CreateMsgPort(void);
void DeleteMsgPort(struct MsgPort* port);
void PutMsg(struct MsgPort* port, struct Message* message);
struct Message* GetMsg(struct MsgPort* port);
struct Message* WaitPort(struct MsgPort* port);
void ReplyMsg(struct Message* message);

APTR AllocEntry(struct MemList* memList);
void FreeEntry(struct MemList* memList);

struct MemList {
    struct Node ml_Node;
    ULONG ml_Size;
    ULONG ml_NumEntries;
    APTR ml_Memory[1];
};

struct IORequest {
    struct Message io_Message;
    struct Device* io_Device;
    struct Unit* io_Unit;
    UWORD io_Command;
    UBYTE io_Flags;
    UBYTE io_Error;
};

struct Device {
    struct Library dd_Library;
    UWORD dd_Flags;
    UBYTE dd_pad;
    APTR dd_OpenVector;
};

struct Unit {
    struct Node unit_Node;
    UBYTE unit_Flags;
    UBYTE unit_pad;
    struct MsgPort unit_MsgPort;
};

APTR CreateTask(char* name, LONG priority, APTR initialPC, ULONG stackSize);
void DeleteTask(struct Task* task);

ULONG GetCC(void);
void SetCC(ULONG cc);

void CacheControl(APTR address, ULONG length, ULONG control);

#endif /* EXEC_EXEC_H */