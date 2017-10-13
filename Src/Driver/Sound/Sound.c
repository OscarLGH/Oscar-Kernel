#include "Sound.h"

SOUND_NODE * SoundCardList = NULL;

void SoundCardRegister(DEVICE_NODE * Dev)
{
	SOUND_NODE * SoundDev;
	
	if(Dev->DeviceType == DEVICE_TYPE_MULTIMEDIA)
	{
		SoundDev = (SOUND_NODE *)KMalloc(sizeof(SOUND_NODE));
		SoundDev->Device = Dev;
		SoundDev->Operations = (SOUND_DRIVER *)&Dev->Driver;
		SoundDev->Next = SoundCardList;
		SoundCardList = SoundDev;	
	}
	Dev = Dev->NextDevice;
}

SOUND_NODE * SoundlayOpen(UINT32 DevNum)
{
	SOUND_NODE * SoundDevNode = SoundCardList;
	
	if(SoundDevNode == NULL)
		return NULL;
	
	while(SoundDevNode)
	{
		if(DevNum == 0)
		{
			if(SoundDevNode->Device->Driver == NULL)
			{
				KDEBUG("Failed to open device %08x due to no driver binded.\n", SoundDevNode->Device->DeviceID);
				return NULL;
			}
			SoundDevNode->Operations = SoundDevNode->Device->Driver;
			return SoundDevNode;
		}
		
		SoundDevNode = SoundDevNode->Next;
		DevNum--;
	}
	return NULL;
}

