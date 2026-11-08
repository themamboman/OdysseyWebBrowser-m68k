#ifndef EXEC_MEMORY_H
#define EXEC_MEMORY_H

#include "exec/exec.h"

typedef struct MemHeader {
    struct Node mh_Node;
    UWORD mh_Attributes;
    UBYTE mh_pad;
    UBYTE mh_FreeBlocks;
    ULONG mh_First;
    ULONG mh_Lower;
    ULONG mh_Upper;
    ULONG mh_Free;
} MemHeader;

#endif