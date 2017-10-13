#ifndef PARTITION_H
#define PARTITION_H

#include "BaseLib.h"
#include "Disk.h"


typedef struct _PARTITION_MAP_ENRTY
{
	UINT64 StartLBA;
	UINT64 EndLBA;
	UINT64 Type;
	UINT64 Attribute;
	struct _PARTITION_MAP_ENRTY * Next;
}PARTITION_MAP_ENRTY;

typedef RETURN_STATUS(*PARTITION_PROBE_FUNC)(DISK_NODE * Disk);

typedef struct _PARTITION_PROBE
{
	PARTITION_PROBE_FUNC PartitionProbeFunc;
	struct _PARTITION_PROBE * Next;
}PARTITION_PROBE;

void RegisterPartitionProbe(PARTITION_PROBE_FUNC PartitionProbeFunc);
void UnregisterPartitionProbe(PARTITION_PROBE_FUNC PartitionProbeFunc);

RETURN_STATUS PartitionProbe(DISK_NODE * Disk);

void PartitionInit();


#endif