#ifndef _VM_H
#define _VM_H

#include "Type.h"
#include "BaseLib.h"


#define VM_STATUS_CLEAR 0
#define VM_STATUS_LAUNCHED 1
#define VM_STATUS_PAUSED 2
#define VM_STATUS_STOPPED 3


typedef struct _VCPU
{
	UINT64 VmStatus;
	UINT64 PhysCpuId;
	UINT64 Index;	//Necessary for addressing parent struct VM.
	VOID *ArchSpecificData;

	RETURN_STATUS (*VcpuInit)(struct _VCPU *Vcpu);
	RETURN_STATUS (*VcpuPinShadowPage)(struct _VCPU *Vcpu, UINT64 ShadowPagePhys);
	RETURN_STATUS (*VcpuPrepare)(struct _VCPU *Vcpu);
	RETURN_STATUS (*VcpuCheck)(struct _VCPU *Vcpu);
	RETURN_STATUS (*VcpuRun)(struct _VCPU *Vcpu);
	RETURN_STATUS (*VcpuPause)(struct _VCPU *Vcpu);
	RETURN_STATUS (*VcpuStop)(struct _VCPU *Vcpu);
	RETURN_STATUS (*ExitHandler)(struct _VCPU *Vcpu);
	
	RETURN_STATUS (*EventInject)(struct _VCPU *Vcpu, UINT64 Type, UINT64 Vector);
	RETURN_STATUS (*PostedInterruptVectorSet)(struct _VCPU *Vcpu, UINT64 Vector);
	RETURN_STATUS (*PostedInterruptSet)(struct _VCPU *Vcpu, UINT64 Vector);

	RETURN_STATUS (*InstructionEmulate)(struct _VCPU *Vcpu);
	
	SPIN_LOCK VcpuSpinLock;
	//LIST_HEAD VcpuHead;
}VCPU;

typedef struct _VM_MEMORY_MAP
{
	VOID *HostVirtual;
	UINT64 GuestPhysical;
	UINT64 Length;
	struct _VM_MEMORY_MAP *Next;
}VM_MEMORY_MAP;


typedef struct _VMEM
{
	UINT64 MemoryAmount;
	VOID *ShadowPageTable;
	VM_MEMORY_MAP *ReversedMap;
	SPIN_LOCK VmemSpinLock;

	RETURN_STATUS (*VmemInit)(struct _VMEM *Vmem);
	RETURN_STATUS (*ShadowPageMap)(struct _VMEM *Vmem, UINT64 Gpa, UINT64 Hpa, UINT64 Len);
	RETURN_STATUS (*ShadowPageUnmap)(struct _VMEM *Vmem, UINT64 Gpa, UINT64 Hpa, UINT64 Len);
	RETURN_STATUS (*AddReversedMap)(struct _VMEM *Vmem, UINT64 Gpa, UINT64 Hva, UINT64 Len);
	RETURN_STATUS (*RemoveMemMap)(struct _VMEM *Vmem, UINT64 Gpa, UINT64 Hva, UINT64 Len);
	RETURN_STATUS (*GuestMemoryRead)(struct _VMEM *Vmem, UINT64 Gpa, VOID *Buffer, UINT64 Len);
	RETURN_STATUS (*GuestMemoryWrite)(struct _VMEM *Vmem, UINT64 Gpa, VOID *Buffer, UINT64 Len);
}VMEM;

RETURN_STATUS VmemAddMemMap(VMEM *Vmem, UINT64 Gpa, UINT64 Hva, UINT64 Len);
RETURN_STATUS VmemRemoveMemMap(VMEM *Vmem, UINT64 Gpa, UINT64 Hva, UINT64 Len);
RETURN_STATUS VmemGuestMemoryRead(VMEM *Vmem, UINT64 Gpa, VOID *Buffer, UINT64 Len);
RETURN_STATUS VmemGuestMemoryWrite(VMEM *Vmem, UINT64 Gpa, VOID *Buffer, UINT64 Len);
RETURN_STATUS VmemAllocate(VMEM *Vmem, UINT64 Gpa, UINT64 Size);

typedef struct _VM_MODE
{
	VCPU *Vcpu;
	VMEM *Vmem;
}VM_NODE;
typedef struct _VM
{
	UINT64 VmNodeCount;
	VM_NODE **VmNodeArray;
}VIRTUAL_MACHINE;

typedef struct _VM_TEMPLATE
{
	VCPU *(*VcpuCreate)();
	VMEM *(*VmemCreate)();
}VM_TEMPLATE;


VIRTUAL_MACHINE *VmNew();
RETURN_STATUS VmNodeCreate(VIRTUAL_MACHINE *Vm);
RETURN_STATUS VmVcpuRun(VCPU *Vcpu);


extern VM_TEMPLATE VmTemplate;


#endif

