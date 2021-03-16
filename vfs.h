
#ifndef VFS_H
#define VFS_H

#include <psp2kern/kernel/iofilemgr/dirent.h>
//Reverse by Princess of Sleeping
//see : https://github.com/Princess-of-Sleeping/vita-utility

typedef struct SceVfsMount2 { // size is 0x14
	const char *unit;     // ex:"host0:"
	const char *device1;  // ex:"bsod_dummy_host_fs"
	const char *device2;  // ex:"bsod_dummy_host_fs"
	int data_0x0C;        // ex:0
	int data_0x10;        // ex:0
} SceVfsMount2;

typedef struct SceVfsMount {   // size is 0x20
	const char *device;    // ex:"/host"
	int data_0x04;
	int data_0x08;         // ex:0x03000004
	int data_0x0C;         // ex:0x00008006
	const char *data_0x10; // ex:"bsod_dummy_host_fs"
	int data_0x14;
	const SceVfsMount2 *data_0x18;
	int data_0x1C;
} SceVfsMount;

typedef struct SceVfsUmount {
	const char *device; // ex:"/host"
	int data_0x04;
} SceVfsUmount;

typedef struct SceVfsTable { // size is 0x34?
	const void *func00; 
	const void *func04;
	const void *func08; 
	const void *func0C; 
	const void *func10; 
	const void *func14; 
	const void *func18;
	const void *func1C;
	const void *func20;
	const void *func24;
	const void *func28;
	const void *func2C; //vfs_devctl
	const void *func30;
} SceVfsTable;

typedef struct SceVfsTable2 { // size is 0x74?
	const void *func00; //vfs_open
	const void *func04; 
	const void *func08; //vfs_close
	const void *func0C; //vfs_part_init
	const void *func10; //vfs_read
	const void *func14; //vfs_write
	const void *func18;
	const void *func1C; //vfs_ioctl
	const void *func20; //vfs_part_deinit
	const void *func24;
	const void *func28;
	const void *func2C; //vfs_dopen
	const void *func30; //vfs_dclose
	const void *func34; //vfs_dread
	const void *func38; //vfs_get_stat
	const void *func3C;
	const void *func40;
	const void *func44;
	const void *func48;
	const void *func4C;
	const void *func50;
	const void *func54;
	const void *func58;
	const void *func5C; //vfs_sync
	const void *func60; //vfs_get_stat_by_fd
	const void *func64;
	const void *func68;
	const void *func6C;
	const void *func70;
} SceVfsTable2;

typedef struct SceVfsAdd {     // size is 0x20
	const SceVfsTable *func_ptr1;
	const char *device;    // ex:"bsod_dummy_host_fs"
	int data_0x08;         // ex:0x11
	int data_0x0C;
	int data_0x10;         // ex:0x10
	const SceVfsTable2 *func_ptr2;
	int data_0x18;
	struct SceVfsAdd *prev;
} SceVfsAdd;

typedef struct SceVfsNode { //size is 0x100
	uint32_t unk_0;
	SceUID tid_4;                  // SceUID of thread in which node was created
	uint32_t some_counter_8;       // counter
	SceUID event_flag_SceVfsVnode; // 0xC - event flag SceVfsVnode

	uint32_t evid_bits;            // 0x10
	uint32_t unk_14;
	uint32_t unk_18;
	uint32_t unk_1C;

	uint8_t data1[0x20];

	void *ops;                     // 0x40
	uint32_t unk_44;
	void *dev_info;                // allocated on heap with uid from uid field
                                       // this is device specific / node specific data
                                       // for partition node this will be vfs_device_info*
                                       // for pfs node it looks like to be child_node_info* 
                                       // which probably points to partition node

	void *mount;                   // 0x4C

	struct SceVfsNode *prev_node;  // 0x50

	struct SceVfsNode *unk_54;     // copied from node base - singly linked list
	uint32_t some_counter_58;      // counter - probably counter of nodes
	void *unk_5C;

	uint32_t unk_60;
	uint32_t unk_64;
	uint32_t unk_68;
	SceUID pool_uid;               // 0x6C - SceIoVfsHeap or other pool

	void *unk_70;
	uint32_t unk_74;               // = 0x8000
	uint32_t unk_78;               // some flag
	uint32_t unk_7C;

	uint32_t unk_80;
	uint32_t unk_84;
	uint32_t unk_88;
	uint32_t unk_8C;

	void *obj_internal_90;
	uint32_t maybe_internal_obj_count_94;
	struct SceVfsNode *child_node; // child node with deeper level of i/o implementation?
	uint32_t unk_9C;

	uint8_t data2[0x30];
   
	uint32_t unk_D0;               // devMajor.w.unk_4E

	uint8_t data3[0x2C];
} SceVfsNode;

int ksceVfsGetNewNode(void *mount, int a2, int a3, SceVfsNode **ppNode);
int ksceVfsNodeWaitEventFlag(SceVfsNode *pNode);

typedef struct SceVfsPath { //size is 0xC
	char *path;
	SceSize path_len;
	char *path2;
} SceVfsPath;

typedef struct SceVfsPartInit {
	SceVfsNode  *node; 
	SceVfsNode **new_node; // result
	void *opt;
	uint32_t flags; 
} SceVfsPartInit;

typedef struct SceVfsDevctl {
	void *unk_0x00;
	const char *dev;
	unsigned int cmd;
	void *indata;
	SceSize inlen;
	void *outdata;
	SceSize outlen;
} SceVfsDevctl;

typedef struct SceVfsOpen {
	SceVfsNode *node;
	SceVfsPath *path_info;
	int flags;
	void *opt;
	int unk_0x10;
} SceVfsOpen;

typedef struct SceVfsClose {
	SceVfsNode *node;
	void *objectBase;
} SceVfsClose;

typedef struct SceVfsRead { // size is 0x14?
	SceVfsNode *node;
	int data_0x04;
	void *data;
	SceSize size;
	int data_0x10; // ???
} SceVfsRead;

typedef struct SceVfsWrite {
	SceVfsNode *node;
	void *objectBase;
	const void *data;
	SceSize size;
} SceVfsWrite;

typedef struct SceVfsDopen {
	SceVfsNode *node;
	SceVfsPath *opts;
	void *objectBase;
} SceVfsDopen;

typedef struct SceVfsDclose {
	SceVfsNode *node;
	void *objectBase;
} SceVfsDclose;

typedef struct SceVfsDread { // size is 0xC
	SceVfsNode *vfs_node;
	void *data_0x04;
	SceIoDirent *dir;
} SceVfsDread;

typedef struct SceVfsStat {
	SceVfsNode *node;
	SceVfsPath *opts;
	SceIoStat *stat;
} SceVfsStat;

typedef struct SceVfsStatByFd {
	SceVfsNode *node;
	void *objectBase;
	SceIoStat *stat;
} SceVfsStatByFd;

int ksceVfsMount(const SceVfsMount *pVfsMount);
int ksceVfsUnmount(const SceVfsUmount *pVfsUmount);

int ksceVfsAddVfs(SceVfsAdd *pVfsAdd);
int ksceVfsDeleteVfs(const char *fs, void *a2); // "deci4p_drfp_dev_fs"

int ksceVfsNodeSetEventFlag(SceVfsNode* pNode);

//reverse by CreepNT
typedef struct _SceVfsIoctl{ //name not sure
	SceVfsNode* node;
	void* objectBase;
	uint32_t unk8; //request type maybe
	void* unkC;
	uint32_t unk10; //size of block pointed to by unkC maybe
} SceVfsIoctl;

#endif