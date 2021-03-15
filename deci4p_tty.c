#include "deci4p_tty.h"
#include <psp2kern/kernel/cpu.h>

SceUID SceDeci4pTtyp_global_mutex;
SceUID SceDeci4pTtyp_global_sema;
SceUID SceDeci4pTtyp_global_heap;

Deci4pTTY_ctx1 ctx1[TTY_COUNT] = {0};
Deci4pTTY_ctx2 ctx2[TTY_COUNT];
Deci4pTTY_ctx3 ctx3[3];

static int suspend_intr_addr = 0; //maybe int[2] ?

static int handler(SceUID pid) { //This is reversed from exit handler, but the code is the same for kill ?
    int ret = ksceKernelWaitSema(SceDeci4pTtyp_global_sema, 1, NULL);
    if (ret < 0) return 0;
    ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
    if (ret < 0) return 0;
    
    int firstNonInitIdx;
    if (ctx3[0].hasBeenInit)
        if (ctx3[1].hasBeenInit)
            if (ctx3[2].hasBeenInit){
                ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                return 0;
            }
            else firstNonInitIdx = 2;
        else firstNonInitIdx = 1;
    else firstNonInitIdx = 0;

    Deci4pTTY_ctx3* ctx3_p = &ctx3[firstNonInitIdx];
    ctx3_p->hasBeenInit = 1;
    ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);

    ctx3_p->unk10 = 0x1000;
    ctx3_p->unk12 = 2;
    ctx3_p->unk14 = pid;
    ctx3_p->unk18 = 0;
    ctx3_p->unk1C = 0;
    SceDeci4pDfMgrForDebugger_6D26CC56(ctx3_p, 0x2C);
    ctx3_p->unk28 = 0;
    ctx3_p->hasBeenInit = 2;
    ctx3_p->SceDeci4pDfMgrForDebugger_529979FB_handle = ctx1[0].SceDeci4pDfMgrForDebugger_529979FB_handle;
    ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
    if (ret >= 0){
        (ctx1[0].associated_ctx3)->unk_1030 = ctx3_p;
        ctx1[0].associated_ctx3 = ctx3_p;
        ctx3_p->unk_1030 = NULL;
        ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
    }
    ret = SceDeci4pDfMgrForDebugger_BADEF855(ctx1[0].SceDeci4pDfMgrForDebugger_529979FB_handle);
    if ( ret == 0x80080007 || ret >= 0 ) {
        ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
        if (ret >= 0){
            Deci4pTTY_ctx3* ctx1unk1030 = ctx1[0].ctx3.unk_1030;
            if (ctx1unk1030 != NULL){
                if (ctx1unk1030 == ctx3_p){
                    ctx1[0].ctx3.unk_1030 = ctx3_p;
                    if (ctx1unk1030->unk_1030 == NULL)
                        ctx1[0].associated_ctx3 = &ctx1[0].ctx3.unk_1030;
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
        memset(ctx3_p, 0, sizeof(Deci4pTTY_ctx3));
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