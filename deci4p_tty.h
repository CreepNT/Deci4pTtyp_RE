#ifndef DECI4P_TTY_H
#define DECI4P_TTY_H

#include <psp2common/types.h>
#include <psp2kern/kernel/proc_event.h>


#include "vfs.h"
#include "func_tables.h"

#define MODULE_NAME "SceDeci4pTtyp"
#define DECI4P_TTY_FS_DEVICE "deci4p_ttyp_dev_fs"
#define DECI4P_TTY_DEVICE "/tty"
#define DECI4P_TTY_UNIT "tty"

#define TTY_COUNT 8
#define MAX_TTY_NUM (TTY_COUNT - 1) 

typedef struct _Deci4pTTY_ctx3_struct Deci4pTTY_ctx3;

struct _Deci4pTTY_ctx3_struct{ //Size is 0x1040
  uint32_t unk0;
  uint32_t unk4;
  uint32_t unk8;
  uint32_t unkC;
  uint16_t unk10;
  uint8_t unk12;
  uint8_t padding0;
  SceUID unk14; //Some PID
  uint32_t unk18;
  uint32_t unk1C;
  //Maybe unk20 and unk24 are a single uint64_t
  uint32_t unk20;
  uint32_t unk24;
  uint32_t unk28;
  uint8_t unk2C[0x1000];
  uint32_t hasBeenInit; //not sure of name
  Deci4pTTY_ctx3* unk_1030; //not sure of type
  uint32_t SceDeci4pDfMgrForDebugger_529979FB_handle; //Copied from ctx1
  uint32_t unk_1038;
  uint32_t unk_103C;
};

typedef struct _Deci4pTTY_serial_storage_buffer{ //Size is 0x40C
  uint32_t unk0;
  char* bufStart; //Maybe
  char* bufEnd; //Maybe
  char buffer[0x400]; //maybe
} Deci4pTTY_ctx2;

typedef struct _Deci4pTTY_ctx1_struct { //Size is 0x10E0
    uint32_t SceDeci4pDfMgrForDebugger_529979FB_handle; //Return of SceDeci4pDfMgrForDebugger_529979FB
    uint32_t open_files_count; //not sure
    SceUID eventFlag;
    SceUID readMutex; //"Deci4pTtyRead" with ATTR_PRIO
    SceUID writeMutex; //"Deci4pTtyWrite" with ATTR_PRIO
    Deci4pTTY_ctx2* ctx2_ptr; //Pointer to the corrsponding ctx2
    uint8_t unk18[0x3C];
    uint32_t unk54; //Set to 1 once initialized
    uint8_t unk58[0x8];
    Deci4pTTY_ctx3 ctx3; //not sure
    Deci4pTTY_ctx3* associated_ctx3; //0x10A0
    uint32_t unk_10A4;
    uint8_t unk_10C0[0x38];
} Deci4pTTY_ctx1;


extern Deci4pTTY_ctx1 ctx1[TTY_COUNT];
extern Deci4pTTY_ctx2 ctx2[TTY_COUNT];
extern Deci4pTTY_ctx3 ctx3[3];
extern SceUID SceDeci4pTtyp_global_mutex;
extern SceUID SceDeci4pTtyp_global_sema;
extern SceUID SceDeci4pTtyp_global_heap;
extern SceVfsUmount deci4p_tty_vfs_unmount;
extern SceVfsMount deci4p_tty_vfs_mount;
extern SceVfsAdd deci4p_tty_vfs_add;


int FUN_810001C0(SceUID pid, SceProcEventInvokeParam1* a2, int a3);
int FUN_81000000(SceUID pid, SceProcEventInvokeParam1* a2, int a3);
extern const SceProcEventHandler deci4p_proc_handler;


struct SceDeci4pDfMgrForDebugger_529979FB_arg2{
    SceSize size; //0x38
    uint32_t unk4; //Set to (0x80030000 | port number)
    uint32_t unk8; //Set to 0
    uint32_t unkC; //Set to 0x1000004
    uint32_t unk10; //Set to 0x20001
    uint16_t unk14; //Set to 0
    uint16_t unk16; //Set to 1
    char name[0x20]; //Set to "TTYP%d" % port number
};

int SceDeci4pDfMgrForDebugger_529979FB(int a1, struct SceDeci4pDfMgrForDebugger_529979FB_arg2* a2, Deci4pTTY_ctx1* a3);
int SceDeci4pDfMgrForDebugger_BADEF855(int SceDeci4pDfMgrForDebugger_529979FB_handle);
int SceDeci4pDfMgrForDebugger_6D26CC56(Deci4pTTY_ctx3* ctx, size_t size);
int SceDebugForKernel_254A4997(char ch);
int SceThreadmgrForDriver_F311808F(int faultingProcessUnk);
int SceThreadmgrForDriver_91382762(/* not sure */);
int kscePmMgrGetProductMode(int* prodMode);

#endif