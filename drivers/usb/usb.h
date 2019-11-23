#ifndef _USB_H
#define _USB_h

#include <types.h>
#include "list.h"

#pragma pack(1)
//USB Descriptors
struct usb_device_descriptor {
	u8 b_length;
	u8 b_descriptor_type;
	u16 bcd_usb;
	u8 class;
	u8 subclass;
	u8 b_device_protocol;
	u8 b_max_packet_size0;
	u16 id_vender;
	u16 id_product;
	u16 bcd_device;
	u8 i_manufacturer;
	u8 i_product;
	u8 i_serial_number;
	u8 b_num_configurations;
};

struct usb_device_qualifier_descriptor {
	u8 b_length;
	u8 b_descriptor_type;
	u16 bcd_usb;
	u8 b_device_class;
	u8 b_device_subclass;
	u8 b_device_protocol;
	u8 b_max_packet_size0;
	u8 b_num_configurations;
	u8 b_reserved;
};

struct usb_configuration_descriptor {
	u8 b_length;
	u8 b_descriptor_type;
	u16 w_total_lenth;
	u8 b_num_interfaces;
	u8 b_configuration_value;
	u8 i_configuration;
	u8 bm_attributes;
	u8 b_max_power;
};

struct usb_other_speed_conf_descriptor {
	u8 b_length;
	u8 b_descriptor_type;
	u16 w_total_lenth;
	u8 b_num_interfaces;
	u8 b_configuration_value;
	u8 i_configuration;
	u8 bm_attributes;
	u8 b_max_power;
};

struct usb_interface_descriptor {
	u8 b_length;
	u8 b_descriptor_type;
	u8 b_interface_number;
	u8 b_alternate_setting;
	u8 b_num_endpoints;
	u8 b_interface_class;
	u8 b_interface_subclass;
	u8 b_interface_protocol;
	u8 i_interface;
};

struct usb_endpoint_descriptor {
	u8 b_length;
	u8 b_descriptor_type;
	u8 b_endpoint_addr;
	u8 bm_attributes;
	u16 w_max_packet_size;
	u8 b_interval;
};

struct usb_string_zero_descriptor {
	u8 b_length;
	u8 b_descriptor_type;
	u16 w_lang_id[256];
};

struct usb_string_descriptor {
	u8 b_length;
	u8 b_descriptor_type;
	u8 b_string[256];
};
#pragma pack(0)

struct urb;
typedef void (*usb_complete_t)(struct urb *);

struct usb_interface;
struct usb_device;

struct usb_endpoint {
	struct usb_endpoint_descriptor desc;
	struct list_head list;
	struct list_head urb_list;
	struct usb_interface *intf;
};

struct usb_interface {
	struct usb_interface_descriptor desc;
	struct list_head list;
	struct list_head endpoint_list;
	struct usb_device *udev;
};
struct usb_device {
	int port;
	int slot;
	int endpoints;
	int speed;
	struct usb_device_descriptor desc;
	struct list_head interface_head;
	struct list_head list;
	void *host_controller_context;
};

enum usb_device_speed {
	USB_SPEED_UNKNOWN = 0,			/* enumerating */
	USB_SPEED_LOW, USB_SPEED_FULL,		/* usb 1.1 */
	USB_SPEED_HIGH,				/* usb 2.0 */
	USB_SPEED_WIRELESS,			/* wireless (usb 2.5) */
	USB_SPEED_SUPER,			/* usb 3.0 */
	USB_SPEED_SUPER_PLUS,			/* usb 3.1 */
};

struct usb_ctrlrequest {
	u8 bRequestType;
	u8 bRequest;
	u16 wValue;
	u16 wIndex;
	u16 wLength;
} __attribute__ ((packed));


struct urb {
	int pipe;
	struct usb_endpoint *ep;
	char *setup_packet;	/* (in) setup packet (control only) */
	void *transfer_buffer;		/* (in) associated data buffer */
	u32 transfer_buffer_length;	/* (in) data buffer length */
	struct usb_device *dev;
	void *context;			/* (in) context for completion */
	int interval;
	int start_frame;
	bool polling_wait;
	usb_complete_t complete;
};

/**
 * usb_fill_control_urb - initializes a control urb
 * @urb: pointer to the urb to initialize.
 * @dev: pointer to the struct usb_device for this urb.
 * @pipe: the endpoint pipe
 * @setup_packet: pointer to the setup_packet buffer
 * @transfer_buffer: pointer to the transfer buffer
 * @buffer_length: length of the transfer buffer
 * @complete_fn: pointer to the usb_complete_t function
 * @context: what to set the urb context to.
 *
 * Initializes a control urb with the proper information needed to submit
 * it to a device.
 */
static inline void usb_fill_control_urb(struct urb *urb,
					struct usb_device *dev,
					unsigned int pipe,
					unsigned char *setup_packet,
					void *transfer_buffer,
					int buffer_length,
					usb_complete_t complete_fn,
					void *context)
{
	urb->dev = dev;
	urb->pipe = pipe;
	urb->setup_packet = setup_packet;
	urb->transfer_buffer = transfer_buffer;
	urb->transfer_buffer_length = buffer_length;
	urb->complete = complete_fn;
	urb->context = context;
}

/**
 * usb_fill_bulk_urb - macro to help initialize a bulk urb
 * @urb: pointer to the urb to initialize.
 * @dev: pointer to the struct usb_device for this urb.
 * @pipe: the endpoint pipe
 * @transfer_buffer: pointer to the transfer buffer
 * @buffer_length: length of the transfer buffer
 * @complete_fn: pointer to the usb_complete_t function
 * @context: what to set the urb context to.
 *
 * Initializes a bulk urb with the proper information needed to submit it
 * to a device.
 */
static inline void usb_fill_bulk_urb(struct urb *urb,
				     struct usb_device *dev,
				     unsigned int pipe,
				     void *transfer_buffer,
				     int buffer_length,
				     usb_complete_t complete_fn,
				     void *context)
{
	urb->dev = dev;
	urb->pipe = pipe;
	urb->transfer_buffer = transfer_buffer;
	urb->transfer_buffer_length = buffer_length;
	urb->complete = complete_fn;
	urb->context = context;
}

/**
 * usb_fill_int_urb - macro to help initialize a interrupt urb
 * @urb: pointer to the urb to initialize.
 * @dev: pointer to the struct usb_device for this urb.
 * @pipe: the endpoint pipe
 * @transfer_buffer: pointer to the transfer buffer
 * @buffer_length: length of the transfer buffer
 * @complete_fn: pointer to the usb_complete_t function
 * @context: what to set the urb context to.
 * @interval: what to set the urb interval to, encoded like
 *	the endpoint descriptor's bInterval value.
 *
 * Initializes a interrupt urb with the proper information needed to submit
 * it to a device.
 *
 * Note that High Speed and SuperSpeed(+) interrupt endpoints use a logarithmic
 * encoding of the endpoint interval, and express polling intervals in
 * microframes (eight per millisecond) rather than in frames (one per
 * millisecond).
 *
 * Wireless USB also uses the logarithmic encoding, but specifies it in units of
 * 128us instead of 125us.  For Wireless USB devices, the interval is passed
 * through to the host controller, rather than being translated into microframe
 * units.
 */
static inline void usb_fill_int_urb(struct urb *urb,
				    struct usb_device *dev,
				    unsigned int pipe,
				    void *transfer_buffer,
				    int buffer_length,
				    usb_complete_t complete_fn,
				    void *context,
				    int interval)
{
	urb->dev = dev;
	urb->pipe = pipe;
	urb->transfer_buffer = transfer_buffer;
	urb->transfer_buffer_length = buffer_length;
	urb->complete = complete_fn;
	urb->context = context;

	if (dev->speed == USB_SPEED_HIGH || dev->speed >= USB_SPEED_SUPER) {
		/* make sure interval is within allowed range */
		//interval = clamp(interval, 1, 16);

		urb->interval = 1 << (interval - 1);
	} else {
		urb->interval = interval;
	}

	urb->start_frame = -1;
}

//USB Standard Request Codes
#define USB_REQUEST_GET_STATUS 0
#define USB_REQUEST_CLEAR_FEATURE 1
#define USB_REQUEST_SET_FEATURE 3
#define USB_REQUEST_SET_ADDRESS 5
#define USB_REQUEST_GET_DESCRIPTOR 6
#define USB_REQUEST_SET_DESCRIPTOR 7
#define USB_REQUEST_GET_CONFIGURATION 8
#define USB_REQUEST_SET_CONFIGURATION 9
#define USB_REQUEST_GET_INTERFACE 10
#define USB_REQUEST_SET_INTERFACE 11
#define USB_REQUEST_SYNCH_FRAME 12

//USB Descriptor types
#define USB_DESCRIPTOR_TYPE_DEVICE 1
#define USB_DESCRIPTOR_TYPE_CONFIGURATION 2
#define USB_DESCRIPTOR_TYPE_STRING 3
#define USB_DESCRIPTOR_TYPE_INTERFACE 4
#define USB_DESCRIPTOR_TYPE_ENDPOINT 5
#define USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER 6
#define USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONF 7
#define USB_DESCRIPTOR_TYPE_INTERFACE_POWER 8

int usb_device_register(struct usb_device *dev);
int usb_device_unregister(struct usb_device *dev);
int usb_core_init() ;

extern struct list_head usb_dev_list;

#pragma pack(1)
struct usbmassbulk_cbw {
	u32 sig;
	u32 tag;
	u32 data_transfer_len;
	u8 flags;
	u8 lun;
	u8 length;
	u8 cb[16];
};

struct usbmassbulk_csw {
	u32 sig;
	u32 tag;
	u32 data_residue;
	u8 status;
};

struct ufi_cmd {
	u8 op_code;
	u8 rsvd0:5;
	u8 logical_unit_number:3;
	u32 logical_block_addr; //big endian
	u32 parameter; //big endian
	u8 rsvd2;
	u8 rsvd3;
};

#define SCSI_READ 0xa8
#define SCSI_READ_CAPACITY 0x25
#define SCSI_INNQUIRY 0x12

#pragma pack(0)
#endif
