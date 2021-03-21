
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/proc_event.h>

#include "vfs.h"
#include "deci4p_tty.h"

#define SCE_KERNEL_EVF_ATTR_0x8000 0x8000

//Imports from SceSysclibForDriver
int snprintf(char*, size_t, char*, ...); 
void* memset(void*, int, size_t);
void * memcpy(void*, void*, size_t);

int ksceKernelCheckDipsw(int);

void _start() __attribute__((weak, alias("module_start")));
int module_start(void){
    //Check if some DIP Switches are set
    if (!ksceKernelCheckDipsw(0xC2) || !(ksceKernelCheckDipsw(0xC7)))
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

    SceDeci4pDfMgrRegisterSysEventForDebuggerParam regparam;
    regparam.size = sizeof(regparam);
    regparam.unk8 = 0;
    regparam.unkC = 0x1000004;
    regparam.unk10 = 0x20001;
    regparam.unk14 = 0;
    regparam.unk16 = 0x1;

    //Setup internal context and register Deci4pSysEvent handlers
    for (int i = 0; i < TTY_COUNT; i++){
        //////Warning maybe incomplete
        char evf_name[0x20];
        regparam.unk4 = 0x80030000 | i; //Addition is same as OR because i < 0x80030000
        snprintf(evf_name, sizeof(evf_name), MODULE_NAME"%d", i);
        snprintf(regparam.name, sizeof(regparam.name), "TTYP%d", i);
        ctx1[i].eventFlag = ksceKernelCreateEventFlag(evf_name, SCE_KERNEL_EVF_ATTR_0x8000 | SCE_KERNEL_EVF_ATTR_MULTI | SCE_KERNEL_EVF_ATTR_TH_FIFO, 0, NULL);
        ctx1[i].ctx2_ptr = &ctx2[i];
        ctx2[i].bufStart = ctx2[i].buffer;
        ctx2[i].bufEnd = ctx2[i].buffer;
        ctx1[i].SysEventHandlerId = sceDeci4pDfMgrRegisterSysEventForDebugger(FUN_81000380, &regparam, &ctx1[i]);
        ctx1[i].unk54 = 1;
        memset(&ctx1[i].ctx3, 0, sizeof(Deci4pTTY_ctx3));
        ctx1[i].associated_ctx3 = &ctx1[i].ctx3;
        ctx1[i].unk_10A4 = 0;
    }
    memset(&ctx3, 0, sizeof(ctx3));

    //Mount the VFS
    int ret = ksceVfsUnmount(&deci4p_tty_vfs_unmount);
    if (ret >= 0){
        ret = ksceVfsAddVfs(&deci4p_tty_vfs_add);
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

int module_stop(SceSize argc, const void* args){
    return SCE_KERNEL_STOP_SUCCESS; //not sure, didn't find it
}