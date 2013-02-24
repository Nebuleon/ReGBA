#include <ucos_ii.h>
#include "usb.h"
#include "udcbus.h"
#include "udc.h"
#include "mass_storage.h"
#if 0
#define dprintf printf
#else
#define dprintf(x...)
#endif
#ifndef NULL
#define NULL
#endif
//#define UDC_FRAME_SIZE 4096
#define UDC_FRAME_SIZE (4 * 1024)

#define RECEIVE_CBW         0
#define RECEIVE_DATA        1
#define SENDING_CSW         2
#define SEND_DATA           3
#define RECEIVE_SERIES_DATA 4
#define SEND_SERIES_DATA    5
#define SENDED_CSW          6

#define MUTI_THREAD


#ifdef MUTI_THREAD
volatile static u32 ssector;
volatile static u32 nsector;

#define MASS_TASK_PRIO	        12
#define MASS_TASK_STK_SIZE	1024
OS_STK MassTaskStack[MASS_TASK_STK_SIZE];
u8 err;
#endif

unsigned char mass_state = RECEIVE_CBW;
//unsigned char send_state = RECEIVE_CBW;
#define MAX_LUN_COUNT 4
PUDC_LUN pDev[MAX_LUN_COUNT] = { 0, 0, 0, 0};
unsigned char curLunCount = 0;

PIPE pipe[3] = {
	              {0,ENDPOINT_TYPE_CONTROL,8},
				  {0x81,ENDPOINT_TYPE_BULK,64},
				  {0x2,ENDPOINT_TYPE_BULK,64}
               };

unsigned int RegisterDev(PUDC_LUN pdev)
{
	if(pdev)
	{
		if(curLunCount > MAX_LUN_COUNT)
		{
			printf("warning:Max Logical Unit number is %d\r\n",MAX_LUN_COUNT);
			return -1;
		}
		pDev[curLunCount++] = pdev;
	}
	return 0;
} 
/*
 * USB mass storage class subclasses
 */
enum UMASS_SUBCLASS
{
	MASS_SUBCLASS_RBC = 1,	// Flash
	MASS_SUBCLASS_8020,	// CD-ROM
	MASS_SUBCLASS_QIC,	// Tape
	MASS_SUBCLASS_UFI,	// Floppy disk device
	MASS_SUBCLASS_8070,	// Floppy disk device
	MASS_SUBCLASS_SCSI	// Any device with a SCSI-defined command set
};


/*
 * USB mass storage class protocols
 */
enum UMASS_PROTOCOL
{
	MASS_PROTOCOL_CBI,	// Command/Bulk/Interrupt
	MASS_PROTOCOL_CBI_CCI,	// Command/Bulk/Interrupt
	MASS_PROTOCOL_BULK = 0x50	// Bulk-Only transport
};

static u32 epout, epin;

static USB_DeviceDescriptor devDesc = 
{
	sizeof(USB_DeviceDescriptor),
	DEVICE_DESCRIPTOR,	//1
	0x0200,     //Version 2.0
//	0x08,
//	0x01,
//	0x50,
	0x00,
	0x00,
	0x00,
	64,	/* Ep0 FIFO size */
	0x0525,
	0xa4a5,
	0x0320,
	0x01, 	//iManufacturer;
	0x02,   //iProduct;
	0x03,
	0x01
};

#define	CONFIG_DESCRIPTOR_LEN	(sizeof(USB_ConfigDescriptor) + \
				 sizeof(USB_InterfaceDescriptor) + \
				 sizeof(USB_EndPointDescriptor) * 2)

static struct {
	USB_ConfigDescriptor    configuration_descriptor;
	USB_InterfaceDescriptor interface_descritor;
	USB_EndPointDescriptor  endpoint_descriptor[2];
} __attribute__ ((packed)) confDesc = {
	{
		sizeof(USB_ConfigDescriptor),
		CONFIGURATION_DESCRIPTOR,
		CONFIG_DESCRIPTOR_LEN,
		0x01,
		0x01,
		0x04,
		0x80,	// Self Powered, no remote wakeup
		0xfa	// Maximum power consumption 200 mA
		//0x00
	},
	{
		sizeof(USB_InterfaceDescriptor),
		INTERFACE_DESCRIPTOR,
		0x00,
		0x00,
		0x02,	/* ep number */
		CLASS_MASS_STORAGE,
		MASS_SUBCLASS_SCSI,
		MASS_PROTOCOL_BULK,
		0x05
	},
	{
		{
			sizeof(USB_EndPointDescriptor),
			ENDPOINT_DESCRIPTOR,
			(1 << 7) | 1,// endpoint 2 is IN endpoint
			2, /* bulk */
			/* Transfer Type: Bulk;
			 * Synchronization Type: No Synchronization;
			 * Usage Type: Data endpoint
			 */
			//512, /* IN EP FIFO size */
			512,
			0
		},
		{
			sizeof(USB_EndPointDescriptor),
			ENDPOINT_DESCRIPTOR,
			(0 << 7) | 1,// endpoint 5 is OUT endpoint
			2, /* bulk */
			/* Transfer Type: Bulk;
			 * Synchronization Type: No Synchronization;
			 * Usage Type: Data endpoint
			 */
			512, /* OUT EP FIFO size */
			0
		}
	}
};
static USB_DeviceQualifierDescriptor devQualifyDesc =
{
	sizeof(USB_DeviceQualifierDescriptor),
	0x06,
	0x0200,
	0x00,
	0x00,
	0x00,
	0x40,
	0x01,
	0x00
};

/*
void sendDevDescString(int size)
{
	u16 str_ret[13] = {
		   0x031a,//0x1a=26 byte
		   0x0041,
		   0x0030,
		   0x0030,
		   0x0041,
		   0x0030,
		   0x0030,
		   0x0041,
		   0x0030,
		   0x0030,
		   0x0041,
		   0x0030,
		   0x0030
		  };
	dprintf("sendDevDescString size = %d\r\n",size);
	if(size >= 26)
		size = 26;
	str_ret[0] = (0x0300 | size);
	HW_SendPKT(0, str_ret,size);
	
}*/



/*
 * Command Block Wrapper (CBW)
 */
#define	CBWSIGNATURE	0x43425355		// "CBSU"
#define	CBWFLAGS_OUT	0x00			// HOST-to-DEVICE
#define	CBWFLAGS_IN	0x80			// DEVICE-to-HOST
#define	CBWCDBLENGTH	16

typedef struct
{
	u32 dCBWSignature;
	u32 dCBWTag;
	s32 dCBWDataXferLength;
	u8 bmCBWFlags;
	/* The bits of this field are defined as follows:
	 *     Bit 7 Direction - the device shall ignore this bit if the
	 *                       dCBWDataTransferLength zero, otherwise:
	 *         0 = Data-Out from host to the device,
	 *         1 = Data-In from the device to the host.
	 *     Bit 6 Obsolete. The host shall set this bit to zero.
	 *     Bits 5..0 Reserved - the host shall set these bits to zero.
	 */
	u8 bCBWLUN : 4,
	   reserved0 : 4;

	u8 bCBWCBLength : 5,
	   reserved1    : 3;
	u8 CBWCB[CBWCDBLENGTH];
} __attribute__ ((packed)) CBW;


/*
 * Command Status Wrapper (CSW)
 */
#define	CSWSIGNATURE			0x53425355	// "SBSU"
#define	CSWSIGNATURE_IMAGINATION_DBX1	0x43425355	// "CBSU"
#define	CSWSIGNATURE_OLYMPUS_C1		0x55425355	// "UBSU"

#define	CSWSTATUS_GOOD			0x0
#define	CSWSTATUS_FAILED		0x1
#define	CSWSTATUS_PHASE_ERR		0x2

typedef struct
{
	u32 dCSWSignature;
	u32 dCSWTag;
	u32 dCSWDataResidue;
	u8 bCSWStatus;
	/* 00h Command Passed ("good status")
	 * 01h Command Failed
	 * 02h Phase Error
	 * 03h and 04h Reserved (Obsolete)
	 * 05h to FFh Reserved
	 */
} __attribute__ ((packed)) CSW;


/*
 * Required UFI Commands
 */
#define	UFI_FORMAT_UNIT			0x04	// output
#define	UFI_INQUIRY			0x12	// input
#define	UFI_MODE_SELECT			0x55	// output
#define	UFI_MODE_SENSE_6		0x1A	// input
#define	UFI_MODE_SENSE_10		0x5A	// input
#define	UFI_PREVENT_MEDIUM_REMOVAL	0x1E
#define	UFI_READ_10			0x28	// input
#define	UFI_READ_12			0xA8	// input
#define	UFI_READ_CAPACITY		0x25	// input
#define	UFI_READ_FORMAT_CAPACITY	0x23	// input
#define	UFI_REQUEST_SENSE		0x03	// input
#define	UFI_REZERO_UNIT			0x01
#define	UFI_SEEK_10			0x2B
#define	UFI_SEND_DIAGNOSTIC		0x1D
#define	UFI_START_UNIT			0x1B
#define	UFI_TEST_UNIT_READY		0x00
#define	UFI_VERIFY			0x2F
#define	UFI_WRITE_10			0x2A	// output
#define	UFI_WRITE_12			0xAA	// output
#define	UFI_WRITE_AND_VERIFY		0x2E	// output
#define	UFI_ALLOW_MEDIUM_REMOVAL	UFI_PREVENT_MEDIUM_REMOVAL
#define	UFI_STOP_UNIT			UFI_START_UNIT

#ifdef MUTI_THREAD
#define UDC_FRAME_SIZE ( 512 *0x80 )

static u32 massBuf0[UDC_FRAME_SIZE / 4] __attribute__ ((aligned (1024)));
static u32 massBuf1[UDC_FRAME_SIZE / 4] __attribute__ ((aligned (1024)));
//static u32 massBuf2[UDC_FRAME_SIZE / 4] __attribute__ ((aligned (1024)));
unsigned char *massbuf = 0;
volatile u8 udc_trigger = 1 , mass_trigger = 1;      //first buffer 0 !
OS_EVENT *sem_buf0, *sem_buf1, *sem_mass, *sem_device ;

#else

#define UDC_FRAME_SIZE (4 * 1024)
static u32 massBuf[UDC_FRAME_SIZE / 4] __attribute__ ((aligned (1024)));
unsigned char *massbuf = 0;

#endif

static CSW csw = {0};
static CBW cbw = {0};
unsigned int g_start_sector = 0;
unsigned short g_nr_sectors = 0;
unsigned int g_sector_size = 0;
static u8 iscardin;

static u32 swap32(u32 n)
{
	return (((n & 0x000000ff) >> 0) << 24) |
	       (((n & 0x0000ff00) >> 8) << 16) |
	       (((n & 0x00ff0000) >> 16) << 8) |
	       (((n & 0xff000000) >> 24) << 0);
}

typedef struct _CAPACITY_DATA {
	u32 Blocks;
	u32 BlockLen;   
	
}CAPACITY_DATA;
typedef struct _READ_FORMAT_CAPACITY_DATA {
	u8 Reserve1[3];
	u8 CapacityListLen;
	CAPACITY_DATA CurMaxCapacity;
	CAPACITY_DATA CapacityData[30];
	
}READ_FORMAT_CAPACITY_DATA;

/*Interface */
unsigned short udc_device_state = 0;
#define UDC_HW_CONNECT    1
#define UDC_SW_CONNECT     0x10


#define WAIT_RECEIVECBW_FINISH    0
#define WAIT_SENDCSW_FINISH       1
#define WAIT_RECEIVE_FINISH       2
#define WAIT_SEND_FINISH          3


static inline unsigned int Handle_UFI_INQUIRY(unsigned handle)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;
	
	static const unsigned char inquiry[] = {
		0x00,	// Direct-access device (floppy)
		0x80,	// 0x80	// Removable Media
		0x00,
		0x00,	// UFI device
		0x29,
		0x00, 0x00, 0x00,
		'I', 'n', 'g', 'e', 'n', 'i', 'c', ' ',
		'J', 'z', 'S', 'O', 'C', ' ', 'U', 'S',
		'B', '-', 'D', 'I', 'S', 'K', ' ', ' ',
		'0', '1', '0', '0'
	};
	u32 size = sizeof(inquiry);
	if(cbw.dCBWDataXferLength < sizeof(inquiry))
		size = cbw.dCBWDataXferLength;
	pBus->StartTransfer(handle,pipe[1].ep,(unsigned char *)inquiry, size);
	return 1;
	
}
static inline unsigned int Handle_UFI_REQUEST_SENSE(unsigned handle)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;
	static unsigned char sense[] = {
		0x70,
		0x00,
		0x06,
		0x00, 0x00, 0x00, 0x00,
		0x0a,
		0x00, 0x00, 0x00, 0x00,
		0x28,
		0x00,
		0x00,
		0x00, 0x00, 0x00
	};
	PUDC_LUN pdev = pDev[cbw.bCBWLUN];

	if ( !(pdev->CheckDevState(handle)) ) {
		sense[2] = 0x02;
		sense[12] = 0x3a;
		iscardin = 1;
	}
	else
	{
		sense[2] = 0x06;
		sense[12] = 0x28;
		iscardin = 0;
	}

	pBus->StartTransfer(handle,pipe[1].ep,(unsigned char *)sense,sizeof(sense));	
	return 1;
	
}

static inline unsigned int Handle_UFI_READ_CAPACITY(unsigned handle)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;
	DEVINFO devinfo;
	PUDC_LUN pdev = pDev[cbw.bCBWLUN]; 
	static u32 resp[2];
	pdev->GetDevInfo((unsigned int)pdev,&devinfo);
	resp[0] = swap32(devinfo.partsize-1); /* last sector */
	resp[1] = swap32(512);		/* sector size */
	if ( !(pdev->CheckDevState(handle)) ) {
  		csw.dCSWDataResidue = cbw.dCBWDataXferLength;
		csw.bCSWStatus = CSWSTATUS_FAILED;
		//if sd out,shall send stall!!
		pBus->StartTransfer(handle,0xff,(unsigned char *)resp, sizeof(resp));
		return 0;
	}
	else
	{
		pBus->StartTransfer(handle,pipe[1].ep,(unsigned char *)resp, sizeof(resp));
	}
	return 1;
	
}

static inline unsigned int Handle_UFI_TEST_UNIT_READY(unsigned handle)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;
	PUDC_LUN pdev = pDev[cbw.bCBWLUN]; 

	if ( iscardin == 0 ) {
		if ( !(pdev->CheckDevState(handle)) ) {
			csw.bCSWStatus = CSWSTATUS_FAILED;
		}
	}
	else
		csw.bCSWStatus = iscardin;
        Handle_CSW(handle);
	mass_state = SENDED_CSW;

	return 1;
}

static inline unsigned int Handle_CSW(unsigned int handle)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;
	PUDC_LUN pdev = pDev[cbw.bCBWLUN];

	pBus->StartTransfer(handle,pipe[1].ep,(unsigned char *)&csw, sizeof(CSW));
	return 1;
	
}

static inline unsigned int Handle_UFI_READ_10(unsigned handle)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;
	PUDC_LUN pdev = pDev[cbw.bCBWLUN];
	unsigned int sectors;
	g_start_sector =
		((unsigned int)cbw.CBWCB[2] << 24) |
		((unsigned int)cbw.CBWCB[3] << 16) |
		((unsigned int)cbw.CBWCB[4] << 8) |
		(unsigned int) cbw.CBWCB[5];
	
	g_nr_sectors =
		((unsigned short)cbw.CBWCB[7] << 8) | (unsigned short)cbw.CBWCB[8];
	sectors = UDC_FRAME_SIZE / g_sector_size;
	if(sectors > g_nr_sectors)
		sectors = g_nr_sectors;
	
#ifdef MUTI_THREAD
	massbuf = (unsigned char *)((unsigned int)massBuf0 | 0xa0000000);
	dprintf("read g_start_sector = %x\n g_nr_sectors = %x sectors = %x\n",g_start_sector,g_nr_sectors,sectors);
	OSSemPend(sem_device, 0, &err);
#endif
	pdev->ReadDevSector((unsigned int)pdev,massbuf,g_start_sector,sectors);

#ifdef MUTI_THREAD
	OSSemPost(sem_device);
#endif
	pBus->StartTransfer(handle,pipe[1].ep, (unsigned char *)massbuf,
						sectors * g_sector_size);
	g_start_sector += sectors;
	g_nr_sectors -= sectors;
	mass_state = SEND_DATA;
	return 1;
	
}

static inline unsigned int Handle_UFI_WRITE_10(unsigned handle)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;
	PUDC_LUN pdev = pDev[cbw.bCBWLUN]; 
	unsigned int sendcount;

	unsigned int sectors;
	
	g_start_sector =
		((u32)cbw.CBWCB[2] << 24) |
		((u32)cbw.CBWCB[3] << 16) |
		((u32)cbw.CBWCB[4] << 8) |
		(u32)cbw.CBWCB[5];
	g_nr_sectors   =
		((u16)cbw.CBWCB[7] << 8) | (u16)cbw.CBWCB[8];
	sendcount = g_nr_sectors * g_sector_size;
	sectors = g_nr_sectors;
	
	if(sendcount > UDC_FRAME_SIZE)
	{
		sendcount = UDC_FRAME_SIZE;
		sectors = sendcount / g_sector_size;
		
	}
	
	dprintf("write s:%x n:%x c:%x\n", g_start_sector, g_nr_sectors,sectors);

#ifdef MUTI_THREAD
	if ( udc_trigger == 1 )
	{
		massbuf = (unsigned char *)((unsigned int)massBuf1 | 0xa0000000);
		OSSemPend(sem_buf1, 0, &err);
	}
	else 
	{
		massbuf = (unsigned char *)((unsigned int)massBuf0 | 0xa0000000);
		OSSemPend(sem_buf0, 0, &err);
	}
//	printf("get here? \n");
#endif
	
	pBus->StartTransfer(handle,pipe[2].ep, massbuf,sendcount);
	if(g_nr_sectors)
		mass_state = RECEIVE_DATA;
	else
	{
		mass_state = SENDED_CSW;
		Handle_CSW(handle);
	}
	
	return 1;
	
}


static inline unsigned int Handle_UFI_READ_FORMAT_CAPACITY(unsigned handle)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;
	DEVINFO devinfo;
	PUDC_LUN pdev = pDev[cbw.bCBWLUN]; 
	READ_FORMAT_CAPACITY_DATA readfcd;
	memset(&readfcd,0,sizeof(readfcd));
	readfcd.CapacityListLen = 0x16;                 //M by yliu
	pdev->GetDevInfo((unsigned int)pdev,&devinfo);
	
	readfcd.CapacityData[0].Blocks = devinfo.partsize-1;
	readfcd.CapacityData[0].BlockLen = devinfo.sectorsize;
	readfcd.CurMaxCapacity.Blocks = devinfo.partsize-1;
	g_sector_size = devinfo.sectorsize;
	readfcd.CurMaxCapacity.BlockLen = g_sector_size;
	readfcd.CurMaxCapacity.BlockLen = (readfcd.CurMaxCapacity.BlockLen << 8) | 0x2;
    
	int size = cbw.bCBWCBLength;
	if(size > sizeof(READ_FORMAT_CAPACITY_DATA))
		size = sizeof(READ_FORMAT_CAPACITY_DATA);

	csw.bCSWStatus = CSWSTATUS_GOOD;
	csw.dCSWDataResidue = 0x0;
	csw.dCSWTag = cbw.dCBWTag;	
	pBus->StartTransfer(handle,pipe[1].ep,(u8 *) &readfcd, cbw.dCBWDataXferLength);

#if 0
	why ++;
	if ( why % 2 == 0) {
		csw.bCSWStatus = CSWSTATUS_GOOD;
		csw.dCSWDataResidue = 0xf0;
		csw.dCSWTag = cbw.dCBWTag;	
		pBus->StartTransfer(handle,pipe[1].ep,(u8 *) &readfcd, cbw.dCBWDataXferLength);
	}
	else
	{
		csw.dCSWDataResidue = cbw.dCBWDataXferLength;
		csw.bCSWStatus = CSWSTATUS_FAILED;
		csw.dCSWTag = cbw.dCBWTag;	
		pBus->StartTransfer(handle,pipe[1].ep,(u8 *) &readfcd, cbw.dCBWDataXferLength);
	}
#endif

	return 1;
	
}

static inline unsigned int Handle_UFI_MODE_SENSE_6(unsigned handle)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;

	static unsigned char sensedata[8] = {
		0x00,0x06, // lenght
		0x00,      // default media
		0x00,      // bit 7 is write protect
        0x00,0x00,0x00,0x00 //reserved
	};
	pBus->StartTransfer(handle,pipe[1].ep,sensedata, sizeof(sensedata));
    return 1;
}

static inline unsigned int USB_HandleCBW(unsigned int handle)
{
	if (cbw.dCBWSignature != CBWSIGNATURE)
		return 0;
    
	csw.dCSWSignature = CSWSIGNATURE;
	csw.bCSWStatus = CSWSTATUS_GOOD;
	csw.dCSWTag = cbw.dCBWTag;
	csw.dCSWDataResidue = 0;
//	if (cbw.bCBWLUN != 0 || (cbw.CBWCB[1] & 0xe0 ) != 0 )
//		printf("AAAAAAAAAAAAA %d \n",cbw.bCBWLUN);
/*
	dprintf("cbw.Signature:%08x\n", cbw.dCBWSignature);
	dprintf("cbw.dCBWTag:%08x\n", cbw.dCBWTag);
	dprintf("cbw.dCBWDataXferLength:%x\n", cbw.dCBWDataXferLength);
	dprintf("cbw.bmCBWFlags:%08x\n", cbw.bmCBWFlags);
	dprintf("cbw.bCBWLUN:%d\n", cbw.bCBWLUN);
	dprintf("cbw.bCBWCBLength:%d\n", cbw.bCBWCBLength);
	dprintf("cbw.CBWCB[0]:%02x\n", cbw.CBWCB[0]);
*/
	mass_state = SENDING_CSW;
	
	switch (cbw.CBWCB[0]) {
	case UFI_INQUIRY:
		Handle_UFI_INQUIRY(handle);
		break;
	case UFI_REQUEST_SENSE:
		Handle_UFI_REQUEST_SENSE(handle);
		break;
	case UFI_READ_CAPACITY:
		if ( !Handle_UFI_READ_CAPACITY(handle))
		{
			Handle_CSW(handle);
			mass_state = SENDED_CSW;
			csw.bCSWStatus = CSWSTATUS_GOOD;
		}
		break;
	case UFI_READ_10:
		Handle_UFI_READ_10(handle);
		break;
		
	case UFI_WRITE_10:
	case UFI_WRITE_AND_VERIFY:
		dprintf("UFI_WRITE_10\r\n");
		
		Handle_UFI_WRITE_10(handle);
		break;
	case UFI_READ_FORMAT_CAPACITY:
		Handle_UFI_READ_FORMAT_CAPACITY(handle);
		Handle_CSW(handle);
		mass_state = SENDED_CSW;

		break;
	case UFI_MODE_SENSE_10:
	case UFI_MODE_SENSE_6:
		Handle_UFI_MODE_SENSE_6(handle);
		break;
	case UFI_TEST_UNIT_READY:
		Handle_UFI_TEST_UNIT_READY(handle);
		break;
	default:
		Handle_CSW(handle);
		mass_state = SENDED_CSW;
		
		break;
	}

}


static unsigned int massDetect(unsigned int handle,unsigned int stat)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;
	unsigned char i;
    // udc gpio detect insert
	dprintf("massDetect udc_device_state = %x\n",udc_device_state);
	if(stat == UDC_JUDGE)
	{	if((udc_device_state & UDC_HW_CONNECT) == 0)
		{
			
			pBus->EnableDevice(handle);
			udc_device_state |= UDC_HW_CONNECT;
			
		}
	}else if(stat == UDC_REMOVE) //udc gpio detect remove 
	{
		
		if(udc_device_state & UDC_HW_CONNECT)
		{
			pBus->DisableDevice(handle);
			udc_device_state &= ~UDC_HW_CONNECT;
		}
		
		if(udc_device_state & UDC_SW_CONNECT)
		{
			for(i = 0;i < curLunCount; i++)
			{
				if(pDev[i])
					pDev[i]->DeinitDev((unsigned int)pDev[i]);
			}
			
			udc_device_state &= ~UDC_SW_CONNECT;
		}
		
	}
	else
		return 0;
	return 1;	
}

static unsigned int massreset(unsigned int handle)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;
	unsigned char ret = 1, i = 0;
	//printf("mass reset \r\n");
	
	pBus->InitEndpointSuppost(handle,&(pipe[0].ep),pipe[0].ep_type,&(pipe[0].max_pkg));
	pBus->InitEndpointSuppost(handle,&(pipe[1].ep),pipe[1].ep_type,&(pipe[1].max_pkg));
	pBus->InitEndpointSuppost(handle,&(pipe[2].ep),pipe[2].ep_type,&(pipe[2].max_pkg));

	devDesc.bMaxPacketSize0 = pipe[0].max_pkg;
	confDesc.endpoint_descriptor[0].bEndpointAddress = pipe[1].ep;
	confDesc.endpoint_descriptor[0].wMaxPacketSize = pipe[1].max_pkg;
	
	confDesc.endpoint_descriptor[1].bEndpointAddress = pipe[2].ep;
	confDesc.endpoint_descriptor[1].wMaxPacketSize = pipe[2].max_pkg;

#ifndef MUTI_THREAD
	massbuf = (unsigned char *)((unsigned int)massBuf0 | 0xa0000000);
#else
//	udc_trigger = 0;
//	mass_trigger = 0;
/*
	sem_buf0 = OSSemCreate(1);
	sem_buf1 = OSSemCreate(1);
	sem_mass = OSSemCreate(0);
	sem_device = OSSemCreate(1);
*/
#endif

	if(curLunCount == 0)
		return;
	ret = curLunCount;
	
	if(ret)
	{
		for(i = 0;i < curLunCount; i++)
		{
			if(pDev[i])
				pDev[i]->InitDev((unsigned int)pDev[i]);
			
		}
		
		devDesc.iSerialNumber = 3;	
		
	}
	udc_device_state |= UDC_SW_CONNECT;
	mass_state = RECEIVE_CBW;

	pBus->StartTransfer(handle,pipe[2].ep,(unsigned char *)&cbw,sizeof(cbw));	
	dprintf("iSerialNumber = %d\n",devDesc.iSerialNumber);
	
	return (unsigned int )ret;
	
}
static unsigned int masssuspend(unsigned int handle)
{
	unsigned char i;
	dprintf("masssuspend!\n");
	
	if(udc_device_state & UDC_SW_CONNECT)
	{
		for(i = 0;i < curLunCount; i++)
			if(pDev[i])
				pDev[i]->DeinitDev((unsigned int)pDev[i]);
		udc_device_state &= ~UDC_SW_CONNECT;
	}
	return 1;
}
static unsigned int Set_UDC_Speed(unsigned stat)
{
	switch(stat)
	{
	case UDC_HIGHSPEED:
		printf("UDC HIGHSPEED\r\n");
		break;
		
	case UDC_FULLSPEED:
		printf("UDC FULLSPEED\r\n");
		break;
		
	}
	return 1;
}

static inline void Get_Dev_Descriptor(unsigned int handle,USB_DeviceRequest *dreq)
{
    PUDC_BUS pBus = (PUDC_BUS)handle;
    unsigned short size = dreq->wLength;
	
	if(size < sizeof(devDesc))
	{
		devDesc.bLength = size;
		pBus->StartTransfer(handle,pipe[0].ep,(unsigned char *)&devDesc, size);
	}
	
	else
	{
		devDesc.bLength = sizeof(devDesc);
		pBus->StartTransfer(handle,pipe[0].ep,(unsigned char *)&devDesc, sizeof(devDesc));
	}
	
}

static inline void Get_DEV_Configuration(unsigned int handle,USB_DeviceRequest *dreq)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;
	switch (dreq->wLength) {
	case 9:
		pBus->StartTransfer(handle,pipe[0].ep, (unsigned char *)&confDesc, 9);
		break;
	case 8:
		pBus->StartTransfer(handle,pipe[0].ep, (unsigned char *)&confDesc, 8);
		break;
	default:
		pBus->StartTransfer(handle,pipe[0].ep, (unsigned char *)&confDesc, sizeof(confDesc));
		break;
	}
}

static inline void Get_DEV_QualifyDescitptor(unsigned int handle,USB_DeviceRequest *dreq)
{
    PUDC_BUS pBus = (PUDC_BUS)handle;
    pBus->StartTransfer(handle,pipe[0].ep, (unsigned char *)&devQualifyDesc, sizeof(devQualifyDesc));	
}


static inline unsigned int usbHandleClassDevReq(unsigned int handle,unsigned char *buf)
{
	u8 scsiLUN = 0;
	PUDC_BUS pBus = (PUDC_BUS)handle;
	switch (buf[1]) {
	case 0xfe:
		scsiLUN = curLunCount - 1;
		dprintf("Get max lun %d %x\n",scsiLUN,handle,pipe[0].ep);
		if (buf[0] == 0xa1)
			pBus->StartTransfer(handle,pipe[0].ep, (unsigned char *)&scsiLUN, 1);
		break;
	case 0xff:
		dprintf("Mass storage reset\n");
		break;
	}
	return 1;
	
}
static inline void Get_DEV_DescriptorString(unsigned int handle,USB_DeviceRequest *dreq)
{
	int size = dreq->wLength;
	PUDC_BUS pBus = (PUDC_BUS)handle;
	static u16 str_ret[] = {
		0x0336,    //0x1a=26 byte
		'F',
		'i',
		'l',
		'e',
		'-',
		'b',
		'a',
		'c',
		'k',
		'e',
		'd',
		'S',

		't',
		'o',
		'r',
		'a',
		'g',
		'e',
		'G',

		'a',
		'd',
		'g',
		'e',
		't'
	};
	static u16 str_lang[] = {
		0x0304,
		0x0409
	};

	static u16 str_isernum[] = {
		0x031a,
		'3',
		'2',
		'3',
		'8',
		'2',
		'0',
		'4',
		'7',
		'4',
		'0',
		'7',
		'7'
	};

	printf("sendDevDescString size = %d type %d \r\n",size,dreq->wValue & 0xff);

	switch ( dreq->wValue & 0xff )
	{
	case 0:       //land ids
		if ( size > sizeof(str_lang) )
			pBus->StartTransfer(handle,pipe[0].ep,(unsigned char *)str_lang,sizeof(str_lang));
		else
			pBus->StartTransfer(handle,pipe[0].ep,(unsigned char *)str_lang,size);
		return;
		break;
	case 2:       //iproduct
		if(size >= 36)
			size = 36;
		str_ret[0] = (0x0300 | size);
		pBus->StartTransfer(handle,pipe[0].ep,(unsigned char *)str_ret,size);
		break;
	case 3:       //iserialnumber
		if(size >= sizeof(str_isernum))
			size = sizeof(str_isernum);
//		str_isernum[0] = (0x0300 | size);
		pBus->StartTransfer(handle,pipe[0].ep,(unsigned char *)str_isernum,size);
		break;
	case 0xee:    //microsoft OS!
		str_isernum[0] = (0x0300 | size);
		pBus->StartTransfer(handle,pipe[0].ep,(unsigned char *)str_isernum,size);
		break;
	}
}

static unsigned int UDC_Setup_Handle(unsigned int handle,unsigned int stat,unsigned char *bufaddr,unsigned int len)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;
	USB_DeviceRequest *dreq = (USB_DeviceRequest *)bufaddr;
	if(len != 8)
		return 0;
	if(dreq->bmRequestType == 0xa1)
	{
		if(dreq->bRequest == 0xfe)
		{
			return usbHandleClassDevReq(handle,bufaddr);
			
		}
		return;
		
		
	}
	if(dreq->bmRequestType == 0x21)
	{
		if(dreq->bRequest == 0xff)
		{
			return usbHandleClassDevReq(handle,bufaddr);
		}
		
	}
	
	switch (dreq->bRequest) {
	case GET_DESCRIPTOR:
		if (dreq->bmRequestType == 0x80)	/* Dev2Host */
			switch(dreq->wValue >> 8) 
			{
			case DEVICE_DESCRIPTOR:
				dprintf("get device!\n");
				Get_Dev_Descriptor(handle,dreq);
				break;
			case CONFIGURATION_DESCRIPTOR:
				dprintf("get config!\n");
				Get_DEV_Configuration(handle,dreq);
				break;
			case STRING_DESCRIPTOR:
				dprintf("get string!\n");
				Get_DEV_DescriptorString(handle,dreq);
				break;
			case DEVICE_QUALIFIER_DESCRIPTOR:
				dprintf("get qualify!\n");
				Get_DEV_QualifyDescitptor(handle,dreq);
				break;
				
			}
		
		break;
	case SET_ADDRESS:
		dprintf("\nSET_ADDRESS!");
		pBus->SetAddress(handle,dreq->wValue);
		break;
		
	case GET_STATUS:
		switch (dreq->bmRequestType) {
		case 80:	/* device */
			massbuf[0] = 1;
			massbuf[1] = 0;
			pBus->StartTransfer(handle,pipe[0].ep,massbuf,2);			
			break;
		case 81:	/* interface */
		case 82:	/* ep */
			massbuf[0] = 0;
			massbuf[1] = 0;
			pBus->StartTransfer(handle,pipe[0].ep,massbuf,2);			
			break;
		}

		break;
   
	case CLEAR_FEATURE:
		printf("CLEAR_FEATURE!\r\n");
		break;
		
	case SET_CONFIGURATION:
		printf("SET_CONFIGURATION!\r\n");
//		pBus->StartTransfer(handle,pipe[0].ep,massbuf,0);			

		break;
		
	case SET_INTERFACE:
		printf("SET_INTERFACE!\r\n");
		break;
	case SET_FEATURE:
		printf("SET_FEATURE!\r\n");
		break;
	default:	
        printf("protal isn't surporst\r\n");
		
	}
	return 1;
}


static unsigned int HandleReceiveData(unsigned handle,unsigned char *buf,unsigned int len)
{
	PUDC_LUN pdev = pDev[cbw.bCBWLUN]; 
	PUDC_BUS pBus = (PUDC_BUS)handle;
	unsigned int ret;
	unsigned short sectors = len / g_sector_size;
   
//	ret = pdev->WriteDevSector((unsigned int)pdev,buf,g_start_sector,sectors);
#ifndef MUTI_THREAD
	ret = pdev->WriteDevSector((unsigned int)pdev,buf,g_start_sector,sectors);
#else
	if ( udc_trigger == 1 )
	{
		udc_trigger = 0;
		OSSemPost(sem_buf1);
	}
	else 
	{
		udc_trigger = 1;
		OSSemPost(sem_buf0);
	}
	
	OSSemPend(sem_device, 0, &err);    //lock device!
	ssector = g_start_sector;
	nsector = sectors;
	OSSemPost(sem_mass);                //wake mass task
#endif

	g_start_sector += sectors;
	g_nr_sectors -= sectors;
	if(g_nr_sectors)
	{
		sectors = g_nr_sectors * g_sector_size;
#ifdef MUTI_THREAD
		if ( udc_trigger == 1 )
		{
			massbuf = (unsigned char *)((unsigned int)massBuf1 | 0xa0000000);
			OSSemPend(sem_buf1, 0, &err);
		}
		else 
		{
			massbuf = (unsigned char *)((unsigned int)massBuf0 | 0xa0000000);
			OSSemPend(sem_buf0, 0, &err);
		}
#endif
		if(sectors > UDC_FRAME_SIZE)
			pBus->StartTransfer(handle,pipe[2].ep,massbuf,UDC_FRAME_SIZE);
		else
			pBus->StartTransfer(handle,pipe[2].ep,massbuf,sectors);
		
	}else
	{
		mass_state = SENDED_CSW;
		//send csw data
		Handle_CSW(handle);
		
		
	}
	return ret;
}

static unsigned int HandleReceive(unsigned handle,unsigned char *buf,unsigned int len)
{
	//printf("mass_state = %d,RECEIVE_CBW = %d\n",mass_state,RECEIVE_CBW);
	
	if(mass_state == RECEIVE_CBW)
	{
		return USB_HandleCBW(handle);
		
	}
	if(mass_state == RECEIVE_DATA) 
	{
		return HandleReceiveData(handle,buf,len);
	}
	
	return 1;
}
static unsigned int HandleSendData(unsigned handle,unsigned char *buf,unsigned int len)
{
	PUDC_LUN pdev = pDev[cbw.bCBWLUN]; 
	PUDC_BUS pBus = (PUDC_BUS)handle;
	unsigned short sectors = UDC_FRAME_SIZE / g_sector_size;
	if(g_nr_sectors)
	{
		
		if(sectors > g_nr_sectors)
			sectors = g_nr_sectors;
		
		
		pdev->ReadDevSector((unsigned int)pdev,massbuf,g_start_sector,sectors);
		pBus->StartTransfer(handle,pipe[1].ep, (unsigned char *)massbuf,
							sectors * g_sector_size);
		g_start_sector += sectors;	
		g_nr_sectors -= sectors;
	
	}else
	{
		mass_state = SENDED_CSW;
		//send csw data
		Handle_CSW(handle);
			
	}
	
	return 1;
}

static unsigned int HandleSend(unsigned handle,unsigned char *buf,unsigned int len)
{
	PUDC_BUS pBus = (PUDC_BUS)handle;
	if(mass_state == SEND_DATA)
	{
		return HandleSendData(handle,buf,len);
		
	}
	if(mass_state == SENDING_CSW)
	{
		mass_state = SENDED_CSW;
		Handle_CSW(handle);
		
	}
	
	if(mass_state == SENDED_CSW)
	{
		mass_state = RECEIVE_CBW;
		pBus->StartTransfer(handle,pipe[2].ep,(unsigned char *)&cbw,sizeof(cbw));
	}
	
	return 1;
}

static unsigned int Notify(unsigned int handle,unsigned int stat,unsigned char *bufaddr,unsigned int len)
{
//	printf("Bus Notify stat :%x \n",stat);
	
	if((stat < 0x200) && (stat >=0x100))
	   return massDetect(handle,stat);
	if(stat == UDC_RESET)
		return massreset(handle);
	if(stat == UDC_SUSPEND)
		return masssuspend(handle);
	if((stat >= UDC_FULLSPEED) && (stat <= UDC_HIGHSPEED))
		return Set_UDC_Speed(stat);
	if(stat == UDC_SETUP_PKG_FINISH)
		return UDC_Setup_Handle(handle,stat,bufaddr,len);
	if(stat == UDC_PROTAL_RECEIVE_FINISH)
		return HandleReceive(handle,bufaddr,len);
	if(stat == UDC_PROTAL_SEND_FINISH)
		return HandleSend(handle,bufaddr,len);
	return 0;
}

#ifdef MUTI_THREAD

void MassTask(void *arg)
{
	PUDC_LUN pdev ; 
//	MASS_ARGS *p ;
	u8 *Massbuf;
	u8 err;

	while (1) {
		OSSemPend(sem_mass, 0, &err);
		pdev = pDev[cbw.bCBWLUN]; 
		if ( mass_trigger == 0)
		{
			Massbuf = (unsigned char *)((unsigned int)massBuf0 | 0xa0000000);
			OSSemPend(sem_buf0, 0, &err);
			pdev->WriteDevSector((unsigned int)pdev,Massbuf,ssector,nsector);
			mass_trigger = 1;
			OSSemPost(sem_buf0);
		}
		else
		{
			Massbuf = (unsigned char *)((unsigned int)massBuf1 | 0xa0000000);
			OSSemPend(sem_buf1, 0, &err);
			pdev->WriteDevSector((unsigned int)pdev,Massbuf,ssector,nsector);
			mass_trigger = 0;
			OSSemPost(sem_buf1);
		}
		
//		}
		OSSemPost(sem_device);
	}
}
#endif

unsigned char InitMassStorage = 0;
void Init_Mass_Storage()
{

	if(InitMassStorage == 0)
	{
		CreateDevice(Notify);

#ifdef MUTI_THREAD
		sem_buf0 = OSSemCreate(1);
		sem_buf1 = OSSemCreate(1);
		sem_mass = OSSemCreate(0);
		sem_device = OSSemCreate(1);
		
//		massBuf0 = uc_malloc( sizeof(u8) * MASS_BUF_ZISE );
//		massBuf1 = uc_malloc( sizeof(u8) * MASS_BUF_ZISE );

//		printf("queue_mass %x %x %x\n",sem_mass,sem_buf0,sem_buf1);
		OSTaskCreate(MassTask, (void *)0,
			     (void *)&MassTaskStack[MASS_TASK_STK_SIZE - 1],
			     MASS_TASK_PRIO);
#endif
	}
	InitMassStorage = 1;
}
