#ifndef ARCH_H
#define ARCH_H

#include "Type.h"

VOID ArchReadCpuId(UINT8 * Buffer);
UINT64 ArchGetCpuIdentifier(VOID);
UINT64 ArchGetCpuType(VOID);
UINT64 ArchGetIrqStatus(VOID);
VOID ArchEnableCpuIrq(VOID);
VOID ArchDisableCpuIrq(VOID);
VOID ArchGetSpinLock(UINT64 * SpinLock);
VOID ArchReleaseSpinLock(UINT64 * SpinLock);
VOID ArchHaltCpu(VOID);

#endif