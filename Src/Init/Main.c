#include "BaseLib.h"
#include "Driver.h"
#include "../Arch/X86_64/Cpu/Cpu.h"

#include "Thread.h"

#include "Partition.h"


//#define DEBUG
#include "Debug.h"
#ifdef DEBUG
#undef DEBUG
#endif


SYSTEM_PARAMETERS * SysParam = (SYSTEM_PARAMETERS*)SYSTEMPARABASE;


void TestMain();
void InitMSG();

extern CONSOLE SysCon;
extern int ThreadCnt;
extern THREAD_CONTROL_BLOCK ThreadTable[256];
extern UINT64 ClockTicks;

void testcolor(void);
void * testprocess(void *);


void InitMSG()
{
	UINT8 Buffer[64] = { 0 };
	ArchReadCpuId(&Buffer[0]);
	
	printk("Cpu:%02d %s\n", ArchGetCpuIdentifier(), Buffer);
}


void ClearBSS(void)
{
	extern int __bss_start, __bss_end;
	int *p = &__bss_start;
	for (; p < &__bss_end; p++)
		*p = 0;
}

void DoInitCall(void)
{
	extern INIT_CALL_FUNC __initcall_start[], __initcall_end[];
	INIT_CALL_FUNC *pfunc;
	for (pfunc = __initcall_start; pfunc < __initcall_end; pfunc++)
	{
		if (*pfunc)
		{
			(*pfunc)();
			//ThreadCreate(NULL, (Func *)*pfunc, NULL);
		}
	}
}


void * MainThreadTest(void * arg);

void * KernelInitThread(void * Arg)
{
	UINT64 Tid = 0;
	THREAD_CONTROL_BLOCK ** ThreadArray = NULL;
	THREAD_CONTROL_BLOCK ** ThreadArrayPtr = NULL;
	THREAD_CONTROL_BLOCK * Current = GetCurrentThread();
	UINT64 ThreadCnt = 0;

	extern INIT_CALL_FUNC  \
		__initcall1_start, \
		__initcall1_end, \
		__initcall2_start, \
		__initcall2_end, \
		__initcall3_start, \
		__initcall3_end, \
		__initcall4_start, \
		__initcall4_end, \
		__initcall5_start, \
		__initcall5_end, \
		__initcall6_start, \
		__initcall6_end, \
		__initcall7_start, \
		__initcall7_end, \
		__initcall8_start, \
		__initcall8_end;

	struct INIT_CALL_ENRTY
	{
		INIT_CALL_FUNC * Start;
		INIT_CALL_FUNC * End;
	}InitArray[] = {
		{ &__initcall1_start, &__initcall1_end },
		{ &__initcall2_start, &__initcall2_end },
		{ &__initcall3_start, &__initcall3_end },
		{ &__initcall4_start, &__initcall4_end },
		{ &__initcall5_start, &__initcall5_end },
		{ &__initcall6_start, &__initcall6_end },
		{ &__initcall7_start, &__initcall7_end },
		{ &__initcall8_start, &__initcall8_end },
	};
	
	INIT_CALL_FUNC InitFunc;
	INIT_CALL_FUNC * InitArrayPtr;
	
	//while(1)
	printk("Initializing devices ...\n");
	
	UINT64 Index0, Index1;
	VOID * ReturnValue;
	UINT64 i = 0;
#if 1
	for (Index0 = 4; Index0 < 8; Index0++)
	{
		ThreadCnt = InitArray[Index0].End - InitArray[Index0].Start;
	
		ThreadArrayPtr = ThreadArray = KMalloc(ThreadCnt * sizeof(*ThreadArrayPtr));

		for (InitArrayPtr = InitArray[Index0].Start; InitArrayPtr < InitArray[Index0].End; InitArrayPtr++)
		{
			InitFunc = *InitArrayPtr;
			if (InitFunc)
			{
				ThreadCreate(ThreadArrayPtr, (Func *)InitFunc, NULL, KERNEL_THREAD, 10);
				//printk("Thread %d created.\n", (*ThreadArrayPtr)->TID);
				ThreadAddWakeup(*ThreadArrayPtr, Current);
				ThreadWakeUp(*ThreadArrayPtr);
				ThreadArrayPtr++;
			}
		}

		for (Index1 = 0; Index1 < ThreadCnt; Index1++)
		{
			ThreadJoin(ThreadArray[Index1], &ReturnValue);
		}
		
		//KFree(ThreadArray);
	}
#endif
	printk("Device initialization done.\n");
	
	ThreadArrayPtr = KMalloc(sizeof(*ThreadArrayPtr));
	ThreadCreate(ThreadArrayPtr, MainThreadTest, NULL, KERNEL_THREAD | CREATE_PROCESS, 10);
	ThreadWakeUp(*ThreadArrayPtr);
	//while (1);
	return 0;
}

void TestDev();
void * ThreadTest(void * p);

int tone_low[] = { 131,147,165,175,196,220,247 };
int tone_mid[] = { 262,294,330,349,392,440,494 };
int tone_high[] = { 523,588,660,698,784,880,988,1048 };
int tone[] = {
	131,147,165,175,196,220,247,
	262,294,330,349,392,440,494,
	523,588,660,698,784,880,988,
	1048,1176,1320,1396,1568,1760,1976,
	2096
};
typedef struct
{
	int m_tone[512];
	int duration[512];
}music;

music BadApple = {
	{ 2, 3, 4, 5, 6,  9,8, 6, 2,   6, 5, 4, 3,   2, 3, 4, 5, 6,   5, 4, 3, 2, 3, 4, 3, 2, 1, 3,
	2, 3, 4, 5, 6,  9,8, 6, 2,   6, 5, 4, 3,   2, 3, 4, 5, 6,   5, 4, 3, 4, 5, 6,
	2, 3, 4, 5, 6,  9,8, 6, 2,   6, 5, 4, 3,   2, 3, 4, 5, 6,   5, 4, 3, 2, 3, 4, 3, 2, 1, 3,
	2, 3, 4, 5, 6,  9,8, 6, 2,   6, 5, 4, 3,   2, 3, 4, 5, 6,   5, 4, 3, 4, 5, 6,
	8,9, 6, 5, 6,   5, 6,8,9, 6, 5, 6,   5, 6, 5, 4, 3, 1, 2,    1, 2, 3, 4, 5, 6, 2,
	5, 6,8,9, 6, 5, 6,   5, 6,8,9, 6, 5, 6,   5, 6, 5, 4, 3, 1, 2,    1, 2, 3, 4, 5, 6, 2,
	5, 6,8,9, 6, 5, 6,   5, 6,8,9, 6, 5, 6,   5, 6, 5, 4, 3, 1, 2,    1, 2, 3, 4, 5, 6, 2,
	5, 6,8,9, 6, 5, 6,   5, 6,8,9, 6, 5, 6,  13,14,15,14,13,12,10,   9,10,9,8, 7, 5, 6,0 },
	{ 1, 1, 1, 1, 2,   1, 1, 2, 2,   1, 1, 1, 1,   1, 1, 1, 1, 2,   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 2,   1, 1, 2, 2,   1, 1, 1, 1,   1, 1, 1, 1, 2,   1, 1, 2, 2, 2, 2,
	1, 1, 1, 1, 2,   1, 1, 2, 2,   1, 1, 1, 1,   1, 1, 1, 1, 2,   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 2,   1, 1, 2, 2,   1, 1, 1, 1,   1, 1, 1, 1, 2,   1, 1, 2, 2, 2, 2,
	1, 1, 1, 1, 2,   1, 1, 1, 1, 1, 1, 2,   1, 1, 1, 1, 1, 1, 2,    1, 1, 1, 1, 1, 1, 2,
	1, 1, 1, 1, 1, 1, 2,   1, 1, 1, 1, 1, 1, 2,   1, 1, 1, 1, 1, 1, 2,    1, 1, 1, 1, 1, 1, 2,
	1, 1, 1, 1, 1, 1, 2,   1, 1, 1, 1, 1, 1, 2,   1, 1, 1, 1, 1, 1, 2,    1, 1, 1, 1, 1, 1, 2,
	1, 1, 1, 1, 1, 1, 2,   1, 1, 1, 1, 1, 1, 2,   1, 1, 1, 1, 1, 1, 2,    1, 1, 1, 1, 1, 1, 2,0 }
};
music AmericanPetro = {
	{ 3,4, 5,5,  5,4,5,6,  7,7,  7,6,7,8,  9,9,  9,8,9,12,9,
	7,8,8,7,6,8,  7,7,6,5,7,  6,3,4,5,6,
	3,4, 5,5,  5,4,5,6,  7,7,  7,6,7,8,  9,9,  9,8,9,12,9,
	7,10,9,8,7,6,5,4,5, 6,7,8,7,6,5,
	2,3,4,5,6,7, 8,8,8,8,8,7,8,  8,9,9,9,8,7,8,9,
	7,8,8,8,8,8,7,8,  8,9,9,9,8,7,8,9,

	8,10,10,12,10,9,9,9,8,7,   8,8,7,6,8, 7,7,7,6,5,
	10,10,12,10,9,9,9,8,7,    8,8,6,4,6,    5,5,5,
	0 },
	{ 1,1, 2,2,  1,1,1,1,  2,2,  1,1,1,1,   2,2,     1 ,1 ,1 ,1 ,5,
	2, 2,1,1,2,2,    2,1,1,2,2,  2,2,2,2,5,
	1,1, 2,2,  1,1,1,1,  2,2,  1,1,1,1,   2,2,     1 ,1 ,1 ,1 ,5,
	2, 2, 2, 2,2,2,2,2,2,2,1,1,2,2,2,
	1,1,1,1,1,1,  2, 2, 2, 2, 1,1, 5,   2, 2, 2, 1, 1, 1, 1,5,
	2,2, 2, 2, 2, 1,1, 5,   2, 2, 2, 1, 1, 1, 1,5,

	2, 2, 2, 2, 2, 2, 2, 1, 1,1,    2, 1,1,2, 2, 2,2,1,1,1,
	2, 2, 2, 2, 2, 2, 1, 1,1,	2, 1,1,2, 2, 2,2,2,

	0 },
};

void * BeepMusicTest(void * arg)
{
	int i, temp;
	while (1)
	{
		i = 0;
		
		while (1)
		{
			temp = AmericanPetro.m_tone[i];
			if (AmericanPetro.duration[i] == 0)
				break;
			if (temp<0)
				temp = -temp + 14 - 1;
			else
				temp = temp + 14 - 1;
			//Speaker(tone[temp],AmericanPetro.duration[i]*125);
			BeepOn(tone[temp]);
			MilliSecondDelay(AmericanPetro.duration[i] * 125);
			BeepOff();

			i++;
		}
		MilliSecondDelay(1000);
		i = 0;
		
		while (1)
		{
			temp = BadApple.m_tone[i];
			if (BadApple.duration[i] == 0)
				break;
			if (temp<0)
				temp = -temp + 14 - 1;
			else
				temp = temp + 14 - 1;
			//Speaker(tone[temp],BadApple.duration[i]*200);
			BeepOn(tone[temp]);
			MilliSecondDelay(BadApple.duration[i] * 200);
			BeepOff();
			i++;
		}

		MilliSecondDelay(1000);
	}
}

void * ColorTest(void * arg)
{
	UINT64 i, Color = 0x00ff0000;
	INT64 k;
	//SysCon.FbDevice->FillArea(SysCon.FbDevice, NULL, 0x00ff00ff);
	RECTANGLE Rect = { 0 };
	UINT64 Incresement = 0x4;
	if (SysCon.FbDevice->FillArea == NULL)
		return NULL;

	Rect.PosX = 1600;
	Rect.PosY = 0;
	Rect.Width = 200;
	Rect.Height = 100;
	//while (1)
	//	printk("Color test");
	while (1)
	{
		k = 256;
		while (k > Incresement)
		{
			Color += (Incresement << 8);
			SysCon.FbDevice->FillArea(SysCon.FbDevice, &Rect, Color);
			k -= Incresement;
		}
		k = 256;
		while (k > Incresement)
		{
			Color -= (Incresement << 16);
			SysCon.FbDevice->FillArea(SysCon.FbDevice, &Rect, Color);
			k -= Incresement;
		}
		k = 256;
		while (k > Incresement)
		{
			Color += Incresement;
			SysCon.FbDevice->FillArea(SysCon.FbDevice, &Rect, Color);
			k -= Incresement;
		}
		k = 256;
		while (k > Incresement)
		{
			Color -= (Incresement << 8);
			SysCon.FbDevice->FillArea(SysCon.FbDevice, &Rect, Color);
			k -= Incresement;
		}
		k = 256;
		while (k > Incresement)
		{
			Color += (Incresement << 16);
			SysCon.FbDevice->FillArea(SysCon.FbDevice, &Rect, Color);
			k -= Incresement;
		}
		k = 256;
		while (k > Incresement)
		{
			Color -= Incresement;
			SysCon.FbDevice->FillArea(SysCon.FbDevice, &Rect, Color);
			k -= Incresement;
		}
	}
}

void * MainThreadTest(void * arg)
{
	int x;
	int y;
	int color;
	THREAD_CONTROL_BLOCK * tid=0;
	THREAD_CONTROL_BLOCK * Current = GetCurrentThread();
	int i = 0;
	int counter0 = 0;

	UINT64 *array;
	UINT64 * TestPtr = (UINT64 *)0x1000;
	
	printk("Main Thread Running.\n");

	color = GetColor(&SysCon);
	//while(1);
	#define TestThreadCnt 10
	THREAD_CONTROL_BLOCK * TIDArray[TestThreadCnt] = {0};
	for(i = 0; i < TestThreadCnt; i++)
	{
		
		//printk("array = %016x\n",KMalloc(sizeof(int)));
		array = KMalloc(sizeof(UINT64));		//没有包含函数头文件，无函数声明，从而返回值高位被截断
		//array = testret();
		//printk("sizeof(array):%d\n",sizeof(array));
		//printk("array:%016x\n",array);
		//array = array00;
		*array = i;
	
		ThreadCreate(&tid, ThreadTest, array, KERNEL_THREAD, 10);	//Must use the global array or heapmemory to pass the parameter.See CSAPP:P983
		TIDArray[i] = tid;
		ThreadAddWakeup(tid, Current);
		ThreadWakeUp(tid);
		
		//printk("array = %x,*array = %d\n",array,*array);
		printk("Creating Sub Thread:ThreadTest,TID = %d",tid->TID);
		//while(1);
		
		GetXY(&SysCon,&x,&y);
		SetXY(&SysCon,8*50,y);
		SetColor(&SysCon,0x0000ff00);
		printk("[OK]\n");
		
		SetColor(&SysCon,color);
	}
	
	void * retvalue;

	printk("Main Thread is waiting subthreads to terminate...\n");
	for(i = 0; i < TestThreadCnt; i++)
	{
		ThreadJoin(TIDArray[i],&retvalue);
		printk("Thread %d terminated with return value %x.\n", TIDArray[i]->TID, retvalue);
	}
	
	color = 0xffffffff;
	x = 0;
	y = 44;
	SetColor(&SysCon,0xFF00FFFF);
	printk("Main Thread Continues Running:\n");

	void VmTest();
	VmTest();

	printk("Timer test 5s...\n");
	int cnt = 5;
	while(cnt--)
	{
		MilliSecondDelay(1000);
		printk("%d ", cnt);
	}
	printk("\n");
	
	ThreadCreate(&tid, ColorTest, array, KERNEL_THREAD, 10);
	ThreadWakeUp(tid);
	//printk("ColorTestThread = %x\n", tid);
	//ThreadCreate(&tid, (void *(*)(void *))TestDev, array, KERNEL_THREAD, 10);
	//ThreadWakeUp(tid);

	//ThreadCreate(&tid, (void *(*)(void *))BeepMusicTest, array, KERNEL_THREAD, 10);
	//ThreadWakeUp(tid);
	
	//Out8(0x64, 0xFE);
	//asm("hlt");
	while(1)
	{
		
		//printk("test string:%d\n", counter0++);
		//print_int_at(counter0,x+50,y,color);
		//counter0++;
		UINT16 Key = GetKey();
		switch (Key)
		{
		case 0x0103:
			printk("\n");
			break;
		case 0x2920:
			printk("Rebooting in 3 seconds.\n");
			MilliSecondDelay(3000);
			Out8(0x64, 0xFE);
		default:
			printk("%c", Key);
			break;
		}
		
	}
	
	return 0;
}

void * ThreadTest(void * p)
{
	UINT64 parameter = (*(UINT64 *)p);
	UINT64 x = 60;
	UINT64 y = 2 + parameter;
	UINT32 colortable[] = { 0x000000FF,0x0000FF00,0x00FF0000,0x0000FFFF,0x00FFFF00,
		0x00FF00FF,0x00FF8000,0x0000FF80,0x008000FF,0x00B000B0,
		0x00B0B000,0x00FFB0B0,0x00B0FFB0,0x00123456,0x00654321 };
	UINT32 color = colortable[parameter & 0xFF];
	int APICID = 0xFFFFFFFF;
	long long counter0 = 0;
	int times = 0x1000 * (parameter + 1);
	//times = 0xFFFF;
	THREAD_CONTROL_BLOCK * tid;

	//printk("array0 = %x,array1 = %x,array2 = %x\n",array0,array1,array2);

	if (parameter == 6)
	{
		UINT64 * array0 = KMalloc(sizeof(UINT64));
		*array0 = 10;
		UINT64 * array1 = KMalloc(sizeof(UINT64));
		*array1 = 11;

		ThreadCreate(&tid, ThreadTest, array0, KERNEL_THREAD, 10);
		ThreadCreate(&tid, ThreadTest, array1, KERNEL_THREAD, 10);
	}
	if (parameter == 10)
	{
		UINT64 * array2 = KMalloc(sizeof(UINT64));
		*array2 = 12;
		ThreadCreate(&tid, ThreadTest, array2, KERNEL_THREAD, 10);
	}
	//if(parameter == 7)
	//	CreateProcess(testprocess,0);
	
	while (times--)
	{
		APICID = ArchGetCpuIdentifier();
		//print_str_at("ThreadID =",x,y,color);

		//print_int_at(tid->TID,x+10,y,color);
		print_int_at(counter0, x + 12, y, color);
		print_str_at("Cpu ID:", x + 24, y, color);
		print_int_at(APICID, x + 32, y, color);

		//	if(CpuStatusTable[GetCpuLogicNum()].RunningThread->ParentThread)
		//	{
		//		int PTID = CpuStatusTable[GetCpuLogicNum()].RunningThread->ParentThread->TID;
		//		print_int_at(PTID,x+52,y,color);
		//	}
		//	if(CpuStatusTable[GetCpuLogicNum()].RunningThread->ChildThread)
		//	{
		//		int CTID = CpuStatusTable[GetCpuLogicNum()].RunningThread->ChildThread->TID;
		//		print_int_at(CTID,x+55,y,color);
		//	}
		//	if(CpuStatusTable[GetCpuLogicNum()].RunningThread->NextThread)
		//	{
		//		int NTID = CpuStatusTable[GetCpuLogicNum()].RunningThread->NextThread->TID;
		//		print_int_at(NTID,x+58,y,color);
		//	}
		//	if(CpuStatusTable[GetCpuLogicNum()].RunningThread->PrevThread)
		//	{
		//		int PrTID = CpuStatusTable[GetCpuLogicNum()].RunningThread->PrevThread->TID;
		//		print_int_at(PrTID,x+61,y,color);
		//	}
		//	
		counter0++;
	}
	return (void *)parameter;
}

void TestDev()
{
	DISK_NODE * Disk;
	NETCARD_NODE * NetCard;
	I2CNODE * I2CDevice;
	UINT8 i;
	UINT8 Buffer[64*1024];

	UINT64 Index;
	UINT64 Value;

	RETURN_STATUS Status = 0;

	/*UINT8 StartupCommand[] = {0x80,0x01,0x00,0x00,0x00,0x0C,0x00,0x00,0x01,0x44,0x00,0x00};
	UINT8 Selftest1Command[] = {0x80,0x01,0x00,0x00,0x00,0x0B,0x00,0x00,0x01,0x43,0x00};
	UINT8 Selftest2Command[] = {0x80,0x01,0x00,0x00,0x00,0x0B,0x00,0x00,0x01,0x43,0x01};
	UINT8 GetversionCommand[] = {0x80,0x01,0x00,0x00,0x00,0x16,0x00,0x00,0x01,0x7A,0x00,0x00,0x00,0x06,0x00,0x00,0x01,0x0B,0x00,0x00,0x00,0x01};
	UINT8 GetAlgCommand[] = {0x80,0x01,0x00,0x00,0x00,0x16,0x00,0x00,0x01,0x7A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64};
	UINT8 ReadPcrCommand[] = {0x80,0x01,0x00,0x00,0x00,0x0E,0x00,0x00,0x01,0x7E,0x00,0x00,0x00,0x07};
  	UINT8 OutBuffer[2048] = {1,2,3,4,5,6,7,8,9,0xA};
	
	for(i=0; I2CDevice = I2COpen(i); i++)
	{
		printk("Sending Startup command to i2c TPM...\n");
		Status = I2CDevice->Operations->I2CWrite(I2CDevice, 0x58, StartupCommand, sizeof(StartupCommand));
		printk("Writing I2C Status = %08x\n", Status);
		MilliSecondDelay(100);
		memset(OutBuffer, 0xFF, 2048);
		Status = I2CDevice->Operations->I2CRead(I2CDevice, 0x58, OutBuffer, 0x6);
		if (OutBuffer[5] > 6)
			Status = I2CDevice->Operations->I2CRead(I2CDevice, 0x58, OutBuffer + 6, OutBuffer[5] - 6);
		else
			printk("Error reading I2C device!\n");

		DumpMem(OutBuffer, OutBuffer[5]);
		if(OutBuffer[6] | OutBuffer[7] | OutBuffer[8] | OutBuffer[9])
			printk("Startup i2c TPM failed!\n");
		else
			printk("Startup i2c TPM success!\n");

		printk("TPM 2.0 Get Version:\n");
		Status = I2CDevice->Operations->I2CWrite(I2CDevice, 0x58, GetversionCommand, sizeof(GetversionCommand));
		printk("Writing I2C Status = %08x\n", Status);
		MilliSecondDelay(100);
		memset(OutBuffer, 0xFF, 2048);
		Status = I2CDevice->Operations->I2CRead(I2CDevice, 0x58, OutBuffer, 0x6);
		if (OutBuffer[5] > 6)
			Status = I2CDevice->Operations->I2CRead(I2CDevice, 0x58, OutBuffer + 6, OutBuffer[5] - 6);
		else
			printk("Error reading I2C device!\n");
		printk("Reading I2C Status = %08x\n", Status);
		
		DumpMem(OutBuffer, OutBuffer[5]);

		printk("TPM 2.0 Get Algorithm:\n");
		Status = I2CDevice->Operations->I2CWrite(I2CDevice, 0x58, GetAlgCommand, sizeof(GetAlgCommand));
		printk("Writing I2C Status = %08x\n", Status);
		MilliSecondDelay(100);
		memset(OutBuffer, 0xFF, 2048);
		Status = I2CDevice->Operations->I2CRead(I2CDevice, 0x58, OutBuffer, 0x6);
		if (OutBuffer[5] > 6)
			Status = I2CDevice->Operations->I2CRead(I2CDevice, 0x58, OutBuffer + 6, OutBuffer[5] - 6);
		else
			printk("Error reading I2C device!\n");
		printk("Reading I2C Status = %08x\n", Status);
		
		DumpMem(OutBuffer, OutBuffer[5]);

		printk("TPM 2.0 Read Pcr:%d:\n",7);
		Status = I2CDevice->Operations->I2CWrite(I2CDevice, 0x58, ReadPcrCommand, sizeof(ReadPcrCommand));
		printk("Writing I2C Status = %08x\n", Status);
		MilliSecondDelay(100);
		memset(OutBuffer, 0xFF, 2048);
		Status = I2CDevice->Operations->I2CRead(I2CDevice, 0x58, OutBuffer, 0x6);
		if (OutBuffer[5] > 6)
			Status = I2CDevice->Operations->I2CRead(I2CDevice, 0x58, OutBuffer + 6, OutBuffer[5] - 6);
		else
			printk("Error reading I2C device!\n");
		printk("Reading I2C Status = %08x\n", Status);

		DumpMem(OutBuffer, OutBuffer[5]);
	}*/
	
	//asm("int $0x80");
	memset(Buffer, 0x11, 512);
	
	for (i = 0; Disk = DiskOpen(i); i++)
	{
		PartitionProbe(Disk);
	}

	for (i = 0; Disk = DiskOpen(i); i++)
	{
		printk("Read Sector 0 on disk %d\n", i);
		
		Status = DiskRead(Disk, 0, 0, 512, Buffer);
		DumpMem(Buffer, 512);

		memset(Buffer, 0xFF, 512);
		printk("Read Sector 10 on disk %d\n", i);
		Status = DiskRead(Disk, 10, 0, 512, Buffer);
		//printk("%016x\n", Status);
		DumpMem(Buffer, 512);
		/*
		printk("Write Sector 10 on disk %d:\n", i);
		memset(Buffer, 0x55, 512);
		Status = DiskWrite(Disk, 10, 512, Buffer);
		memset(Buffer, 0x66, 512);
		printk("Read Sector 10 on disk %d:\n", i);
		Status = DiskRead(Disk, 10, 512, Buffer);
		DumpMem(Buffer, 512);*/
	}

	int j;
	for(i=0; NetCard = OpenNetCard(i); i++)
	{
		printk("NetCard %d:\n", i);
		
		NetCard->Operations->ReadMacAddress(NetCard, Buffer);
		printk("MAC Address:");
		
		for(j=0; j<5; j++)
		{
			printk("%02x-", Buffer[j]);
		}
		printk("%02x\n", Buffer[5]);
		
	}
}

extern PHYSICAL_MEMORY_AREA PhysicalMemoryArea;
UINT64 ApStartSig = 0;

#include "Vfs.h"
#include "../Arch/X86_64/Cpu/Vmx.h"

void VmTest()
{
	RETURN_STATUS Status;

	Status = VmxSupport();

	UINT64 X64ReadCr(UINT64 Index);
	
	printk("VMX Support:%s,%s\n", 
		Status == RETURN_UNSUPPORTED ? "CPU Unsupported" : "CPU Supported", 
		Status == RETURN_ABORTED ? "BIOS Disabled":"BIOS Enabled"
	);

	Status = VmxStatus();
	printk("VMX Status:%s\n",
		Status == RETURN_NOT_READY ? "Not Ready":"Ready");

	VmxEnable();

	Status = VmxStatus();
	printk("VMX Status:%s\n",
		Status == RETURN_NOT_READY ? "Not Ready" : "Ready");
	
	VIRTUAL_MACHINE *Vm = VmxNew();
		
	Status = VmxInit(Vm);
	if (Status == RETURN_SUCCESS)
		printk("VMX:Enter VMX operation.\n");
	else
		printk("VMX:Failed to enter VMX operation.\n");

	VmxCopyHostState(Vm);
	VmxSetVmcs(&Vm->Vmcs);
	
	printk("Host RIP:%016x\n", VmRead(HOST_RIP));

	UINT8 Buffer[50];
	ArchReadCpuId(Buffer);
	printk("Host CPU String:%s\n", Buffer);
	strcpy(Buffer, "Intel(R) Core(TM) i7-5960X CPU @ 8.00GHz");

	printk("VM Launch...\n");
	VmLaunch(&Vm->HostRegs, &Vm->GuestRegs);
	UINT32 VmExitReason = VmRead(VM_EXIT_REASON);
	extern SPIN_LOCK ConsoleSpinLock;
	//ConsoleSpinLock.SpinLock = 0;
	//printk("VM Exit.Exit reason:%02d\n", VmExitReason);

	if (VmExitReason == 0xa)
	{
VM_RESUME:
	
		VmResume(&Vm->HostRegs, &Vm->GuestRegs);
		
		ConsoleSpinLock.SpinLock = 0;
		VmExitReason = VmRead(VM_EXIT_REASON);
		//printk("VM exit after resume.Exit reason:%02d\n", VmExitReason);

		if (Vm->GuestRegs.RAX == 0x80000002 && Vm->GuestRegs.RCX == 0)
		{
			Vm->GuestRegs.RAX = *(UINT32 *)&Buffer[0];
			Vm->GuestRegs.RBX = *(UINT32 *)&Buffer[4];
			Vm->GuestRegs.RCX = *(UINT32 *)&Buffer[8];
			Vm->GuestRegs.RDX = *(UINT32 *)&Buffer[12];
		}

		if (Vm->GuestRegs.RAX == 0x80000003 && Vm->GuestRegs.RCX == 0)
		{
			Vm->GuestRegs.RAX = *(UINT32 *)&Buffer[16];
			Vm->GuestRegs.RBX = *(UINT32 *)&Buffer[20];
			Vm->GuestRegs.RCX = *(UINT32 *)&Buffer[24];
			Vm->GuestRegs.RDX = *(UINT32 *)&Buffer[28];
		}

		if (Vm->GuestRegs.RAX == 0x80000004 && Vm->GuestRegs.RCX == 0)
		{
			Vm->GuestRegs.RAX = *(UINT32 *)&Buffer[32];
			Vm->GuestRegs.RBX = *(UINT32 *)&Buffer[36];
			Vm->GuestRegs.RCX = *(UINT32 *)&Buffer[40];
			Vm->GuestRegs.RDX = *(UINT32 *)&Buffer[44];
		}

		if (Vm->GuestRegs.RAX == 0x80000001 && Vm->GuestRegs.RCX == 0)
		{
			Vm->GuestRegs.RAX = 0;
			Vm->GuestRegs.RBX = 0;
			Vm->GuestRegs.RCX = 0;
			Vm->GuestRegs.RDX = 0;
		}
		
		UINT64 Rip = VmRead(GUEST_RIP);
		Rip += VmRead(VM_EXIT_INSTRUCTION_LEN);
		VmWrite(GUEST_RIP, Rip);
		if (VmExitReason == 0xa)
			goto VM_RESUME;
//VM_EXIT:
	}

	//VmExitReason = VmRead(VM_EXIT_REASON);
	printk("VM Exit.Exit reason:%x\n", VmExitReason);
	printk("Guest RIP:%016x\n", VmRead(GUEST_RIP));
	printk("VM Exit qualification:%x\n", VmRead(EXIT_QUALIFICATION));
	printk("VM Exit Info:%x\n", VmRead(VM_EXIT_INTR_ERROR_CODE));
}


void InitMain()
{
	UINT32 Counter = 0;
	DisableLocalIrq();
	printk("Kernel Build Time:%s %s\n", __DATE__, __TIME__);

	MEMORY_REGION * Ptr = PhysicalMemoryArea.MemoryRegion;
	printk("Availabel Memory:%dMB.\n", PhysicalMemoryArea.PhysTotalSize >> 20);

	printk("Initializing Virtual File System...\n");
	//VfsInit();
	/*
	while (Ptr)
	{
		printk("Memory Region:Base:%016x,Size:%dKB.\n", Ptr->Base, Ptr->Length / 1024);
		Ptr = Ptr->Next;
	}
	*/
	printk("Initializing Task Manager...\n");
	TaskManagerInit();

	ApStartSig = 1;

	//while(Counter++ < 20)
	//	printk("=================%d=================\n", Counter);
	
	printk("Initializing MainThread...\n");
	
	THREAD_CONTROL_BLOCK * InitThread;
	ThreadCreate(&InitThread, KernelInitThread, NULL, KERNEL_THREAD | CREATE_PROCESS, 10000);
	
	printk("Waking up MainThread...\n");
	
	ThreadWakeUp(InitThread);
	
	printk("Basic Initialization Done.\n");
	printk("Waiting for Clock Interrupt to start 1st Progress...\n");
	
	printk("Enabling Interrupts...\n");

	
	
	EnableIrqn(0x2);
	EnableLocalIrq();

	while (1)
	{
		//printk("Halting...\n");
		ArchHaltCpu();
	};
}
