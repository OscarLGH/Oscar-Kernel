#include "NetCard.h"

NETCARD_NODE * NetCardList = NULL;

void NetCardRegister(DEVICE_NODE * Dev)
{
	NETCARD_NODE * NetCard = NetCardList;
	NETCARD_NODE * NetCardNew = NULL;

	if(Dev->DeviceType == DEVICE_TYPE_NET)
	{
		NetCardNew = (NETCARD_NODE *)KMalloc(sizeof(NETCARD_NODE));
		NetCardNew->Device = Dev;
		NetCardNew->Operations = (NETCARD_DRIVER *)Dev->Driver;
		NetCardNew->Next = NULL;
		
		if(NetCard != NULL)
		{
			while(NetCard->Next)
			{
				NetCard = NetCard->Next;
			}
			NetCard->Next = NetCardNew;
		}
		else
			NetCardList = NetCardNew;

		//printk("NetCard->Device->Position:%x\n", NetCardList->Device->Position);
	}
}

NETCARD_NODE * OpenNetCard(UINT64 DevNum)
{
	NETCARD_NODE * NetCardNode = NetCardList;
	
	if(NetCardNode == NULL)
		return NULL;
	
	while(NetCardNode)
	{
		if(DevNum == 0)
		{
			if(NetCardNode->Device->Driver == NULL)
			{
				KDEBUG("Failed to open device %08x due to no driver binded.\n", NetCardNode->Device->DeviceID);
				return NULL;
			}
			NetCardNode->Operations = NetCardNode->Device->Driver;
			return NetCardNode;
		}
		NetCardNode = NetCardNode->Next;
		DevNum--;
	}
	return NULL;
}


