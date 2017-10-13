#ifndef _SOUND_H
#define _SOUND_H


#include "Device.h"
#include "Lib/Memory.h"

typedef struct _SOUND_OPERATIONS
{
	void * abc;
}SOUND_DRIVER;

typedef struct _SOUND_NODE
{
	DEVICE_NODE * Device;
	SOUND_DRIVER * Operations;
	struct _SOUND_NODE * Next;
}SOUND_NODE;

extern DRIVER_NODE * DriverList[32];

void SoundCardRegister(DEVICE_NODE * Dev);
SOUND_NODE * SoundlayOpen(UINT32 DevNum);





#endif
