#include <psp2common/types.h>
#include <psp2common/error.h>

#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/cpu.h>

#include "vfs.h"
#include "deci4p_tty.h"

static int32_t SuspendIntr_param = 0;

int FUN_810011B4(void* a1){
    //a1 is maybe SveVfsNode** (or struct* with first field SceVfsNode*)
    void* p = *(void**)a1;
    *(SceUID*)(p + 0x44) = SceDeci4pTtyp_global_heap;
    *(uint32_t*)(p + 0x68) = 0x40;

    return 0;
}

int FUN_81001178(void){
    return 0;
}

int FUN_8100117C(void* a1){
    void* p = *(void**)(a1 + 0x8);

    *(uint32_t*)(p + 0x48) = *(uint32_t*)(*(void**)a1 + 0xC4);
    *(void**)(p + 0x4C) = *(void**)a1;
    *(double*)(p + 0x50) = 0.0f; //loaded with vstr.64
    *(double*)(p + 0x60) = 0.0f; //loaded with vstr.64
    *(uint32_t*)(p + 0x74) = 0x1;
    *(uint32_t*)(p + 0x78) = 0x10020;
    *(double*)(p + 0x80) = 0.0f; //loaded with vstr.64
    *(double*)(p + 0x88) = 0.0f; //loaded with vstr.64

    return 0;
}

int FUN_81001170(){
    return 0;
}

int FUN_81001174(void){
    return 0;
}

int FUN_810012EC(SceVfsOpen* args){ //VFS_Open
    int portNumber = (int)args->node->dev_info; //i.e. tty0 => this is 0 (I think ?)
    if (portNumber > MAX_TTY_NUM)
        return SCE_ERROR_ERRNO_ENXIO;

    Deci4pTTY_ctx1* p = &ctx1[portNumber];
    if (p->open_files_count == 0){
        //Initialize state
        Deci4pTTY_ctx2* ctx2_ptr = p->ctx2_ptr;
        ctx2_ptr->bufStart = ctx2_ptr->buffer;
        ctx2_ptr->bufEnd = ctx2_ptr->buffer;
        SceUID ret = ksceKernelCreateMutex("SceDeci4pTtyRead", SCE_KERNEL_MUTEX_ATTR_TH_PRIO, 0, NULL);
        if (ret < 0) {
            ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
            return ret;
        }
        p->readMutex = ret;
        ret = ksceKernelCreateMutex("SceDeci4pTtyWrite", SCE_KERNEL_MUTEX_ATTR_TH_PRIO, 0, NULL);
        if (ret < 0) {
            ksceKernelDeleteMutex(p->readMutex);
            p->readMutex = SCE_UID_INVALID_UID;
            ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
            return ret;
        }
        p->writeMutex = ret;
    }
    p->open_files_count++;
    return SCE_OK;
}

int FUN_810012C4(SceVfsClose* args){ //VFS_Close
    void** heapMemPtr = (void**)((uintptr_t)args->objectBase + 0x20);
    if (*heapMemPtr != NULL){
        ksceKernelFreeHeapMemory(SceDeci4pTtyp_global_heap, *heapMemPtr);
        *heapMemPtr = NULL;
    }

    int portNumber = (int)args->node->dev_info; //i.e. tty0 => this is 0 (I think ?)
    if (portNumber > MAX_TTY_NUM)
        return SCE_ERROR_ERRNO_ENXIO;

    Deci4pTTY_ctx1* p = &ctx1[portNumber];
    p->open_files_count--;
    if (p->open_files_count == 0){
        if (p->readMutex > 0) ksceKernelDeleteMutex(p->readMutex);
        if (p->writeMutex > 0) ksceKernelDeleteMutex(p->writeMutex);
        p->readMutex = 0;
        p->writeMutex = 0;
        ksceKernelSetEventFlag(p->eventFlag, 0x4);
        ksceKernelSetEventFlag(p->eventFlag, 0xFFFFFFFB);
    }
    return 0;
}

typedef struct _Deci4p_partinit_opt{
    char* name;
    int nameLen; // == strlen(name)
} Deci4p_partinit_opt;

int FUN_8100123C(SceVfsPartInit* args){ //VFS_Part_init
    Deci4p_partinit_opt* opt = args->opt;
    if (opt->nameLen == 4 && opt->name[0] == 't' && opt->name[1] == 't' && opt->name[2] == 'y'){
        SceVfsNode* newNode;
        char portNum = opt->name[3];
        int ret = ksceVfsGetNewNode(args->node->mount, *(int*)(*(int*)((uintptr_t)args->node->mount + 0x5C)), 0, &newNode);
        if (ret > 0){
            ksceVfsNodeWaitEventFlag(newNode);
            newNode->mount = args->node->mount;
            newNode->prev_node = args->node;
            newNode->dev_info = (void*)(portNum - 0x30); //char -> integer
            newNode->unk_74 = 0x1;
            newNode->unk_78 = 0x2010;
            *(args->new_node) = newNode;
            return 0;
        }
    }
    return SCE_ERROR_ERRNO_ENOENT;
}

size_t VfsReadInternal(int port_num, void* data, size_t size, int a4){
    Deci4pTTY_ctx2* ctx2_p = ctx1[port_num].ctx2_ptr;
    char* data_p = data;
    if ((int32_t)size < 0)
        return SCE_ERROR_ERRNO_EINVAL;
    int ret = ksceKernelLockMutex(ctx1[port_num].readMutex, 1, NULL);
    if (ret < 0)
        return ret;
    
    if (size == 0) {
        ksceKernelUnlockMutex(ctx1[port_num].readMutex,1);
        return 0;
    }
        
    size_t len = 0;
    while (len < size){
        //Wait for event flag

        char *start = ctx2_p->bufStart, *end = ctx2_p->bufEnd; //Use local variables to avoid race condition (I think)
        for (uint32_t evf_bit = a4 & 4, evf; evf_bit == 0; evf_bit = evf & 4, start = ctx2_p->bufStart, end = ctx2_p->bufEnd){
            if (start != end) break /*goto skip_check*/;
            int ret2 = ksceKernelWaitEventFlag(ctx1[port_num].eventFlag, 0x5, SCE_KERNEL_EVF_WAITMODE_OR, &evf, NULL);
            if (ret2 < 0){
                ksceKernelUnlockMutex(ctx1[port_num].readMutex,1);
                return ret2;
            }
            ksceKernelClearEventFlag(ctx1[port_num].eventFlag, 0xFFFFFFFE);
        }
        if (start == end) break; //local vars should avoid race condition and need for goto (maybe)
//skip_check:

        ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
        if (ret < 0)
            return ret;

        end = ctx2_p->bufEnd;

        char ch;
        if (end == ctx2_p->bufStart) //wtf ?
            ch = '\0';
        else {
            if ((void*)&ctx2[port_num + 1] <= (void*)ctx2_p->bufEnd) { //Make sure we don't overwrite past the buffer
                ctx2_p->bufEnd = ctx2_p->buffer;
            }
            ctx2_p->bufEnd++;
            ch = *(ctx2_p->bufEnd);
        }
        ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
        ret =  SceThreadmgrForDriver_91382762();
        if ( (ret == 0) || (ctx2_p->unk0 & 1) || (ch == '\0') ){
            data_p[len] = ch;
            len++;
        }
        else {
            if (ch != '\r'){
                if (ch == '\n'){
                    data_p[len] = '\n';
                    len++;
                    break;   
                }
                if (ch != '\x08' /* BackSpace */ && ch != '\x7F' /* DEL */){
                    data_p[len] = ch;
                    len++;
                    continue;
                }
                if (len == 0) continue;
                len--;

                //here it does some strange fuckery
                //maybe it should be commented out
                //i am not 100% sure because it uses a lot of duplicate variables and it's very hard to keep track
                char* chp2 = &data_p[len];
                if ( (data_p[len] & 0x80) && ((data_p[len] & 0xC0) == 0x80) ){
                    int unk = ctx2_p->unk0 & 0x1;
                    int lencpy = len - 1;
                    do {
                        unk++;
                        if ( ((unk == 5) && (len <= unk)) || (unk > 4) )
                            break;
                        ch = chp2[-2];
                        chp2--;
                    } while ((ch & 0xC0) == 0x80);
                    if (unk != 0 && unk != 6){
                        len = 1 << (5 - (unk - 1));
                        if ( (len ^ ((len - 1) | ch)) != 0xFF){
                            data_p[len] = ch;
                            len++;
                            continue;
                        }
                        len = (lencpy - unk) - 1;
                        continue;
                    }
                }
                //strange fuckery ends here
            }
        }
        
    }
    ksceKernelUnlockMutex(ctx1[port_num].readMutex, 1);
    return len;
    
}

size_t FUN_81001208(SceVfsRead* args){ //VFS_Read
    int ret = ksceVfsNodeSetEventFlag(args->node);
    if (ret >= 0){
        int ret2 = VfsReadInternal((int)args->node->dev_info, args->data, args->size, 0);
        ret = ksceVfsNodeWaitEventFlag(args->node);
        //asm("ands.w r0, r0, r0, asr #32")
        //asm("it cc")
        //asm("mov r5, r0") //r5 stores return of ksceVfsNodeWaitEventFlag
        //then it returns
        if (ret >= 0) 
            return ret2;
    }
    return ret;
}

size_t VfsWriteInternal(int port_number, const void* data, size_t size, uint32_t* a4){
    if (size < 0)
        return SCE_ERROR_ERRNO_EINVAL;

    int ret = ksceKernelLockMutex(ctx1[port_number].writeMutex, 1, NULL);
    if (ret < 0)
        return ret;

    if (size == 0) { //This check is originally done in each if case. But it doesn't matter (I think).
        ksceKernelUnlockMutex(ctx1[port_number].writeMutex, 1);
        return 0;
    }

    int32_t prodMode[2] = {0};
    const char* buf = data;
    kscePmMgrGetProductMode(prodMode);
    if (prodMode[0] == 0){
        //init ctx3 (if not already done)
        Deci4pTTY_ctx2* ctx2_p = ctx1[port_number].ctx2_ptr;
        if ( (ctx1[port_number].unk54 & 1) && (ctx2_p->unk0 & 1) ){
            do {
                ret = ksceKernelWaitSema(SceDeci4pTtyp_global_sema, 1, NULL);
                if (ret < 0)
                    break;
                ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
                if (ret < 0)
                    break;

                int firstNonInitIdx;
                if (ctx3[0].hasBeenInit)
                    if (ctx3[1].hasBeenInit)
                        if (ctx3[2].hasBeenInit){
                            ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                            break;
                        }
                        else firstNonInitIdx = 2;
                    else firstNonInitIdx = 1;
                else firstNonInitIdx = 0;
                
                Deci4pTTY_ctx3* ctx3_p = &ctx3[firstNonInitIdx];
                ctx3_p->hasBeenInit = 1;
                ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                ctx3_p->unk12 = 0;

                ret = SceThreadmgrForDriver_91382762();
                if (ret == 0){
                    if (a4 == NULL){
                        ctx3_p->unk10 = 0x8000;
                        ctx3_p->unk14 = (SceUID)NULL;
                        ctx3_p->unk18 = 0;
                    }
                    else {
                        ctx3_p->unk10 = 0x1000;
                        ctx3_p->unk14 = (SceUID)a4;
                        ctx3_p->unk18 = 0;
                    }
                }
                else {
                    ctx3_p->unk10 = 0x1000;
                    SceKernelFaultingProcessInfo fpi;
                    ksceKernelGetFaultingProcess(&fpi);
                    ctx3_p->unk14 = fpi.pid;
                    ret = SceThreadmgrForDriver_F311808F(fpi.unk);
                    ctx3_p->unk18 = ret;
                }

                /* if (some_stack_var_set_to_0 != 0) {
                    //returns things to r0 and r1 it seems
                    r0,r1 = ScePamgrForDriver_FE9BFD27();
                    ctx3_p->unk20 = r0;
                    ctx3_p->unk24 = r1;
                } else */
                ctx3_p->unk20 = 0;
                ctx3_p->unk24 = 0;

                size_t used_size = (size > 0xFFF) ? 0x1000 : size;
                memcpy(ctx3_p->unk2C, buf, used_size); //From SceSysclibForDriver
                SceDeci4pDfMgrForDebugger_6D26CC56(ctx3_p, used_size + 0x2C);
                ctx3_p->SceDeci4pDfMgrForDebugger_529979FB_handle = ctx1[port_number].SceDeci4pDfMgrForDebugger_529979FB_handle;
                ctx3_p->unk28 = used_size;
                ctx3_p->hasBeenInit = 2;
                ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
                if (ret >= 0){
                    Deci4pTTY_ctx3** ctx1_some_ptr = &ctx1[port_number].associated_ctx3;
                    (*ctx1_some_ptr)->unk_1030 = ctx3_p;
                    *ctx1_some_ptr = ctx3_p;
                    ctx3_p->unk_1030 = NULL;
                    ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                }
                ret = SceDeci4pDfMgrForDebugger_BADEF855(ctx1[port_number].SceDeci4pDfMgrForDebugger_529979FB_handle);
                if (ret < 0 && ret != 0x80080007){
                    if (ret == 0x8008000A){
                        ctx2_p->unk0 |= 0x2;
                    }
                    else {
                        ctx1[port_number].unk54 = (ctx1[port_number].unk54 & 0xFFFFFFFE);
                    }
                    ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
                    if (ret >= 0){
                        Deci4pTTY_ctx3* ptr3 = ctx1[port_number].ctx3.unk_1030;
                        if (ptr3 != NULL){
                            if (ptr3 != ctx3_p){
                                //walk through some linked list thing ?
                                Deci4pTTY_ctx3 *pcv7, *ncptr = ptr3;
                                do {
                                    pcv7 = ncptr;
                                    ncptr = pcv7->unk_1030;
                                    if (ncptr == NULL) break;
                                    ptr3 = pcv7;
                                } while(ncptr != ctx3_p);
                                if (ncptr != NULL){
                                    ptr3->unk_1030 = ctx3_p->unk_1030;
                                    if (ncptr->unk_1030 == NULL)
                                        ctx1[port_number].associated_ctx3 = &ctx1[port_number].ctx3;
                                    ctx3_p->unk_1030 = NULL;
                                }
                            }
                            else {
                                ptr3->unk_1030 = ctx3_p->unk_1030;
                                if (ctx3_p->unk_1030 == NULL)
                                    ctx1[port_number].associated_ctx3 = &ctx1[port_number].ctx3;
                                ctx3_p->unk_1030 = NULL;
                            }
                        }
                        ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                    }
                    int state = ksceKernelCpuSuspendIntr(&SuspendIntr_param);
                    memset(ctx3_p, 0, sizeof(Deci4pTTY_ctx3));
                    ksceKernelCpuResumeIntr(&SuspendIntr_param, state);
                    ksceKernelSignalSema(SceDeci4pTtyp_global_sema, 1);
                }
                buf += used_size;
                size -= used_size;
                if ( (size == used_size) ||  !(ctx1[port_number].unk54 & 0x1) || ((uintptr_t)ctx2_p & 1) )
                    break;
            } while (1);
            buf = data;
        }

        //do the actual printing
        for (; (uintptr_t)buf < ((uintptr_t)data + size); buf++)
            SceDebugForKernel_254A4997(*buf);
    }
    else if (prodMode[0] == 1){
        size_t len = 0;
        do {
            if ( ((len + 1) < size) && (buf[len] == '\\') && (buf[len+1] == 'n')){
                ksceDebugPrintf("\n");
                len += 2;
            } else {
                ksceDebugPrintf("%c", buf[len]);
                len++;
            }
        } while(len < size);
    }
    else
        ksceDebugPrintf("unknown mode %x\n", prodMode[0]);
    
    ksceKernelUnlockMutex(ctx1[port_number].writeMutex, 1);
    return size;
}

size_t FUN_810011C8(SceVfsWrite* args){ //VFS_Write
    int ret = ksceVfsNodeSetEventFlag(args->node);
    if (ret >= 0){
        int ret2 = VfsWriteInternal((int)args->node->dev_info, args->data, args->size, *(uint32_t**)(args->objectBase + 0x20));
        ret = ksceVfsNodeWaitEventFlag(args->node);
        //asm("ands.w r0, r0, r0, asr #32")
        //asm("it cc")
        //asm("mov r5, r0") //r5 stores return of ksceVfsNodeWaitEventFlag
        //then it returns
        if (ret >= 0)
            return ret2;
    }
    return ret;
}

int FUN_810012F8(SceVfsIoctl* args){ //VFS_Ioctl
    if ( (uint32_t)args->node->dev_info > MAX_TTY_NUM)
        return SCE_ERROR_ERRNO_EINVAL;

    int ret = ksceKernelLockMutex(SceDeci4pTtyp_global_mutex, 1, NULL);
    if (ret < 0)
        return ret;

    if (args->unk8 == 0x6400 && args->unk10 == 8) { //Allocate and copy thing
        uint32_t* dstp = (args->objectBase + 0x20), *srcp = args->unkC;
        if (dstp == NULL){
            dstp = ksceKernelAllocHeapMemory(SceDeci4pTtyp_global_heap, 0x8);
            if (dstp == NULL) {
                ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
                return SCE_ERROR_ERRNO_ENOMEM;
            }
            else
                *(uint32_t**)(args->objectBase + 0x20) = dstp;
        }
        //Maybe optimized-out memcpy
        dstp[0] = srcp[0];
        dstp[1] = srcp[1];
        ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
        return ret; //Maybe return 0 ?
    } 
    else if (args->unk8 == 0x6401){ //Free thing
        void* p = (args->objectBase + 0x20);
        if (p != NULL) {
            ksceKernelFreeHeapMemory(SceDeci4pTtyp_global_heap, p);
            *(uint32_t**)p = NULL;
        }
        ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
        return ret; //Maybe return 0 ?
    }
    else {
        ksceKernelUnlockMutex(SceDeci4pTtyp_global_mutex, 1);
        return SCE_ERROR_ERRNO_EINVAL;
    }
}

int FUN_810011B0(){
    return 0;
}

SceVfsTable vfs_func_table_1 = {
    .func00 = FUN_810011B4,
    .func04 = FUN_81001178,
    .func08 = FUN_8100117C,
    .func0C = NULL,
    .func10 = NULL,
    .func14 = NULL,
    .func18 = NULL,
    .func1C = NULL,
    .func20 = FUN_81001170,
    .func24 = FUN_81001174,
    .func28 = NULL,
    .func2C = NULL,
    .func30 = NULL
};

SceVfsTable2 vfs_func_table_2 = {
    .func00 = FUN_810012EC,
    .func04 = NULL,
    .func08 = FUN_810012C4,
    .func0C = FUN_8100123C,
    .func10 = FUN_81001208,
    .func14 = FUN_810011C8,
    .func18 = NULL,
    .func1C = FUN_810012F8,
    .func20 = NULL,
    .func24 = NULL,
    .func28 = NULL,
    .func2C = NULL,
    .func30 = NULL,
    .func34 = NULL,
    .func38 = NULL,
    .func3C = NULL,
    .func40 = NULL,
    .func44 = NULL,
    .func48 = NULL,
    .func4C = NULL,
    .func50 = FUN_810011B0,
    .func54 = NULL,
    .func58 = NULL,
    .func5C = NULL,
    .func60 = NULL,
    .func64 = NULL,
    .func68 = NULL,
    .func6C = NULL,
    .func70 = NULL,
};