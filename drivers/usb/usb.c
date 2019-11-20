#include "usb.h"
#include "init.h"

struct list_head usb_dev_list;

int usb_device_register(struct usb_device *dev)
{
	list_add(&dev->list, &usb_dev_list);
}

int usb_device_unregister(struct usb_device *dev)
{
	list_del(&dev->list);
}

int usb_core_init()
{
	INIT_LIST_HEAD(&usb_dev_list);
}

