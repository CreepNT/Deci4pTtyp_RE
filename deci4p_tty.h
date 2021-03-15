
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

typedef struct _Deci4pTTY_ctx3_struct{ //Size is 0x1040
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
} Deci4pTTY_ctx3;

typedef struct _Deci4pTTY_serial_storage_buffer{ //Size is 0x40C
  uint32_t unk0;
  char* bufStart; //Maybe
  char* bufEnd; //Maybe
  uint8_t buffer[0x400]; //maybe
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
static int just_for_debug1 = sizeof(Deci4pTTY_ctx1); //4320
static int just_for_debug2 = sizeof(Deci4pTTY_ctx2); //1036
static int just_for_debug3 = sizeof(Deci4pTTY_ctx3); //4160

extern SceUID SceDeci4pTtyp_global_mutex;
extern SceUID SceDeci4pTtyp_global_sema;
extern SceUID SceDeci4pTtyp_global_heap;

const SceVfsUmount deci4p_tty_vfs_unmount = {
    .device = DECI4P_TTY_FS_DEVICE,
    .data_0x04 = 0,
};

static const SceVfsMount2 deci4p_tty_vfs_mount2 = {
  .unit = DECI4P_TTY_UNIT,
  .device1 = DECI4P_TTY_FS_DEVICE,
  .device2 = DECI4P_TTY_FS_DEVICE,
  .data_0x0C = 0,
  .data_0x10 = 0
};

const SceVfsMount deci4p_tty_vfs_mount = {
  .device = DECI4P_TTY_DEVICE,
  .data_0x04 = 0,
  .data_0x08 = 0x4000020,
  .data_0x0C = 0x6003,
  .data_0x10 = DECI4P_TTY_FS_DEVICE,
  .data_0x14 = 0,
  .data_0x18 = &deci4p_tty_vfs_mount2,
  .data_0x1C = 0
};

const SceVfsAdd deci4p_tty_vfs_add = {
    .func_ptr1 = &vfs_func_table_1,
    .device = DECI4P_TTY_FS_DEVICE,
    .data_0x08 = 0x13,
    .data_0x0C = 0,
    .data_0x10 = 0x10,
    .func_ptr2 = &vfs_func_table_2,
    .data_0x18 = 0
};

static int FUN_810001C0(SceUID pid, SceProcEventInvokeParam1* a2, int a3);
static int FUN_81000000(SceUID pid, SceProcEventInvokeParam1* a2, int a3);
const SceProcEventHandler deci4p_proc_handler = {
    .size = sizeof(deci4p_proc_handler),
    .create = NULL,
    .exit = FUN_810001C0,
    .kill = FUN_81000000,
    .stop = NULL,
    .start = NULL,
    .switch_process = NULL
};

struct SceDeci4pDfMgrForDebugger_529979FB_arg2{
    SceSize size; //0x38
    uint32_t unk4; //Set to (0x80030000 | port number)
    uint32_t unk8; //Set to 0
    uint32_t unkc; //Set to 0x1000004
    uint32_t unk10; //Set to 0x20001
    uint16_t unk14; //Set to 0
    uint16_t unk16; //Set to 1
    char* name; //Set to "TTYP%d" % port number
};

int SceDeci4pDfMgrForDebugger_529979FB(int a1, struct SceDeci4pDfMgrForDebugger_529979FB_arg2* a2, Deci4pTTY_ctx1* a3);
int SceDeci4pDfMgrForDebugger_BADEF855(int SceDeci4pDfMgrForDebugger_529979FB_handle);
int SceDeci4pDfMgrForDebugger_6D26CC56(Deci4pTTY_ctx3* ctx, size_t size);
int SceDebugForKernel_254A4997(char ch);
int SceThreadmgrForDriver_F311808F(int faultingProcessUnk);
int SceThreadmgrForDriver_91382762(/* not sure */);
int kscePmMgrGetProductMode(int* prodMode);