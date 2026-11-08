#ifndef EXEC_PORTS_H
#define EXEC_PORTS_H

#include "exec/exec.h"

#define EXEC_PORTS_H

/* MsgPort and Message are already defined in exec.h, just expose additional functions */
struct MsgPort* CreateMsgPort(void);
void DeleteMsgPort(struct MsgPort* port);
void PutMsg(struct MsgPort* port, struct Message* message);
struct Message* GetMsg(struct MsgPort* port);
struct Message* WaitPort(struct MsgPort* port);
void ReplyMsg(struct Message* message);

#define PA_SIGNAL    0
#define PA_SOFTINT   1
#define PA_IGNORE    2

#define NT_MESSAGE   1
#define NT_REPLYMSG  2
#define NT_DEVICE    3
#define NT_SOFTINT   4
#define NT_FREEMSG   5
#define NT_SIGNAL    6
#define NT_EXCEPTION 7
#define NT_UNUSED    8

#endif /* EXEC_PORTS_H */