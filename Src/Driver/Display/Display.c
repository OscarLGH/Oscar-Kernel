#include "Display.h"

DISPLAY_NODE * DisplayAdapterList = NULL;

void DisplayAdapterRegister(FB_DEVICE * FbDev)
{
	DISPLAY_NODE * DispDev;
	
	DispDev = (DISPLAY_NODE *)KMalloc(sizeof(DISPLAY_NODE));
	DispDev->FbDevice = FbDev;
	DispDev->Next = DisplayAdapterList;
	DisplayAdapterList = DispDev;	
}

extern UINT64 VesaFbStart(DEVICE_NODE * Dev);

void DisplayEarlyInit()
{
	VesaFbStart(NULL);
}

DISPLAY_NODE * DisplayOpen(UINT32 DevNum)
{
	DISPLAY_NODE * DispDevNode = DisplayAdapterList;
	
	if(DispDevNode == NULL)
		return NULL;
	
	while(DispDevNode)
	{
		if(DevNum == 0)
		{
			return DispDevNode;
		}
		
		DispDevNode = DispDevNode->Next;
		DevNum--;
	}
	return NULL;
}

