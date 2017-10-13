#include "Partition.h"

#include "MBR.h"
#include "GPT.h"


PARTITION_PROBE *PartitionProbeFuncList = NULL;

void RegisterPartitionProbe(PARTITION_PROBE_FUNC PartitionProbeFunc)
{
	PARTITION_PROBE *TempEntry = PartitionProbeFuncList;
	PARTITION_PROBE *NewEntry = KMalloc(sizeof(PARTITION_PROBE));

	NewEntry->PartitionProbeFunc = PartitionProbeFunc;
	NewEntry->Next = NULL;

	if (TempEntry)
	{
		while (TempEntry->Next)
		{
			TempEntry = TempEntry->Next;
		}
		TempEntry->Next = NewEntry;
	}
	else
		PartitionProbeFuncList = NewEntry;
}


void UnregisterPartitionProbe(PARTITION_PROBE_FUNC PartitionProbeFunc)
{
	PARTITION_PROBE *TempEntry = PartitionProbeFuncList;

	if (TempEntry)
	{
		while (TempEntry->Next)
		{
			if (TempEntry->Next->PartitionProbeFunc == PartitionProbeFunc)
				break;

			TempEntry = TempEntry->Next;
		}
		TempEntry->Next = TempEntry->Next->Next;
		KFree(TempEntry->Next);
	}
	else
		printk("Fatal Error!Attempting to unregister item from empty list!\n");
}

RETURN_STATUS PartitionProbe(DISK_NODE * Disk)
{
	PARTITION_PROBE *TempEntry = PartitionProbeFuncList;
	RETURN_STATUS Status;
	
	while (TempEntry)
	{
		Status = TempEntry->PartitionProbeFunc(Disk);
		if (!Status)
			return RETURN_SUCCESS;

		TempEntry = TempEntry->Next;
	}
		
	return RETURN_NOT_FOUND;
}

void PartitionInit()
{
	RegisterPartitionProbe(GPTPartitionProbe);
	RegisterPartitionProbe(MBRPartitionProbe);
}
