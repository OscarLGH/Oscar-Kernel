/*	X86_64 architecture related implementation
*	Oscar
*	2017.4
*/

#include "Cpu.h"

VOID ArchReadCpuId(UINT8 * Buffer)
{
	X64_CPUID_BUFFER * CpuIdBuf = (X64_CPUID_BUFFER *)Buffer;
	X64GetCpuId(0x80000002, 0x00000000, CpuIdBuf);
	CpuIdBuf = (X64_CPUID_BUFFER *)(Buffer + 16);
	X64GetCpuId(0x80000003, 0x00000000, CpuIdBuf);
	CpuIdBuf = (X64_CPUID_BUFFER *)(Buffer + 32);
	X64GetCpuId(0x80000004, 0x00000000, CpuIdBuf);
	return;
}

UINT64 ArchGetCpuIdentifier(VOID)
{
	X64_CPUID_BUFFER CpuIdReg = { 0 };
	X64GetCpuId(0x00000001, 0x00000000, &CpuIdReg);

	return (CpuIdReg.EBX >> 24);
}

UINT64 ArchGetCpuType(VOID)
{
	UINT64 MsrIndex = MSR_IA32_APICBASE;
	UINT64 Value = 0;
	Value = X64ReadMsr(MsrIndex);
	return Value & BIT8;
}

UINT64 ArchGetIrqStatus(VOID)
{
	UINT64 Value = 0;
	Value = X64GetFlagsReg();
	return Value & BIT9;
}

VOID ArchGetSpinLock(UINT64 * SpinLock)
{
	X64GetSpinLock(SpinLock);
}

VOID ArchReleaseSpinLock(UINT64 * SpinLock)
{
	X64ReleaseSpinLock(SpinLock);
}
