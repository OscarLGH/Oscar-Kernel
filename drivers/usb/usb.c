#include "usb.h"

struct list_head usb_dev_list;

int usb_device_register(struct usb_device *dev)
{
	list_add(&dev->list, &usb_dev_list);
}

int usb_device_unregister(struct usb_device *dev)
{
	list_del(&dev->list);
}
