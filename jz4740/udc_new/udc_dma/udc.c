#include <ucos_ii.h>
#include <jz4740.h>
#include "usb.h"
#include "udc.h"
#include "udcbus.h"

#ifdef USE_MIDWARE
#include "midware.h"
#endif

#define EP1_INTR_BIT 2
#define EP_FIFO_NOEMPTY 2

#define GPIO_UDC_DETE_PIN (32 * 3 + 6)
#define GPIO_UDC_DETE GPIO_UDC_DETE_PIN
#define IRQ_GPIO_UDC_DETE (IRQ_GPIO_0 + GPIO_UDC_DETE)
#define MAX_CABLE_DETE  3
#define CABLE_DETE_TIME  500                //wait time in ms
#define CABLE_CONNECTED          0
#define CABLE_DISCONNECT         1

#define DEBUGMSG 0
#if DEBUGMSG
#define dprintf printf
#else
#define dprintf(x...)
#endif

#define IS_CACHE(x) (x < 0xa0000000)

#ifdef USE_MIDWARE
volatile static MIDSRC udcsrc;
volatile static MIDSRCDTA res;
volatile static u32 udcid;
volatile static u32 cable_stat;
#endif

typedef struct
{
	unsigned char ep;
	unsigned char state;
	unsigned short fifosize;	
	unsigned int curlen;
	unsigned int totallen;
	unsigned int data_addr;
	unsigned int fifo_addr;
}EPSTATE,*PEPSTATE;

#define CPU_READ     0
#define CPU_WRITE     1

#define DMA_READ     2
#define DMA_WRITE     3


#define WRITE_FINISH  4
#define READ_FINISH  5

#define TXFIFOEP0 USB_FIFO_EP0

EPSTATE epstat[3] = {
	{0x00, 0,0,0,0,0,TXFIFOEP0 + 0},
	{0x81, 0,0,0,0,0,TXFIFOEP0 + 4},
	{0x01, 0,0,0,0,0,TXFIFOEP0 + 4}
};

#define GetEpState(x) (&epstat[x])

static OS_EVENT *udcEvent;
u8 USB_Version;
static unsigned char udc_irq_type = 0;

//void (*tx_done)(void) = NULL;

#define UDC_TASK_PRIO	7
#define UDC_TASK_STK_SIZE	1024 * 20
static OS_STK udcTaskStack[UDC_TASK_STK_SIZE];

#define PHYADDR(x) (x & 0x1fffffff)

static void DMA_ReceiveData(PEPSTATE pep)
{
	unsigned int size;
	size = pep->totallen - (pep->totallen % pep->fifosize);
	if(IS_CACHE(pep->data_addr))
		dma_cache_wback_inv(pep->data_addr, pep->totallen);
	//IntrOutMask = 0x0;
	
	
	jz_writel(USB_REG_ADDR2,PHYADDR(pep->data_addr));
	jz_writel(USB_REG_COUNT2,size);
	jz_writel(USB_REG_CNTL2,0x001d);

	jz_writeb(USB_REG_INDEX,1);	
	usb_setw(USB_REG_OUTCSR,0xa000);
	//printf("dma receive %x\n",size);    
	pep->curlen = size;
	
	
}
static void DMA_SendData(PEPSTATE pep)
{
	  unsigned int size;
	  
	  size = pep->totallen - (pep->totallen % pep->fifosize);
	  
	  if(IS_CACHE(pep->data_addr))
		  dma_cache_wback_inv(pep->data_addr, size);
	  jz_writel(USB_REG_ADDR1,PHYADDR(pep->data_addr));
	  jz_writel(USB_REG_COUNT1,size);//fifosize[ep]);
	  jz_writel(USB_REG_CNTL1,0x001f);
	  jz_writeb(USB_REG_INDEX, 1);
	  usb_setw(USB_REG_INCSR,0x9400);
	  
	  //printf("dma send %x\n",size);
	  pep->curlen = size;
}
static inline void DMA_SendData_Finish()
{
	jz_writel(USB_REG_CNTL1,0x0);
	jz_writeb(USB_REG_INDEX,1);
	usb_clearw(USB_REG_INCSR,0x9400);
	
}

static void DMA_ReceieveData_Finish()
{
	dprintf("\n Disable DMA!");
	
	jz_writel(USB_REG_CNTL2,0x0);
	jz_writeb(USB_REG_INDEX,1);
	usb_clearw(USB_REG_OUTCSR,0xa000);
}



static void udc_reset(unsigned int handle)
{
//	PUDC_BUS pBus = (PUDC_BUS) handle;
	u8 byte;
	PEPSTATE pep;
	
	//data init
	
	//ep0state = USB_EP0_IDLE;
//	IntrOutMask = 0x2;

	// __cpm_stop_udc();
	/* Enable the USB PHY */
	//REG_CPM_SCR |= CPM_SCR_USBPHY_ENABLE;
	/* Disable interrupts */

	jz_writew(USB_REG_INTRINE,0);
	jz_writew(USB_REG_INTROUTE, 0);
	jz_writeb(USB_REG_INTRUSBE, 0);

	jz_writeb(USB_REG_FADDR,0);
//jz_writeb(USB_REG_POWER,0x60);   //High speed

	jz_writeb(USB_REG_INDEX,0);
	jz_writeb(USB_REG_CSR0,0xc0);

	jz_writeb(USB_REG_INDEX,1);
	jz_writew(USB_REG_INCSR,0x2048);
	jz_writew(USB_REG_INMAXP,512);
	
	jz_writeb(USB_REG_INDEX,1);
	jz_writew(USB_REG_OUTCSR,0x0090);
	jz_writew(USB_REG_OUTMAXP,512);
	byte = jz_readb(USB_REG_POWER);
	dprintf("REG_POWER: %02x\r\n",byte);
	epstat[0].fifosize = 64;
	
	if(epstat[1].state != WRITE_FINISH)
		jz_readw(USB_REG_INTRIN);
	if(epstat[2].state != READ_FINISH)
		jz_readw(USB_REG_INTROUT);
	
	epstat[0].state = 0;
	epstat[1].state = WRITE_FINISH;
	epstat[2].state = READ_FINISH;
	epstat[0].curlen = 0;
	epstat[1].curlen = 0;
	epstat[2].curlen = 0;
	epstat[0].totallen = 0;
	epstat[1].totallen = 0;
	epstat[2].totallen = 0;
	
	if ((byte & 0x10)==0) 
	{
		jz_writeb(USB_REG_INDEX,1);
		jz_writew(USB_REG_INMAXP,64);
		jz_writew(USB_REG_INCSR,0x2048);
		jz_writeb(USB_REG_INDEX,1);
		jz_writew(USB_REG_OUTMAXP,64);
		jz_writew(USB_REG_OUTCSR,0x0090);
		USB_Version=USB_FS;

		//fifosize[1]=64;
      
		epstat[1].fifosize = 64;
		epstat[2].fifosize = 64;
		printf("usb1.1\n");
		

	}
	else
	{
		jz_writeb(USB_REG_INDEX,1);
		jz_writew(USB_REG_INMAXP,512);
		jz_writew(USB_REG_INCSR,0x2048);
		jz_writeb(USB_REG_INDEX,1);
		jz_writew(USB_REG_OUTMAXP,512);
		jz_writew(USB_REG_OUTCSR,0x0090);
		USB_Version=USB_HS;
		epstat[1].fifosize = 512;
		epstat[2].fifosize = 512;
		printf("usb2.0\n");
		
	}
//DMA USE
	jz_writel(USB_REG_CNTL1,0);
	
	jz_writel(USB_REG_ADDR1,0);
	jz_writel(USB_REG_COUNT1,0);//fifosize[ep]);

	jz_writel(USB_REG_CNTL2,0);
	jz_writel(USB_REG_ADDR2,0);
	jz_writel(USB_REG_COUNT2,0);//fifosize[ep]);

	jz_readb(USB_REG_INTR); // clear dma interrupt
	jz_writew(USB_REG_INTRINE,0x1);   //enable ep0 intr
	jz_writeb(USB_REG_INTRUSBE,0x6);
//	DisableDevice();	
}


static void udcReadFifo(PEPSTATE pep, int size)
{
	unsigned int *ptr = (unsigned int *)(pep->data_addr + pep->curlen);
	unsigned int fifo = pep->fifo_addr;
	unsigned char *c;
	int s = size / 4;
	unsigned int x;
	if(((unsigned int )ptr & 3) == 0)
	{
		while(s--)
			*ptr++ = REG32(fifo);
			
	}
	else
	{
		while(s--)
		{
			x = REG32(fifo);
			*ptr++ = (x >> 0)& 0x0ff;
			*ptr++ = (x >> 8)  & 0x0ff;
			*ptr++ = (x >> 16) & 0x0ff;
			*ptr++ = (x >> 24) & 0xff;
		}
	}
	
	s = size & 3;
	c = (unsigned char *)ptr;
	while(s--)
		*c++ = REG8(fifo);
	pep->curlen += size;
	
	
#if 0

	c = (unsigned char *)(pep->data_addr + pep->curlen - size);
	dprintf("recv:(%d)", size);
	for (s=0;s<size;s++) {
		if (s % 16 == 0)
			dprintf("\n");
		dprintf(" %02x", *(c+s));
	}
	dprintf("\n");
#endif
}

static void udcWriteFifo(PEPSTATE pep, int size)
{
	unsigned int *d = (unsigned int *)(pep->data_addr + pep->curlen);
	unsigned int fifo = pep->fifo_addr;
	u8 *c;
	int s, q;

#if 0
	unsigned char *ptr =(unsigned char *)d;
	
	dprintf("send:fifo(%x) = (%d)",fifo, size);
	for (s=0;s<size;s++) {
		if (s % 16 == 0)
			dprintf("\n");
		dprintf(" %02x", ptr[s]);
	}
	dprintf("\n");
#endif
	
	if (size > 0) {
		s = size >> 2;
		while (s--)
			REG32(fifo) = *d++;
		q = size & 3;
		if (q) {
			c = (u8 *)d;
			while (q--)
				REG8(fifo) = *c++;
		}
	} 
	pep->curlen += size;
}
void EP0_Handler (unsigned int handle)
{
    PEPSTATE pep = GetEpState(0);
	unsigned char byCSR0;

/* Read CSR0 */
	jz_writeb(USB_REG_INDEX, 0);
	byCSR0 = jz_readb(USB_REG_CSR0);
   
//	printf("EP0 CSR = %x\n",byCSR0);
    if(byCSR0 == 0)
		return;
	
/* Check for SentStall 
   if sendtall is set ,clear the sendstall bit*/
	if (byCSR0 & USB_CSR0_SENTSTALL) 
	{
		jz_writeb(USB_REG_CSR0, (byCSR0 & ~USB_CSR0_SENDSTALL));
		pep->state = CPU_READ;
		
		printf("\nSentstall!");
		return;
	}
/* Check for SetupEnd */
	if (byCSR0 & USB_CSR0_SETUPEND) 
	{
		jz_writeb(USB_REG_CSR0, (byCSR0 | USB_CSR0_SVDSETUPEND));
		pep->state = CPU_READ;
		printf("\nSetupend!");
		//return;
	}
/* Call relevant routines for endpoint 0 state */
	if (pep->state == CPU_READ) 
	{
		if (byCSR0 & USB_CSR0_OUTPKTRDY)   //There are datas in fifo
		{
            USB_DeviceRequest dreq;
			
            *(unsigned int *) &dreq =  REG32(pep->fifo_addr);
			*(unsigned int *) ((unsigned int)&dreq + 4)=  REG32(pep->fifo_addr);
		
			
			dprintf("\nbmRequestType:%02x\nbRequest:%02x\n"
					"wValue:%04x\nwIndex:%04x\n"
					"wLength:%04x\n",
					dreq.bmRequestType,
					dreq.bRequest,
					dreq.wValue,
					dreq.wIndex,
					dreq.wLength);

			if ( dreq.bRequest == SET_ADDRESS || dreq.bRequest == SET_CONFIGURATION || dreq.bRequest == CLEAR_FEATURE )
				usb_setb(USB_REG_CSR0, 0x40 | USB_CSR0_DATAEND);//clear OUTRD bit and DATAEND
			else
				usb_setb(USB_REG_CSR0, 0x40);//clear OUTRD bit

			BusNotify(handle,UDC_SETUP_PKG_FINISH,(unsigned char *)&dreq,8);
		}
		else 
		{
			dprintf("0:R DATA\n");
			
		}
		
	}
	
	if (pep->state == CPU_WRITE) 
	{
		int sendsize;
		sendsize = pep->totallen - pep->curlen;
//		printf("send size = %d\r\n",sendsize);
		
		if (sendsize < 64) 
		{
			udcWriteFifo(pep,sendsize);
			pep->curlen = pep->totallen;
			usb_setb(USB_REG_CSR0, USB_CSR0_INPKTRDY | USB_CSR0_DATAEND);
			pep->state = CPU_READ;
			
		} else 
		{
			if(sendsize)
			{
				udcWriteFifo(pep, 64);
				usb_setb(USB_REG_CSR0, USB_CSR0_INPKTRDY);
				pep->curlen += 64;
				
			}else
				pep->state = CPU_READ;
			
		}
	}
//	printf("pep state = %d %d\r\n",CPU_WRITE,pep->state);

	return;
}

void EPIN_Handler(unsigned int handle,PEPSTATE pep)
{
	unsigned int size;
   
	
	//fifo = fifoaddr[EP];
	size = pep->totallen - pep->curlen;
//	printf("pep->totallen = %d size = %d \n",pep->totallen,size);
	if(size == 0)
	{
		
		pep->state = WRITE_FINISH;
		usb_clearw(USB_REG_INTRINE,EP1_INTR_BIT);  // close ep1 in intr
		BusNotify(handle,UDC_PROTAL_SEND_FINISH,(unsigned char *)pep->data_addr,pep->curlen);
//		printf("Send finish \n");
		return;
		
	}
	
	if(size < pep->fifosize)
	{
		udcWriteFifo(pep,size);
	}else
		udcWriteFifo(pep,pep->fifosize);
	usb_setb(USB_REG_INCSR, USB_INCSR_INPKTRDY);
	
}

void EPOUT_Handler(unsigned int handle,PEPSTATE pep)
{
    unsigned int size;
	
	jz_writeb(USB_REG_INDEX, 1);

	size = jz_readw(USB_REG_OUTCOUNT);	
 	udcReadFifo(pep, size);
	usb_clearb(USB_REG_OUTCSR,USB_OUTCSR_OUTPKTRDY);
	pep->state = CPU_READ;
	dprintf("EPOUT: totallen = %d curlen = %d",pep->totallen,pep->curlen);
	
	if(pep->totallen == pep->curlen)
	{
		pep->state = READ_FINISH;
		usb_clearw(USB_REG_INTROUTE,EP1_INTR_BIT);
		BusNotify(handle,UDC_PROTAL_RECEIVE_FINISH,
			  (unsigned char *)pep->data_addr,pep->curlen);
		
		
	}
	
	//	USB_HandleUFICmd();
	dprintf("\nEPOUT_handle return!");
}
void udcIntrbhandle(unsigned int handle,unsigned char val)
{
	unsigned char byte;
	
	if (val & USB_INTR_RESET) 
	{
		dprintf("UDC reset intrupt!\r\n");  
		udc_reset(handle);
		
		byte = jz_readb(USB_REG_POWER);
		//dprintf("REG_POWER: %02x\r\n",byte);

		if ((byte&0x10) ==0 ) 
		{
			BusNotify(handle,UDC_FULLSPEED,0,0);
		}else
			BusNotify(handle,UDC_HIGHSPEED,0,0);
		// enable USB Suspend interrupt
		
		byte = jz_readb(USB_REG_INTRUSBE);
		jz_writeb(USB_REG_INTRUSBE,byte | USB_INTR_SUSPEND);
      
		BusNotify(handle,UDC_RESET,0,0);
		
	}
	if(val & USB_INTR_SUSPEND)
	{
		BusNotify(handle,UDC_SUSPEND,0,0);
		
		byte = jz_readb(USB_REG_INTRUSBE);
		jz_writeb(USB_REG_INTRUSBE,(byte & (~USB_INTR_SUSPEND) & 7));
#ifdef USE_MIDWARE
		printf("udc suspend %x\n",byte);
		if ( cable_stat == CABLE_CONNECTED )
		{
			printf("USB uninstall ! \n");
			udcsrc.Src = SRC_UDC;
			udcsrc.Event = EVENT_UNINSTALL;
			OSQPost(udcsrc.CurEvent1 , (void *)&udcid);
			OSSemPost(udcsrc.CurEvent);

		}
#endif
	}
	if(val & 2)
	{
		printf("udc resume\n");
	}
	
	
}

void udc4740Proc (unsigned int handle)
{
	u8	IntrUSB = 0;
	u16	IntrIn = 0;
	u16	IntrOut = 0;
	u16 IntrDMA = 0;
	PEPSTATE pep;
	cli();
/* Read interrupt regiters */
	IntrUSB = jz_readb(USB_REG_INTRUSB);
/* Check for resume from suspend mode */
	if(IntrUSB != 8)
		udcIntrbhandle(handle,IntrUSB);
	/* Check for endpoint 0 interrupt */
	
	IntrIn  = jz_readw(USB_REG_INTRIN);
//	if(jz_readw(USB_REG_INTROUTE))
	IntrOut = jz_readw(USB_REG_INTROUT);
	//printf("IntrIn = %x IntrOut = %x\n",IntrIn,IntrOut);
	
	if (IntrIn & USB_INTR_EP0) 
	{
		//dprintf("\nUDC EP0 operations!\n");
		EP0_Handler(handle);
	}	

	pep = GetEpState(1);
	if(pep->state == CPU_WRITE)
	{
		if (IntrIn & 2) 
		{
			dprintf("\nUDC EP1 IN operation!");
			EPIN_Handler(handle,pep);
			//return;
		}
	}
	
	IntrDMA = jz_readb(USB_REG_INTR);
	//printf("IntrDMA = %x\n",IntrDMA);
	
	if(pep->state == DMA_WRITE)
	{
		dprintf("dma write intr = %x\n",IntrDMA);

				
		if(IntrDMA & 1)
		{
			
			dprintf("addr %x,count %x,cntl %x\r\n",
				   jz_readl(USB_REG_ADDR1),jz_readl(USB_REG_COUNT1),jz_readl(USB_REG_CNTL1));
			DMA_SendData_Finish();
			
			if(pep->curlen != pep->totallen)
			{
				/*??????????????*/
				printf("cur_len %d,totallen %d\r\n",pep->curlen,pep->totallen);
				usb_setw(USB_REG_INTRINE,EP1_INTR_BIT);  // open ep1 in intr
				pep->state = CPU_WRITE;
				EPIN_Handler(handle,pep);
				
			   
                 //jz_writeb(USB_REG_INDEX, 1);
				//usb_setb(USB_REG_INCSR, USB_INCSR_INPKTRDY);
			}else 
			{
				pep->state = WRITE_FINISH;
				usb_clearw(USB_REG_INTRINE,EP1_INTR_BIT);  // close ep1 in intr
				BusNotify(handle,UDC_PROTAL_SEND_FINISH,
						   (unsigned char *)(pep->data_addr),pep->curlen);
				
				
			}
			//return;
		}
		

	}
	
	pep = GetEpState(2);
	if(pep->state == CPU_READ)
	{
		
		if ((IntrOut /*& IntrOutMask*/ ) & 2) 
		{
			dprintf("UDC EP1 OUT operation!\n");
			EPOUT_Handler(handle,pep);
			//return;		
		}
	}
	if(pep->state == DMA_READ)
	{
		if(IntrDMA == 0)
			IntrDMA = jz_readb(USB_REG_INTR);

		dprintf("\nDMA_REA intrDMA = %x\n",IntrDMA);
		
		if (IntrDMA & 0x2)     //channel 2 :OUT
		{
			dprintf("\n INTR 2!");
			DMA_ReceieveData_Finish();
			if((pep->totallen % pep->fifosize) != 0)
			{
				usb_setw(USB_REG_INTROUTE,EP1_INTR_BIT);	//Enable Ep Out
				pep->state = CPU_READ;
				EPOUT_Handler(handle,pep);
			}else
			{

				pep->state = READ_FINISH;
				BusNotify(handle,UDC_PROTAL_RECEIVE_FINISH,
						  (unsigned char *)(pep->data_addr),pep->curlen);
			}
			//return;
			

		}
	}
	sti();
	//dprintf("\n UDCProc finish!\n");
	
	//return;
}

void GPIO_Handle(unsigned int arg)
{
	//__gpio_ack_irq(GPIO_UDC_DETE);
	__gpio_mask_irq(GPIO_UDC_DETE);
	udc_irq_type |= 0x10;
	OSSemPost(udcEvent);
}

static void GPIO_IST(void * arg)
{
	PUDC_BUS pBus = (PUDC_BUS) arg;
	unsigned int stat = 0;
	unsigned int gpio_state = 0;
	unsigned int count = 0;
	static unsigned int judge = 0, i;
	u8 byte1 , byte2 ,err;

	dprintf("\n GPIO IRQ!!");
	gpio_state = __gpio_get_pin(GPIO_UDC_DETE);
	while(1)
	{
		
		udelay(1);
		if(gpio_state == __gpio_get_pin(GPIO_UDC_DETE))
		{
			count++;
			if(count > 2000)
				break;
		}else
		{
			count = 0;
			gpio_state = __gpio_get_pin(GPIO_UDC_DETE);
		}
		
	}
//	printf("gpio state = %x\r\n",gpio_state);
	
	if (gpio_state == 0)
	{
//		printf("\n UDC_DISCONNECT!");
		//disable udc phy!
		
		if(judge == 1)
		{
			__gpio_as_irq_rise_edge(GPIO_UDC_DETE);
			stat = UDC_REMOVE;
			judge = 0;
		
		}
#ifdef USE_MIDWARE
		if ( cable_stat == CABLE_CONNECTED )
		{
			printf("USB cable out! \n");
			udcsrc.Src = SRC_UDC;
			udcsrc.Event = EVENT_USB_OUT;
			OSQPost(udcsrc.CurEvent1 , (void *)&udcid);
			OSSemPost(udcsrc.CurEvent);
			cable_stat = CABLE_DISCONNECT;
		}

#endif
	}else if (gpio_state == 1)
	{
//		printf("\n UDC_CONNECT!");
		//enable udc phy!
		if(judge == 0)
		{
			__gpio_as_irq_fall_edge(GPIO_UDC_DETE);
			stat = UDC_JUDGE;
			judge = 1;
		}
		
	}

#ifdef USE_MIDWARE	
	if ( cable_stat == CABLE_CONNECTED )
		return;

	if ( stat == UDC_JUDGE )
	{
		//for test USB or POWER cable!!
		EnableDevice(NULL);
		for ( i = 0; i <= MAX_CABLE_DETE; i ++)
		{
			//sleep wait a while!
			OSTimeDlyHMSM(0,0,0,CABLE_DETE_TIME);
 			byte1 = jz_readb(USB_REG_POWER);
			byte2 = jz_readb(USB_REG_INTRUSB);
			if ( ( byte1 & 0x08 )|| ( byte2 & 0x04 ) ) //reset occur!
				break;
			else 
				printf("Wait reset time out! \n");
		}
		DisableDevice(NULL);
		cable_stat = CABLE_CONNECTED;

		if ( i > MAX_CABLE_DETE ) //power cable!
		{
			printf("Power cable insert! \n");
			udcsrc.Src = SRC_UDC;
			udcsrc.Event = EVENT_POWER_IN;
			OSQPost(udcsrc.CurEvent1 , (void *)&udcid);
			OSSemPost(udcsrc.CurEvent);
			//do no thing when power cable!
		}
		else                      //usb cable!!
		{
			udcsrc.Src = SRC_UDC;
			udcsrc.Event = EVENT_USB_IN;
			OSQPost(udcsrc.CurEvent1 , (void *)&udcid);
			OSSemPost(udcsrc.CurEvent);
			printf("USB cable insert! \n");

//			printf("read val %d \n",res.Val);
			if ( res.Val == 1 || res.Val == 0xffff) //up layer said yes!
			{
				BusNotify((unsigned int)pBus,stat,0,0);
			}
			else           //up layer said no!
			{
				printf("As power cable insert! \n");
				udcsrc.Src = SRC_UDC;
				udcsrc.Event = EVENT_POWER_IN;	
				OSQPost(udcsrc.CurEvent1 , (void *)&udcid);
				OSSemPost(udcsrc.CurEvent);
			}
		}
	}
	else
#endif
		if(stat != 0)
			BusNotify((unsigned int)pBus,stat,0,0);
	
}

void GPIO_IRQ_init(PUDC_BUS pBus)
{
	int err = 0;
	__gpio_as_irq_rise_edge(GPIO_UDC_DETE);
	request_irq(IRQ_GPIO_UDC_DETE, GPIO_Handle, pBus);
	__gpio_disable_pull(GPIO_UDC_DETE);
	REG_CPM_SCR &= ~CPM_SCR_USBPHY_ENABLE;  //disable UDC_PHY
	jz_writeb(USB_REG_POWER,0x60);   //High speed	
}

static void udcIntrHandler(unsigned int arg)
{
	
	__intc_mask_irq(IRQ_UDC);
	udc_irq_type |= 0x1;
	//dprintf("UDC irq\r\n");
	OSSemPost(udcEvent);
}

static void udcTaskEntry(void *arg)
{
	u8 err;
	while (1) {
		OSSemPend(udcEvent, 0, &err);
//		printf("udc_irq_type = %x\r\n",udc_irq_type);
		
		if(udc_irq_type & 0x10)
		{
			GPIO_IST(arg);
			udc_irq_type &= ~0x10;
			__gpio_unmask_irq(GPIO_UDC_DETE);
		}
		if(udc_irq_type & 1)
		{
			udc4740Proc((unsigned int)arg);
			udc_irq_type &= ~0x1;
			__intc_unmask_irq(IRQ_UDC);
			
		}
	}
}


/*   interface   */
static void SetAddress(unsigned int handle,unsigned short value)
{
	printf("Set address %d\r\n",value);
	jz_writeb(USB_REG_FADDR,value);
}

static void EnableDevice(unsigned int handle)
{
	__cpm_start_udc();
	REG_CPM_SCR |= CPM_SCR_USBPHY_ENABLE;
	jz_writeb(USB_REG_POWER,0x60);     //enable sofeconnect
//	jz_writeb(USB_REG_POWER,0x40);     //1.1 mode
	__intc_unmask_irq(IRQ_UDC);
	printf("Enable USB Phy!\r\n");
	
}
static void DisableDevice(unsigned int handle)
{
	
	REG_CPM_SCR &= ~CPM_SCR_USBPHY_ENABLE;
	jz_writeb(USB_REG_POWER,0x00);      //disable sofeconnet!
	__cpm_stop_udc();
	printf("Disable USB Phy!\r\n");
}
void StartTransfer(unsigned int handle,unsigned char ep,unsigned char *buf,unsigned int len)
{
	PEPSTATE pep;
	dprintf("StartTransfer ep = %x buf:%08x len = %d\r\n",ep,buf,len);
	unsigned char state;
	switch(ep)
	{
	case 0:
		pep = GetEpState(0);
		pep->totallen = len;
		pep->curlen = 0;
		pep->data_addr = (unsigned int)buf;		
		pep->state = CPU_WRITE;
		
		break;
	case 0x81:
		pep = GetEpState(1);
		pep->totallen = len;
		pep->curlen = 0;
		pep->data_addr = (unsigned int)buf;
		usb_setw(USB_REG_INTRINE,EP1_INTR_BIT); //open ep1 in intr

		if(len < pep->fifosize)
		{
			pep->state = CPU_WRITE;
			
			jz_writeb(USB_REG_INDEX,1);
			state =  jz_readw(USB_REG_INCSR);
			if(!(state & EP_FIFO_NOEMPTY))
			{
				len = len > pep->fifosize ? pep->fifosize :len;
				udcWriteFifo(pep,len);
				usb_setw(USB_REG_INCSR, USB_INCSR_INPKTRDY);		
				
			}
					
				
		}
		else
		{
			jz_writeb(USB_REG_INDEX,1);
			state =  jz_readw(USB_REG_INCSR);
			pep->state = DMA_WRITE;
			if(!(state & EP_FIFO_NOEMPTY))
				DMA_SendData(pep);	
		}
		break;
		
	case 0x1:
		pep = GetEpState(2);
		pep->totallen = len;
		pep->curlen = 0;
		pep->data_addr = (unsigned int)buf;
		if(len < pep->fifosize)
		{
			
			pep->state = CPU_READ;
			usb_setw(USB_REG_INTROUTE,EP1_INTR_BIT);	//Enable Ep Out
			
		}
		else
		{
			pep->state = DMA_READ;
			DMA_ReceiveData(pep);
			
		}

		break;
	case 0xff:             //mean send stall!
//		printf("Send Stall! %x \n",jz_readw(USB_REG_INCSR));
		usb_setw( USB_REG_INCSR, 0x10);	//set stall
		while( ! (jz_readw(USB_REG_INCSR) & 0x20 ) );                //wait stall sent!
		usb_setw( USB_REG_INCSR, 0x60);	             //clear datatag!
		usb_clearw( USB_REG_INCSR, 0x10);	             //clear sendstall
		usb_clearw( USB_REG_INCSR, 0x20);	             //clear sentstall
//		printf("Clear stall! %x \n",jz_readw(USB_REG_INCSR));

		break;
	}
}
void InitEndpointSuppost(unsigned int handle,unsigned char *ep,USB_ENDPOINT_TYPE ep_type,unsigned short *ep_max_pkg)
{
	PEPSTATE pep;

	if(ep_type == ENDPOINT_TYPE_CONTROL)
	{
		*ep = 0;
		*ep_max_pkg = MAX_EP0_SIZE;
		
	}
	if(ep_type == ENDPOINT_TYPE_BULK)
	{
		if(*ep & 0x80)
			pep = GetEpState(1);
		else
			pep = GetEpState(2);
		
		*ep = pep->ep;
		*ep_max_pkg = pep->fifosize;
	}
//	printf("ep = %x ep_type = %x epmax = %x\r\n",*ep,ep_type,pep->fifosize);	
}

#ifdef USE_MIDWARE
static void GetRequest(MIDSRCDTA *dat)
{
	dat->Val = res.Val;
//	printf("Up layer get :%d \n",res.Val);
}

static void Response(MIDSRCDTA *dat)
{

	res.Val = dat->Val;
	printf("Up layer said :%d \n",res.Val);

}
#endif

void InitUDC(PUDC_BUS pBus)
{
	pBus->EnableDevice = EnableDevice;
	pBus->SetAddress = SetAddress;
	pBus->StartTransfer = StartTransfer;
	pBus->InitEndpointSuppost = InitEndpointSuppost;
	pBus->DisableDevice = DisableDevice;
	printf("Init UDC %s %s\n",__DATE__,__TIME__);

#ifdef USE_MIDWARE
	udcsrc.GetRequest = GetRequest;
	udcsrc.Response = Response;
	udcsrc.Name = "UDC";
	printf("Register Midware SRC udc! \n");
	RegisterMidSrc(&udcsrc);
	udcid = udcsrc.ID;
	res.Val = 0xffff;
	cable_stat = CABLE_DISCONNECT;
#endif	
	dprintf("Init UDC\n");
	USB_Version=USB_HS;
	__intc_mask_irq(IRQ_UDC);
	__gpio_mask_irq(GPIO_UDC_DETE);
	
	udcEvent = OSSemCreate(0);
	dprintf("UDC with DMA reset!!\r\n");
	request_irq(IRQ_UDC, udcIntrHandler, 0);
	GPIO_IRQ_init(pBus);
	udc_reset((unsigned int)pBus);
	dprintf("UDC with DMA reset finish!!\r\n");
	dprintf("Create UDC Task!!\r\n");
	OSTaskCreate(udcTaskEntry, (void *)pBus,
		     (void *)&udcTaskStack[UDC_TASK_STK_SIZE - 1],
		     UDC_TASK_PRIO);
	
	__gpio_unmask_irq(GPIO_UDC_DETE);
//	__intc_unmask_irq(IRQ_UDC);

	if ( __gpio_get_pin(GPIO_UDC_DETE) == 1 )
	{
		__gpio_mask_irq(GPIO_UDC_DETE);
		udc_irq_type |= 0x10;
		OSSemPost(udcEvent);
	}

}

