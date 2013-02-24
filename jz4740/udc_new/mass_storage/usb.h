#ifndef __USB_H
#define	__USB_H

#ifndef u8
#define u8	unsigned char
#endif

#ifndef u16
#define u16	unsigned short
#endif

#ifndef u32
#define u32	unsigned int
#endif

#ifndef s8
#define s8	char
#endif

#ifndef s16
#define s16	short
#endif

#ifndef s32
#define s32	int
#endif

extern int usbdebug;

typedef enum 
{
	ENDPOINT_TYPE_CONTROL,
	/* Typically used to configure a device when attached to the host.
	 * It may also be used for other device specific purposes, including
	 * control of other pipes on the device.
	 */
	ENDPOINT_TYPE_ISOCHRONOUS,
	/* Typically used for applications which need guaranteed speed.
	 * Isochronous transfer is fast but with possible data loss. A typical
	 * use is audio data which requires a constant data rate.
	 */
	ENDPOINT_TYPE_BULK,
	/* Typically used by devices that generate or consume data in relatively
	 * large and bursty quantities. Bulk transfer has wide dynamic latitude
	 * in transmission constraints. It can use all remaining available bandwidth,
	 * but with no guarantees on bandwidth or latency. Since the USB bus is
	 * normally not very busy, there is typically 90% or more of the bandwidth
	 * available for USB transfers.
	 */
	ENDPOINT_TYPE_INTERRUPT
	/* Typically used by devices that need guaranteed quick responses
	 * (bounded latency).
	 */
}USB_ENDPOINT_TYPE;


enum USB_STANDARD_REQUEST_CODE {
	GET_STATUS,
	CLEAR_FEATURE,
	SET_FEATURE = 3,
	SET_ADDRESS = 5,
	GET_DESCRIPTOR,
	SET_DESCRIPTOR,
	GET_CONFIGURATION,
	SET_CONFIGURATION,
	GET_INTERFACE,
	SET_INTERFACE,
	SYNCH_FRAME
};


enum USB_DESCRIPTOR_TYPE {
	DEVICE_DESCRIPTOR = 1,
	CONFIGURATION_DESCRIPTOR,
	STRING_DESCRIPTOR,
	INTERFACE_DESCRIPTOR,
	ENDPOINT_DESCRIPTOR,
	DEVICE_QUALIFIER_DESCRIPTOR,
	OTHER_SPEED_CONFIGURATION_DESCRIPTOR,
	INTERFACE_POWER1_DESCRIPTOR
};


enum USB_FEATURE_SELECTOR {
	ENDPOINT_HALT,
	DEVICE_REMOTE_WAKEUP,
	TEST_MODE
};


enum USB_CLASS_CODE {
	CLASS_DEVICE,
	CLASS_AUDIO,
	CLASS_COMM_AND_CDC_CONTROL,
	CLASS_HID,
	CLASS_PHYSICAL = 0x05,
	CLASS_STILL_IMAGING,
	CLASS_PRINTER,
	CLASS_MASS_STORAGE,
	CLASS_HUB,
	CLASS_CDC_DATA,
	CLASS_SMART_CARD,
	CLASS_CONTENT_SECURITY = 0x0d,
	CLASS_VIDEO,
	CLASS_DIAGNOSTIC_DEVICE = 0xdc,
	CLASS_WIRELESS_CONTROLLER = 0xe0,
	CLASS_MISCELLANEOUS = 0xef,
	CLASS_APP_SPECIFIC = 0xfe,
	CLASS_VENDOR_SPECIFIC = 0xff
};


typedef struct {
	u8 bmRequestType;
	u8 bRequest;
	u16 wValue;
	u16 wIndex;
	u16 wLength;
} __attribute__ ((packed)) USB_DeviceRequest;


typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u16 bcdUSB;
	u8 bDeviceClass;
	u8 bDeviceSubClass;
	u8 bDeviceProtocol;
	u8 bMaxPacketSize0;
	u16 idVendor;
	u16 idProduct;
	u16 bcdDevice;
	u8 iManufacturer;
	u8 iProduct;
	u8 iSerialNumber;
	u8 bNumConfigurations;
} __attribute__ ((packed)) USB_DeviceDescriptor;


typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u16 bcdUSB;
	u8 bDeviceClass;
	u8 bDeviceSubClass;
	u8 bDeviceProtocol;
	u8 bMaxPacketSize0;
	u8 bNumConfigurations;
	u8 bReserved;
} __attribute__ ((packed)) USB_DeviceQualifierDescriptor;


typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u16 wTotalLength;
	u8 bNumInterfaces;
	u8 bConfigurationValue;
	u8 iConfiguration;
	u8 bmAttributes;
	u8 MaxPower;
} __attribute__ ((packed)) USB_ConfigDescriptor;


typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u16 wTotalLength;
	u8 bNumInterfaces;
	u8 bConfigurationValue;
	u8 iConfiguration;
	u8 bmAttributes;
	u8 bMaxPower;
} __attribute__ ((packed)) USB_OtherSpeedConfigDescriptor;


typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u8 bInterfaceNumber;
	u8 bAlternateSetting;
	u8 bNumEndpoints;
	u8 bInterfaceClass;
	u8 bInterfaceSubClass;
	u8 bInterfaceProtocol;
	u8 iInterface;
} __attribute__ ((packed)) USB_InterfaceDescriptor;


typedef struct {
	u8 bLegth;
	u8 bDescriptorType;
	u8 bEndpointAddress;
	u8 bmAttributes;
	u16 wMaxPacketSize;
	u8 bInterval;
} __attribute__ ((packed)) USB_EndPointDescriptor;


typedef struct {
	u8 bLength;
	u8 bDescriptorType;
	u16 SomeDesriptor[1];
} __attribute__ ((packed)) USB_StringDescriptor;


#endif // !defined(__USB_H)

