#include "Debug.h"

VOID X64SetupBranchRecord()
{
	/* Enable LBR Stack.*/
	UINT64 CtlReg = X64ReadMsr(MSR_IA32_DEBUGCTLMSR);
	CtlReg |= LBR;
	X64WriteMsr(MSR_IA32_DEBUGCTLMSR, CtlReg);

	/* Setup condition */
	UINT64 SelectReg = X64ReadMsr(MSR_LBR_SELECT);
	//SelectReg |= ((UINT8)~(NEAR_REL_CALL | NEAR_IND_CALL));
	X64WriteMsr(MSR_LBR_SELECT, SelectReg);
}

RETURN_STATUS X64ReadBranchRecord(UINT64 Index, CPU_BRANCH_DATA * Data)
{
	if (Index > 16)
		return RETURN_UNSUPPORTED;

	Data->FromIp = X64ReadMsr(MSR_LBR_NHM_FROM + Index);
	Data->ToIp = X64ReadMsr(MSR_LBR_NHM_TO + Index);

	return RETURN_SUCCESS;
}

RETURN_STATUS X64GetCpuDebugInfo(CPU_DEBUG_INFO * Info)
{
	Info->LbrTop = X64ReadMsr(MSR_LBR_TOS);
	RETURN_STATUS Status;

	UINT64 Index = 0;
	for (Index = 0; Index < BRANCH_REGS; Index++)
	{
		Status = X64ReadBranchRecord(Index, &Info->BranchData[Index]);
	}

	return Status;
}


SPIN_LOCK DumpSpinLock;
void RegisterDump(UINT64 * StackAddr, UINT64 ExceptionNum)
{
	int i;
	X86_64_STACK_FRAME * StackFrame = (X86_64_STACK_FRAME *)(StackAddr + 5);
	char * ExceptionName[] =
	{
		"#DE",
		"#DB",
		"NMI",
		"#BP",
		"#OF",
		"#BR",
		"#UD",
		"#NM",
		"#DF",
		"Not Used",
		"#TS",
		"#NP",
		"#SS",
		"#GP",
		"#PF",
		"Not Used",
		"#MF",
		"#AC",
		"#MC",
		"#XM"
	};
	//CONSOLEInit(&SysCon);
	//ClearScreen(&SysCon);
	SpinLockIrqSave(&DumpSpinLock);
	printk("\nAn Exception Has Occurd.\n%s (0x%x)\n", ExceptionName[ExceptionNum], ExceptionNum);
	printk("\nCPU APIC ID:%d\n", ArchGetCpuIdentifier());
	printk("\nRegisters:\n");
	printk("CS :%04x ", *(UINT64 *)((UINT64)StackFrame + 21 * 8));
	printk("DS :%04x ", StackFrame->DS);
	printk("ES :%04x ", StackFrame->ES);
	printk("FS :%04x ", StackFrame->FS);
	printk("GS :%04x ", StackFrame->GS);
	printk("SS :%04x\n", *(UINT64 *)((UINT64)StackFrame + 24 * 8));

	printk("CR0:%016x ", StackAddr[0]);
	printk("CR2:%016x ", StackAddr[1]);
	printk("CR3:%016x ", StackAddr[2]);
	printk("CR4:%016x ", StackAddr[3]);
	printk("CR8:%016x\n", StackAddr[4]);

	printk("RAX:%016x ", StackFrame->RAX);
	printk("RBX:%016x ", StackFrame->RBX);
	printk("RCX:%016x ", StackFrame->RCX);
	printk("RDX:%016x\n", StackFrame->RDX);
	printk("RSI:%016x ", StackFrame->RSI);
	printk("RDI:%016x ", StackFrame->RDI);
	printk("RBP:%016x ", StackFrame->RBP);
	printk("RSP:%016x\n", *(UINT64 *)((UINT64)StackFrame + 23 * 8));

	printk("R08:%016x ", StackFrame->R8);
	printk("R09:%016x ", StackFrame->R9);
	printk("R10:%016x ", StackFrame->R10);
	printk("R11:%016x\n", StackFrame->R11);
	printk("R12:%016x ", StackFrame->R12);
	printk("R13:%016x ", StackFrame->R13);
	printk("R14:%016x ", StackFrame->R14);
	printk("R15:%016x\n", StackFrame->R15);

	printk("RIP:%016x\n", *(UINT64 *)((UINT64)StackFrame + 20 * 8));

	printk("RFLAGS:%016x\n", *(UINT64 *)((UINT64)StackFrame + 22 * 8));

	printk("ERRON:%016x\n", *(UINT64 *)((UINT64)StackFrame + 19 * 8));

	CPU_DEBUG_INFO Info;
	X64GetCpuDebugInfo(&Info);

	printk("LastBranch Top:%d\n", Info.LbrTop);
	for (i = 0; i < 16; i++)
		printk("From:%016x to %016x\n", Info.BranchData[i].FromIp, Info.BranchData[i].ToIp);

	SpinUnlockIrqRestore(&DumpSpinLock);
	while (1);
	X64HaltCpu();
}

