#include "Vm.h"

RETURN_STATUS VmemAddMemMap(VMEM *Vmem, UINT64 Gpa, UINT64 Hva, UINT64 Len)
{
	VM_MEMORY_MAP *MemMapPtr = Vmem->ReversedMap;
	VM_MEMORY_MAP *NewMapNode;
	VOID *HostVirtualAddress = NULL;

	while(MemMapPtr) {
		if (Gpa == MemMapPtr->GuestPhysical &&
			Hva == (UINT64)MemMapPtr->HostVirtual) {
			return RETURN_ACCESS_DENIED;
		}
		MemMapPtr = MemMapPtr->Next;
	}

	if (MemMapPtr == NULL) {
		NewMapNode = KMalloc(sizeof(*NewMapNode));
		NewMapNode->GuestPhysical = Gpa;
		NewMapNode->HostVirtual = (UINT64 *)Hva;
		NewMapNode->Length = Len;
		NewMapNode->Next = Vmem->ReversedMap;
		Vmem->ReversedMap = NewMapNode;
		return RETURN_SUCCESS;
	}
	
	return RETURN_SUCCESS;
}

RETURN_STATUS VmemRemoveMemMap(VMEM *Vmem, UINT64 Gpa, UINT64 Hva, UINT64 Len)
{
	VM_MEMORY_MAP *MemMapPtr = Vmem->ReversedMap;
	VM_MEMORY_MAP *NodeToRemove;
	VOID *HostVirtualAddress = NULL;

	while(MemMapPtr) {
		if (Gpa == MemMapPtr->Next->GuestPhysical &&
			Hva == (UINT64)MemMapPtr->Next->HostVirtual) {
			break;
		}
		MemMapPtr = MemMapPtr->Next;
	}

	if (MemMapPtr == NULL) {
		return RETURN_NOT_FOUND;
	}
	else {
		NodeToRemove = MemMapPtr->Next;
		MemMapPtr->Next = MemMapPtr->Next->Next;
		KFree(NodeToRemove);
	}

	return RETURN_SUCCESS;
}

RETURN_STATUS VmemAllocate(VMEM *Vmem, UINT64 Gpa, UINT64 Size)
{
	UINT8 *GuestMemory = KMalloc(Size);

	UINT64 Index;
	for (Index = 0; Index * 0x200000 < Size; Index++)
		Vmem->ShadowPageMap(Vmem, Gpa + Index * 0x200000, VIRT2PHYS(GuestMemory) + Index * 0x200000, 0x200000);
	
	VmemAddMemMap(Vmem, Gpa, (UINT64)GuestMemory, Size);
}

RETURN_STATUS VmemGuestMemoryRead(VMEM *Vmem, UINT64 Gpa, VOID *Buffer, UINT64 Len)
{
	VM_MEMORY_MAP *MemMapPtr = Vmem->ReversedMap;
	VOID *HostVirtualAddress = NULL;

	while(MemMapPtr) {
		if (Gpa >= MemMapPtr->GuestPhysical &&
			Gpa <= MemMapPtr->GuestPhysical + MemMapPtr->Length) {
			break;
		}
		MemMapPtr = MemMapPtr->Next;
	}

	if (MemMapPtr == NULL) {
		return RETURN_NOT_FOUND;
	}
	else {
		if (Gpa + Len > MemMapPtr->GuestPhysical + MemMapPtr->Length)
			return RETURN_OUT_OF_RESOURCES;
		
		HostVirtualAddress = (VOID *)(MemMapPtr->HostVirtual + (Gpa - MemMapPtr->GuestPhysical));
		memcpy(Buffer, HostVirtualAddress, Len);
	}

	return RETURN_SUCCESS;
}

RETURN_STATUS VmemGuestMemoryWrite(VMEM *Vmem, UINT64 Gpa, VOID *Buffer, UINT64 Len)
{
	VM_MEMORY_MAP *MemMapPtr = Vmem->ReversedMap;
	VOID *HostVirtualAddress = NULL;

	while(MemMapPtr) {
		if (Gpa >= MemMapPtr->GuestPhysical &&
			Gpa <= MemMapPtr->GuestPhysical + MemMapPtr->Length) {
			break;
		}
		MemMapPtr = MemMapPtr->Next;
	}

	if (MemMapPtr == NULL) {
		return RETURN_NOT_FOUND;
	}
	else {
		if (Gpa + Len > MemMapPtr->GuestPhysical + MemMapPtr->Length)
			return RETURN_OUT_OF_RESOURCES;
		
		HostVirtualAddress = (VOID *)(MemMapPtr->HostVirtual + (Gpa - MemMapPtr->GuestPhysical));
		memcpy(HostVirtualAddress, Buffer, Len);
	}

	return RETURN_SUCCESS;
	
}


VIRTUAL_MACHINE *VmNew()
{
	VIRTUAL_MACHINE *Vm = KMalloc(sizeof(*Vm));
	memset(Vm, 0, sizeof(*Vm));
	return Vm;
}

RETURN_STATUS VmNodeCreate(VIRTUAL_MACHINE *Vm)
{
	Vm->VmNodeArray[Vm->VmNodeCount] = KMalloc(sizeof(VM_NODE));
		
	Vm->VmNodeArray[Vm->VmNodeCount]->Vcpu = VmTemplate.VcpuCreate();
	Vm->VmNodeArray[Vm->VmNodeCount]->Vcpu->VcpuInit(Vm->VmNodeArray[Vm->VmNodeCount]->Vcpu);

	Vm->VmNodeArray[Vm->VmNodeCount]->Vmem = VmTemplate.VmemCreate();
	Vm->VmNodeArray[Vm->VmNodeCount]->Vmem->VmemInit(Vm->VmNodeArray[Vm->VmNodeCount]->Vmem);

	Vm->VmNodeCount++;
	return RETURN_SUCCESS;
}


RETURN_STATUS VmVcpuRun(VCPU *Vcpu)
{
	RETURN_STATUS Status;
	while (1) {
		Status = Vcpu->VcpuRun(Vcpu);
		if (Status)
			return Status;

		Status = Vcpu->ExitHandler(Vcpu);
		if (Status)
			return Status;
	}

	return RETURN_ABORTED;
}



