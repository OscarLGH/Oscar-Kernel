#ifndef FIFO_CHANNEL_H
#define FIFO_CHANNEL_H

#include "Vram.h"
#include "../NvidiaChannel.h"

typedef struct _NV_FIFO
{
	NV_OBJECT Object;

	//NV_ENGINE Engine;
	UINT64 SpinLock;
	UINT64 Nr;

	struct
	{
		UINT64 Engm;
		UINT64 Runm;
	}Recover;

	UINT64 PbDmaCnt;

	struct
	{
		//NV_ENGINE * Engine;
		UINT64 Runl;
		UINT64 PbId;
	}Engine[16];

	UINT64 EngineCnt;

	struct
	{
		NV_GPU_OBJ * Mem[2];
		UINT64 Next;
		UINT64 Engm;
	}RunList[64];

	UINT64 RunListCnt;

	struct
	{
		NV_GPU_OBJ * Mem;
		NV_MEM Bar;
	}User;

}NV_FIFO;


typedef struct _NV_FIFO_CHANNEL
{
	NV_OBJECT Object;

	UINT64 Engines;
	UINT64 ChannelId;
	NV_GPU_OBJ * Inst;
	NV_GPU_OBJ * Push;
	VOID * User;
	UINT64 Addr;
	UINT64 Size;

	NV_FIFO * Fifo;
	UINT64 RunL;

	NV_GPU_OBJ *Pgd;

	struct
	{
		NV_GPU_OBJ *Inst;
		VIRTUAL_MEMORY_AREA *Vma;
	}Engn[4096];

}NV_FIFO_CHANNEL;

#endif