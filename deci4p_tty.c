#include "deci4p_tty.h"
#include <psp2kern/kernel/cpu.h>

SceUID SceDeci4pTtyp_global_mutex;
SceUID SceDeci4pTtyp_global_sema;
SceUID SceDeci4pTtyp_global_heap;

Deci4pTTY_ctx1 ctx1[TTY_COUNT] = {0};
Deci4pTTY_ctx2 ctx2[TTY_COUNT];
Deci4pTTY_ctx3 ctx3[3];

const SceProcEventHandler deci4p_proc_handler = {
    .size = sizeof(deci4p_proc_handler),
    .create = NULL,
    .exit = FUN_810001C0,
    .kill = FUN_81000000,
    .stop = NULL,
    .start = NULL,
    .switch_process = NULL
};

SceVfsUmount deci4p_tty_vfs_unmount = {
    .device = DECI4P_TTY_FS_DEVICE,
    .data_0x04 = 0,
};

static SceVfsMount2 deci4p_tty_vfs_mount2 = {
  .unit = DECI4P_TTY_UNIT,
  .device1 = DECI4P_TTY_FS_DEVICE,
  .device2 = DECI4P_TTY_FS_DEVICE,
  .data_0x0C = 0,
  .data_0x10 = 0
};

SceVfsMount deci4p_tty_vfs_mount = {
  .device = DECI4P_TTY_DEVICE,
  .data_0x04 = 0,
  .data_0x08 = 0x4000020,
  .data_0x0C = 0x6003,
  .data_0x10 = DECI4P_TTY_FS_DEVICE,
  .data_0x14 = 0,
  .data_0x18 = &deci4p_tty_vfs_mount2,
  .data_0x1C = 0
};

SceVfsAdd deci4p_tty_vfs_add = {
    .func_ptr1 = &vfs_func_table_1,
    .device = DECI4P_TTY_FS_DEVICE,
    .data_0x08 = 0x13,
    .data_0x0C = 0,
    .data_0x10 = 0x10,
    .func_ptr2 = &vfs_func_table_2,
    .data_0x18 = 0
};

static int suspend_intr_addr = 0; //maybe int[2] ?

static int handler(SceUID pid) { //This is reversed from exit handler, but the code is the same for kill ?
    int ret = ksceKernelWaitSema(SceDeci4pTtyp_global_sema, 1, NULL);
    if (ret < 0) return 0;
    ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
    if (ret < 0) return 0;
    
    int firstFreeIdx;
    if (ctx3[0].currentState) //Check if non-0 (not free)
        if (ctx3[1].currentState)
            if (ctx3[2].currentState){
                ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                return 0;
            }
            else firstFreeIdx = 2;
        else firstFreeIdx = 1;
    else firstFreeIdx = 0;

    Deci4pTTY_ctx3* ctx3_p = &ctx3[firstFreeIdx];
    ctx3_p->currentState = 1;
    ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);

    ctx3_p->unk10 = 0x1000;
    ctx3_p->unk12 = 2;
    ctx3_p->unk14 = pid;
    ctx3_p->unk18 = NULL;
    ctx3_p->unk1C = NULL;
    SceDeci4pDfMgrForDebugger_6D26CC56(ctx3_p, CTX3_HEADER_SIZE);
    ctx3_p->unk28 = 0;
    ctx3_p->currentState = 2;
    ctx3_p->SysEventHandlerId = ctx1[0].SysEventHandlerId;
    ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
    if (ret >= 0){
        (ctx1[0].associated_ctx3)->unk_1030 = ctx3_p;
        ctx1[0].associated_ctx3 = ctx3_p;
        ctx3_p->unk_1030 = NULL;
        ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
    }
    ret = SceDeci4pDfMgrForDebugger_BADEF855(ctx1[0].SysEventHandlerId);
    if ( ret < 0 && ret != 0x80080007) {
        ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
        if (ret >= 0){
            Deci4pTTY_ctx3* ctx1unk1030 = ctx1[0].ctx3.unk_1030;
            if (ctx1unk1030 != NULL){
                if (ctx1unk1030 == ctx3_p){
                    ctx1[0].ctx3.unk_1030 = ctx3_p;
                    if (ctx1unk1030->unk_1030 == NULL)
                        ctx1[0].associated_ctx3 = &ctx1[0].ctx3;
                    ctx3_p->unk_1030 = NULL;
                }
                else {
                    int skip = 0;
                    Deci4pTTY_ctx3 *pcv5, **addr = &ctx1[0].ctx3.unk_1030;
                    do {
                        pcv5 = ctx1unk1030;
                        addr = &pcv5->unk_1030;
                        ctx1unk1030 = pcv5->unk_1030;
                        if (ctx1unk1030 == NULL){
                            skip = 1;
                            break;
                        }
                    } while (ctx3_p != ctx1unk1030);
                    if (!skip){
                        *addr = ctx3_p->unk_1030;
                        if (ctx1unk1030->unk_1030 == NULL)
                            ctx1[0].associated_ctx3 = pcv5;
                        ctx3_p->unk_1030 = NULL;
                    }
                }
            }
            ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
        }
        ret = ksceKernelCpuSuspendIntr(&suspend_intr_addr);
        memset(ctx3_p, 0, sizeof(*ctx3_p));
        ksceKernelCpuResumeIntr(&suspend_intr_addr, ret);
        ksceKernelSignalSema(SceDeci4pTtyp_global_sema, 1);
    }
    return 0;
}

int FUN_810001C0(SceUID pid, SceProcEventInvokeParam1* a2, int a3){ //exit handler
    return handler(pid); //does not actually do a function call - read handler() comment
}

int FUN_81000000(SceUID pid, SceProcEventInvokeParam1* a2, int a3){ //kill handler
    return handler(pid); //does not actually do a function call - read handler() comment
}

int FUN_81000380(int a1, void* a2, void* args){ //Sysevent handler
    Deci4pTTY_ctx1* arg = (Deci4pTTY_ctx1*)args;

    switch(a1){
        case 1:
        case 2:
        {
            if ( (arg->unk44 != 0) && ( arg->unk48 > 0x2C /* maybe CTX3_HEADER_SIZE ? */ ) ){
                arg->unk44 = 0;
                int ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
                if (ret < 0)
                    return 0;

                int unk44 = arg->unk44;
                do {
                    uint8_t* pbv3 = arg->unk4C;
                    uint32_t unk0 = arg->ctx2_ptr->unk0;
                    if (unk0 & 1){
LAB_81000724:   
                        if (!(unk0 & 2)){
                            char* p = arg->ctx2_ptr->bufStart + 1;
                            if ( (void*)&arg->ctx2_ptr[1] <= (void*)p ) { //Check bufStart+1 doesn't go past the end of this ctx2
                                p = arg->ctx2_ptr->buffer;
                            }
                            if (p != arg->ctx2_ptr->bufEnd){
                                *arg->ctx2_ptr->bufStart = *pbv3;
                                arg->ctx2_ptr->bufStart = p;
                            }
                        }
                    }
                    else {
                        if ((*pbv3 == 3) && (unk0 & 0x4)) {
                            arg->unk54 |= 0x2;
                            return 0;
                        }
                        else if ( (*pbv3 == 0) || ((*pbv3 & 0x7F) != 0x11) ){
                            goto LAB_81000724;
                        }
                        else if ((*pbv3 & 0x7F) == 13) {
                            arg->ctx2_ptr->unk0 |= 2;
                        }
                        else /* (*pbv3 & 0x7F) == 11 */ {
                            arg->ctx2_ptr->unk0 &= 0xFFFFFFFD;
                        }
                    }
                    pbv3++;
                    unk44--;
                } while (unk44 != 0);
            }
            if (a1 != 2){
                if (arg->unk48 < 0x2C){
                    int ret = SceDeci4pDfMgrForDebugger_5F2C7E11(arg->SysEventHandlerId, arg->unk18, sizeof(arg->unk18));
                    arg->unk48 += ret;
                    return 0;
                }
                else {
                    int ret = SceDeci4pDfMgrForDebugger_5F2C7E11(arg->SysEventHandlerId, arg->unk4C, sizeof(arg->unk4C));
                    arg->unk44 = ret;
                    arg->unk48 += ret;
                    return 0;
                }
                ksceKernelSetEventFlag(arg->eventFlag, 0x3);
                int ret = ksceKernelWaitSema(SceDeci4pTtyp_global_sema, 1, NULL);
                if (ret < 0){
                    arg->unk48 = 0;
                    return 0;
                }
                ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
                if (ret < 0){
                    arg->unk48 = 0;
                    return 0;
                }
                int firstFreeIdx;
                if (ctx3[0].currentState) //Check if non-0 (not free)
                    if (ctx3[1].currentState)
                        if (ctx3[2].currentState){
                            ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                            arg->unk48 = 0;
                            return 0;
                        }
                        else firstFreeIdx = 2;
                    else firstFreeIdx = 1;
                else firstFreeIdx = 0;

                Deci4pTTY_ctx3* ctx3_p = &ctx3[firstFreeIdx];
                ctx3_p->currentState = 1;
                ctx3_p ->unk10 = arg->unk28;
                ctx3_p->unk12 = 1;
                ctx3_p->unk14 = (SceUID)&arg->unk2C;
                ctx3_p->unk18 = &arg->unk30;
                ctx3_p->unk1C = &arg->unk34;
                SceDeci4pDfMgrForDebugger_6D26CC56(ctx3_p, CTX3_HEADER_SIZE);
                ctx3_p->unk28 = 0;
                ctx3_p->currentState = 2;
                ctx3_p->SysEventHandlerId = arg->SysEventHandlerId;
                ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
                if (ret >= 0){
                    arg->associated_ctx3->unk_1030 = ctx3_p;
                    arg->associated_ctx3 = ctx3_p;
                    ctx3_p->unk_1030 = NULL;
                    ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                }

                ret = SceDeci4pDfMgrForDebugger_BADEF855(arg->SysEventHandlerId);
                if (ret < 0 && ret != 0x80080007){
                    ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
                    if (ret >= 0){
                        Deci4pTTY_ctx3* pcv9 = (arg->ctx3).unk_1030;
                        if (pcv9 != NULL){
                            if (ctx3_p == pcv9){
                                (arg->ctx3).unk_1030 = ctx3_p->unk_1030;
                                if (ctx3_p->unk_1030 == NULL)
                                    arg->associated_ctx3 = &arg->ctx3;
                                ctx3_p->unk_1030 = NULL;
                            }
                            else {
                                int skip = 0;
                                Deci4pTTY_ctx3* pcv10;
                                do {
                                    pcv10 = pcv9;
                                    pcv9 = pcv10->unk_1030;
                                    if (pcv9 == NULL){
                                        skip = 1;
                                        break;
                                    }
                                } while (ctx3_p != pcv9);
                                if (!skip){
                                    pcv10->unk_1030 = ctx3_p->unk_1030;
                                    if (pcv9->unk_1030 == NULL)
                                        arg->associated_ctx3 = &arg->ctx3;
                                    ctx3_p->unk_1030 = NULL;
                                }
                            }
                            int prev_state = ksceKernelCpuSuspendIntr(&suspend_intr_addr);
                            memset(ctx3_p, 0, sizeof(*ctx3_p));
                            ksceKernelCpuResumeIntr(&suspend_intr_addr, prev_state);
                            ksceKernelSignalSema(SceDeci4pTtyp_global_sema, 0x1);
                        }
                        ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                    }
                }
            }
            break;
        }

        case 3:
        {
            int ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
            if (ret >= 0){
                Deci4pTTY_ctx3* ctx3_p = (arg->ctx3).unk_1030;
                if (ctx3_p != NULL){
                    (arg->ctx3).unk_1030 = ctx3_p->unk_1030;
                    if (ctx3_p->unk_1030 == NULL)
                        arg->associated_ctx3 = &arg->ctx3;
                    ctx3_p->unk_1030 = NULL;
                    ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                    arg->unk_10A4 = ctx3_p;
                    ret = SceDeci4pDfMgrForDebugger_CACAB5F9(ctx3_p);
                    SceDeci4pDfMgrForDebugger_C3390112(arg->SysEventHandlerId, ctx3_p, ret);
                }
                ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
            }
            break;
        }
        
        case 4:
        {
            Deci4pTTY_ctx3* ctx3_p = arg->unk_10A4;
            if (ctx3_p != NULL){
                int prev_state = ksceKernelCpuSuspendIntr(&suspend_intr_addr);
                memset(ctx3_p, 0, sizeof(*ctx3_p));
                ksceKernelCpuResumeIntr(&suspend_intr_addr, prev_state);
                ksceKernelSignalSema(SceDeci4pTtyp_global_sema, 1);
                arg->unk_10A4 = NULL;
                int ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
                if (ret >= 0){
                    ctx3_p = (arg->ctx3).unk_1030;
                    ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                    if (ctx3_p != NULL)
                        SceDeci4pDfMgrForDebugger_BADEF855(ctx3_p->SysEventHandlerId);
                }
            }
            break;
        }

        case 5:
        {
            if ( *(uint8_t*)((int)a2 + 0x11) == 0 ){
                if ( *(uint8_t*)((int)a2 + 0x14) == 0 ){
                    arg->ctx2_ptr->unk0 |= 0x2;
                    ksceKernelSetEventFlag(arg->eventFlag, 0x2);
                    int ret;
                    Deci4pTTY_ctx3* pcv15;
                    while ( (ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL)), ret >= 0 ){
                        pcv15 = (arg->ctx3).unk_1030;
                        if (pcv15 == NULL){
                            ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                            return 0;
                        }
                        (arg->ctx3).unk_1030 = pcv15->unk_1030;
                        if (pcv15->unk_1030 == NULL)
                            arg->associated_ctx3 = &arg->ctx3;
                        pcv15->unk_1030 = NULL;
                        ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                        int prev_state = ksceKernelCpuSuspendIntr(&suspend_intr_addr);
                        memset(pcv15, 0, sizeof(*pcv15));
                        ksceKernelCpuResumeIntr(&suspend_intr_addr, prev_state);
                        ksceKernelSignalSema(SceDeci4pTtyp_global_sema, 1);
                    }
                }
                else if ( *(uint8_t*)((int)a2 + 0x14) == 1 ){
                    arg->ctx2_ptr->unk0 &= 0xFFFFFFFD; //Clear 0x2 bit
                    ksceKernelSetEventFlag(arg->eventFlag, 0x2);
                }
                break;
            }
            else if ( *(uint8_t*)((int)a2 + 0x11) == 1 ){
                if ( *(uint8_t*)((int)a2 + 0x24) == 0 ){
                    arg->unk54 &= 0xFFFFFFFE; //Clear LSB
                    ksceKernelSetEventFlag(arg->eventFlag, 0x2);
                    int ret;
                    Deci4pTTY_ctx3* pcv15;
                    while ( (ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL)), ret >= 0 ){ //Same code as above, so maybe it's an inline function ?
                        pcv15 = (arg->ctx3).unk_1030;
                        if (pcv15 == NULL){
                            ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                            return 0;
                        }
                        (arg->ctx3).unk_1030 = pcv15->unk_1030;
                        if (pcv15->unk_1030 == NULL)
                            arg->associated_ctx3 = &arg->ctx3;
                        pcv15->unk_1030 = NULL;
                        ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                        int prev_state = ksceKernelCpuSuspendIntr(&suspend_intr_addr);
                        memset(pcv15, 0, sizeof(*pcv15));
                        ksceKernelCpuResumeIntr(&suspend_intr_addr, prev_state);
                        ksceKernelSignalSema(SceDeci4pTtyp_global_sema, 1);
                    }
                }
                else if ( *(uint8_t*)((int)a2 + 0x24) == 1 ){
                    arg->unk54 |= 0x1;
                    ksceKernelSetEventFlag(arg->eventFlag, 0x2);
                }
                break;
            }
        }

        case 6:
        {
            if ( *(uint8_t*)((int)a2 + 0x24) == 1 ){
                arg->ctx2_ptr->unk0 |= 0x2;
                ksceKernelSetEventFlag(arg->eventFlag, 0x2);
            }
            else if ( *(uint8_t*)((int)a2 + 0x24) == 2 ){
                arg->unk54 &= 0xFFFFFFFE; //Clear LSB
                ksceKernelSetEventFlag(arg->eventFlag, 0x2);
            }
            break;
        }

    }
    return 0;
}