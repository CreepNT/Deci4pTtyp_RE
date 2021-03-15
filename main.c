
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/proc_event.h>

#include "vfs.h"
#include "deci4p_tty.h"

#define SCE_KERNEL_EVF_ATTR_MULTI 0x1000
#define SCE_KERNEL_EVF_ATTR_0x8000 0x8000


int module_start(void){
    //Check if some DIP Switches are set
    if (!ksceKernelCheckDipsw(0xC2) || !(ksceKernelCheckDispsw(0xC7)))
        return SCE_KERNEL_START_NO_RESIDENT;
    
    memset(&ctx1, 0, sizeof(ctx1));
    memset(&ctx2, 0, sizeof(ctx2));

    //Create kernel-managed stuff needed for operation
    SceKernelHeapCreateOpt hOpt;
    hOpt.size = sizeof(hOpt);
    hOpt.uselock = 0x401;
    hOpt.field_10 = 0x10F0D006;
    SceDeci4pTtyp_global_heap = ksceKernelCreateHeap(MODULE_NAME, 0x1000, &hOpt);
    if (SceDeci4pTtyp_global_heap < 0)
        return SCE_KERNEL_START_FAILED;

    SceDeci4pTtyp_global_mutex = ksceKernelCreateMutex(MODULE_NAME, SCE_KERNEL_MUTEX_ATTR_TH_PRIO, 0, NULL);
    if (SceDeci4pTtyp_global_mutex < 0)
        return SCE_KERNEL_START_FAILED;

    SceDeci4pTtyp_global_sema = ksceKernelCreateSema(MODULE_NAME, SCE_KERNEL_SEMA_ATTR_TH_PRIO, 3, 3, NULL);
    if (SceDeci4pTtyp_global_sema < 0)
        return SCE_KERNEL_START_FAILED;

    struct SceDeci4pDfMgrForDebugger_529979FB_arg2 pckt;
    pckt.size = sizeof(pckt);
    pckt.unk8 = 0;
    pckt.unkc = 0x1000004;
    pckt.unk10 = 0x20001;
    pckt.unk14 = 0;
    pckt.unk16 = 0x1;

    //Setup internal context + call some Deci4pDfMgrForDebugger exports
    for (int i = 0; i < TTY_COUNT; i++){
        //////Warning maybe incomplete
        char evf_name[0x20];
        pckt.unk4 = 0x80030000 | i; //Addition is same as OR because i < 0x80030000
        snprintf(evf_name, sizeof(evf_name), MODULE_NAME"%d", i);
        snprintf(pckt.name, sizeof(pckt.name), "TTYP%d", i);
        ctx1[i].eventFlag = ksceKernelCreateEventFlag(evf_name, SCE_KERNEL_EVF_ATTR_0x8000 | SCE_KERNEL_EVF_ATTR_MULTI | SCE_KERNEL_EVF_ATTR_TH_FIFO, 0, NULL);
        ctx1[i].ctx2_ptr = &ctx2[i];
        ctx2[i].bufStart = ctx2[i].buffer;
        ctx2[i].bufEnd = ctx2[i].buffer;
        ctx1[i].SceDeci4pDfMgrForDebugger_529979FB_handle = SceDeci4pDfMgrForDebugger_529979FB(0x81000381, &pckt, &ctx1[i]);
        ctx1[i].unk54 = 1;
        memset(&ctx1[i].unk60, 0, 0x1040);
        ctx1[i].ptr_to_unk60 = &ctx1[i].unk60;
        ctx1[i].unk_10A4 = 0;
    }
    memset(&ctx3, 0, sizeof(ctx3));

    //Mount the VFS
    int ret = ksceVfsUnmount(&deci4p_tty_vfs_unmount);
    if (ret >= 0){
        ret = ksceVfsMount(&deci4p_tty_vfs_add);
        if (ret >= 0)
            ksceVfsMount(&deci4p_tty_vfs_mount);
    }

    //Register ProcEvent handler
    ret = ksceKernelRegisterProcEventHandler(MODULE_NAME, &deci4p_proc_handler, 0);
    if (ret < 0)
        return SCE_KERNEL_START_FAILED;
    else
        return SCE_KERNEL_START_SUCCESS;

}