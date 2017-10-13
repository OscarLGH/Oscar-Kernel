#include "Lib/Memory.h"
#include "SystemParameters.h"

typedef struct Bucket_Desc
{
	void * Page;
	struct Bucket_Desc * Next;
	void * FreePtr;
	UINT16 RefCnt;
	UINT16 BucketSize;
}BucketDesc;

typedef struct
{
	int Size;
	BucketDesc * Chain;
}BucketDir;

BucketDir BucketDirs[] = {
	{0x10,		(BucketDesc *)0},
	{0x20,		(BucketDesc *)0},
	{0x40,		(BucketDesc *)0},
	{0x80,		(BucketDesc *)0},
	{0x100,		(BucketDesc *)0},
	{0x200,		(BucketDesc *)0},
	{0x400,		(BucketDesc *)0},
	{0x800,		(BucketDesc *)0},
	{0x1000,	(BucketDesc *)0},
	{0x2000,	(BucketDesc *)0},
	{0x4000,	(BucketDesc *)0},
	{0x8000,	(BucketDesc *)0},
	{0x10000,	(BucketDesc *)0},
	{0x20000,	(BucketDesc *)0},
	{0x40000,	(BucketDesc *)0},
	{0x80000,	(BucketDesc *)0},
	{0x100000,	(BucketDesc *)0},
	{0x200000,	(BucketDesc *)0},
	{0,			(BucketDesc *)0}
};

BucketDesc * free_bucket_desc = (BucketDesc *)0;

static void InitBucketDesc()
{
	BucketDesc *bdesc,*first;
	int i;
	
	VOID * PhyPage,* VirtPage;
	
	PhyPage = AllocatePage();
	VirtPage = (VOID *)PHYS2VIRT(PhyPage);
	//MapPage((void *)VirtPage,(void *)PhyPage,2,PG_RWW | PG_USS | PG_P);
	
	first = bdesc = (BucketDesc *)VirtPage;
	
	//printk("bdesc:%x\n",bdesc);
	
	if(!bdesc)
		KDEBUG("Out of memory!\n");
		
	for(i = PAGE_SIZE / sizeof(BucketDesc); i > 1; i--)
	{
		bdesc->Next = bdesc + 1;
		bdesc++;
	}
	
	bdesc->Next = free_bucket_desc;
	free_bucket_desc = first;
}

SPIN_LOCK MallocSpinLock = { 0 };
void * KMalloc(UINT64 Len)
{
	BucketDir * BDir;
	BucketDesc *BDesc;
	void *retval;
	VOID * PhyPage, *VirtPage;
	
	for(BDir = BucketDirs; BDir->Size; BDir++)
		if(BDir->Size >= Len)
			break;	
	
	//printk("BDir->Size = %d\n",BDir->Size);
	
	if (!BDir->Size)
	{
		KDEBUG("Large allocation.\n");
		PhyPage = AllocatePage();
		VirtPage = (VOID *)PHYS2VIRT(PhyPage);
		int i = Len / PAGE_SIZE + 1;
		while (i--)
		{
			AllocatePage();
		}
		return VirtPage;
	}
		
		
	SpinLockIrqSave(&MallocSpinLock);
	
	for(BDesc = BDir->Chain; BDesc; BDesc = BDesc->Next)
		if(BDesc->FreePtr)
			break;

	if(!BDesc)
	{
		char * cp;
		int i;
		//KDEBUG("%s:%d:%x\n",__FUNCTION__,__LINE__,cp);
		//KDEBUG("&free_bucket_desc=%x\n",&free_bucket_desc);
		//KDEBUG("free_bucket_desc=%x\n",free_bucket_desc);
		if(!free_bucket_desc)
			InitBucketDesc();
		
		PhyPage = AllocatePage();
		VirtPage = (VOID *)PHYS2VIRT(PhyPage);
		//MapPage((void *)VirtPage,(void *)PhyPage,2,PG_RWW | PG_USS | PG_P);
		//while(1);
		BDesc = free_bucket_desc;
		free_bucket_desc = BDesc->Next;
		BDesc->RefCnt = 0;
		BDesc->BucketSize = BDir->Size;
		BDesc->Page = BDesc->FreePtr = cp = (void *)VirtPage;
		
		//KDEBUG("BDesc->Page = %x\n",BDesc->Page);
		
		
		if(!cp)
			KDEBUG("Out of memory.\n");
		
		for(i = PAGE_SIZE / BDir->Size; i > 1; i--)
		{
			*((char **)cp) = cp + BDir->Size;
			cp += BDir->Size;
		}
		
		*((char **)cp) = 0;
		BDesc->Next = BDir->Chain;
		BDir->Chain = BDesc;
	}
	
	retval = (void *)BDesc->FreePtr;
	BDesc->FreePtr = *((void **)retval);
	BDesc->RefCnt++;
	
	//KDEBUG("BDesc->FreePtr:%x\n",BDesc->FreePtr);
	memset(retval, 0, Len);
	
	SpinUnlockIrqRestore(&MallocSpinLock);
	
	/*之前这里直接打开了中断导致使用TSS.RSP0时在唤醒第一个线程发生莫名其妙的异常.
	可能调试的时候中断里把保存寄存器的动作注释了?所以给分配了一个错误的地址？
	2017.1.5 可能是GCC的red-zone导致的问题，中断处理程序覆盖了函数局部变量导致异常.内核代码不能使用red-zone.
	*/

	//KDEBUG("retval = %x\n",retval);
	
	return retval;
}

void KFree(void *Obj)
{
	BucketDir * BDir;
	BucketDesc *BDesc,*Prev;
	void * Page;
	
	Page = (void *)((UINT64)Obj&0xFFFFFFFFFFE00000);
	
	for(BDir = BucketDirs; BDir->Size; BDir++)
	{
		for(BDesc = BDir->Chain; BDesc; BDesc = BDesc->Next)
			if(BDesc->Page == Page)
				goto found;
			Prev = BDesc;
	}
	KDEBUG("Bad address passed to kernel\n");
found:	
	DisableLocalIrq();
	*((void **)Obj) = BDesc->FreePtr;
	BDesc->FreePtr = Obj;
	BDesc->RefCnt--;
	
	if(BDesc->RefCnt == 0)
	{
		if((Prev && (Prev->Next != BDesc)) || (!Prev && (BDir->Chain != BDesc)))
			for(Prev = BDir->Chain; Prev; Prev = Prev->Next)
				if(Prev->Next == BDesc)
					break;
		
		if(Prev)
			Prev->Next = BDesc->Next;
		else
		{
			if(BDir->Chain != BDesc)
				KDEBUG("Malloc bucket chains corrupted.\n");
			BDir->Chain = BDesc->Next;
		}
		KDEBUG("Page = %016x\n", BDesc->Page);
		FreePage((VOID *)VIRT2PHYS(BDesc->Page));
		BDesc->Next = free_bucket_desc;
		free_bucket_desc = BDesc;
	}
	ResumeLocalIrq();
	return;
}