/* Jz ethernet support
 *
 *  Copyright (c) 2007
 *  Ingenic Semiconductor, <jgao@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* ********************************************************************* */

#include "jz4740.h"
#include "jz_cs8900.h"
#include "ethif_cs8900_jz4740.h"

#include "lwip/debug.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "netif/etharp.h"

/* ********************************************************************* */
/* File local definitions */

/* Define those to better describe your network interface. */
#define Print_Header
#define IFNAME0 'e'
#define IFNAME1 'n'

/* ********************************************************************* */
#define PKTSIZE_ALIGN 2048
#define PKTALIGN      32
#define PKTBUFSRX     16
//#define MIN_PACKET_SIZE 76
#define MIN_PACKET_SIZE 60
static int next_rx;
static int next_tx;

volatile u8_t  PktBuf[(PKTBUFSRX+1) * PKTSIZE_ALIGN + PKTALIGN] __attribute__ ((aligned (PKTSIZE_ALIGN)));
volatile u8_t *NetTxPacket = 0; 
volatile u8_t *NetRxPackets[PKTBUFSRX];
#define ETH_GPIO 59
/* packet page register access functions */

#ifdef CS8900_BUS32
/* we don't need 16 bit initialisation on 32 bit bus */
#define get_reg_init_bus(x) get_reg((x))
#else
static unsigned short get_reg_init_bus (int regno)
{
	/* force 16 bit busmode */
	volatile unsigned char c;
    //c = CS8900_BUS16_0;
	//c = CS8900_BUS16_1;
	//c = CS8900_BUS16_0;
	//c = CS8900_BUS16_1;
	//c = CS8900_BUS16_0;

	CS8900_PPTR = regno;
//	printf("CS8900_PPTR:%08x\n",&CS8900_PPTR);
	udelay(100);
	
	return (unsigned short) CS8900_PDATA;
}
#endif

//MAC 
#if 1
#define ETHADDR0 0xff 
#define ETHADDR1 0xff 
#define ETHADDR2 0xff 
#define ETHADDR3 0xff 
#define ETHADDR4 0x20 
#define ETHADDR5 0x10
#endif 
#if 0
#define ETHADDR0 0x68 
#define ETHADDR1 0x68 
#define ETHADDR2 0x68 
#define ETHADDR3 0x68 
#define ETHADDR4 0x68 
#define ETHADDR5 0x68 
#endif 
/////////////***************get timer******************/////////////////////////////

#define TIMER_CHAN  0
#define TIMER_FDATA 0xffff  /* Timer full data value */
#define TIMER_HZ    (CFG_HZ/4)
static ulong timestamp;
static ulong lastdec;
#define READ_TIMER  REG_TCU_TCNT(TIMER_CHAN)  /* macro to read the 16 bit timer */

ulong get_timer(ulong base)
{
	return get_timer_masked () - base;
}

ulong get_timer_masked (void)
{
	ulong now = READ_TIMER;

	if (lastdec <= now) {
		/* normal mode */
		timestamp += (now - lastdec);
	} else {
		/* we have an overflow ... */
		timestamp += TIMER_FDATA + now - lastdec;
	}
	lastdec = now;

	return timestamp;
}

static unsigned short get_reg (int regno)
{
	CS8900_PPTR = regno;
	return (unsigned short) CS8900_PDATA;
}


static void put_reg (int regno, unsigned short val)
{
	CS8900_PPTR = regno;
	CS8900_PDATA = val;
}

static void eth_reset (void)
{
	int tmo,i;
	unsigned short us;

	/* reset NIC */
	put_reg (PP_SelfCTL, get_reg (PP_SelfCTL) | PP_SelfCTL_Reset);

	/* wait for 200ms */
	for (i=0; i<200; i++) {
		udelay (1000);
	}
	/* Wait until the chip is reset */

	tmo = get_timer (0) + 1 * CFG_HZ;
	while ((((us = get_reg_init_bus (PP_SelfSTAT)) & PP_SelfSTAT_InitD) == 0)
		   && tmo < get_timer (0))
		/*NOP*/;
        if(tmo < get_timer(0))
		printf("reset overtime!!\r\n");
}

static void eth_reginit (void)
{
	int i;
	put_reg (PP_LineCTL, (0x0013U | 0x0080U/*SerTxOn*/ | 0x0040U/*SerRxOn*/)); 
 	put_reg (PP_RxCTL,(0x0005U | 0x0800U | 0x0400U | 0x0100U)); 
	put_reg (PP_RxCFG,(0x0003U | 0x0100U));        // enable receive interrupt 
	put_reg (PP_TxCFG,(0x0007U | 0x0000U));                    // disable transmit interrupt (is default) 
	put_reg (PP_IntReg,0x0000U);                            // use interrupt number 0 
    	put_reg (PP_BufCFG,0x000bU); 
    	put_reg (PP_BusCTL,(0x0017U | 0x8000U));// enable interrupt generation 

	/** 
	 * @note You MUST wait 30 ms before accessing the CS8900 
	 */ 
        for (i = 0; i < 300; i++)
	{
		udelay(1000);  
	}
}

static void cs8900_get_enetaddr (unsigned char * addr)
{
	int i;
	unsigned char env_enetaddr[6];
	char *tmp = getenv ("ethaddr");
	char *end;

	for (i=0; i<6; i++) {
//		env_enetaddr[i] = tmp ? simple_strtoul(tmp, &end, 16) : 0;
		if (tmp)
			tmp = (*end) ? end+1 : end;
	}

	/* verify chip id */
	if (get_reg_init_bus (PP_ChipID) != 0x630e)
		return;
	eth_reset ();
	if ((get_reg (PP_SelfST) & (PP_SelfSTAT_EEPROM | PP_SelfSTAT_EEPROM_OK)) ==
			(PP_SelfSTAT_EEPROM | PP_SelfSTAT_EEPROM_OK)) {

		/* Load the MAC from EEPROM */
		for (i = 0; i < 6 / 2; i++) {
			unsigned int Addr;

			Addr = get_reg (PP_IA + i * 2);
			addr[i * 2] = Addr & 0xFF;
			addr[i * 2 + 1] = Addr >> 8;
		}

		if (memcmp(env_enetaddr, "\0\0\0\0\0\0", 6) != 0 &&
		    memcmp(env_enetaddr, addr, 6) != 0) {
			printf ("\nWarning: MAC addresses don't match:\n");
			printf ("\tHW MAC address:  "
				"%02X:%02X:%02X:%02X:%02X:%02X\n",
				addr[0], addr[1],
				addr[2], addr[3],
				addr[4], addr[5] );
			printf ("\t\"ethaddr\" value: "
				"%02X:%02X:%02X:%02X:%02X:%02X\n",
				env_enetaddr[0], env_enetaddr[1],
				env_enetaddr[2], env_enetaddr[3],
				env_enetaddr[4], env_enetaddr[5]) ;
//				debug ("### Set MAC addr from environment\n");
			memcpy (addr, env_enetaddr, 6);
		}
		if (!tmp) {
			char ethaddr[20];
			sprintf (ethaddr, "%02X:%02X:%02X:%02X:%02X:%02X",
				 addr[0], addr[1],
				 addr[2], addr[3],
				 addr[4], addr[5]) ;
//			debug ("### Set environment from HW MAC addr = \"%s\"\n",				ethaddr);
//			setenv ("ethaddr", ethaddr);
		}

	}
}

static void jz_eth_halt (struct eth_device *dev)
{
	/* disable transmitter/receiver mode */
	put_reg (PP_LineCTL, 0);

	/* "shutdown" to show ChipID or kernel wouldn't find he cs8900 ... */
	get_reg_init_bus (PP_ChipID);
}

static int low_level_init(struct netif *netif)
{
	u16 id,status;
	int i;
#define bi_enetaddr netif->hwaddr

	/* verify chip id */
	id = get_reg_init_bus (PP_ChipID);
	if (id != 0x630e) {
		printf ("CS8900 jz_eth_init error!\n");
		return 0;
	}

	eth_reset ();
	/* set the ethernet address */
	put_reg (PP_IA + 0, bi_enetaddr[0] | (bi_enetaddr[1] << 8));
	put_reg (PP_IA + 2, bi_enetaddr[2] | (bi_enetaddr[3] << 8));
	put_reg (PP_IA + 4, bi_enetaddr[4] | (bi_enetaddr[5] << 8));

	eth_reginit();

	return 0;
}

/* Get a data block via Ethernet */
static struct pbuf *ETH_Input(struct netif *netif)
{
	struct pbuf *p = NULL, *q = NULL;
	u16 *ptr;
        int i;
	unsigned short rxlen;
	unsigned short *addr;
	unsigned short status;
	printf("input\n");
	for(i=0;i<10;i++)
	{
		udelay(1000);
	}

	status = CS8900_RTDATA;  /* stat */
	rxlen = CS8900_RTDATA;   /* len */

	if (!(status & RxOK)) {
		printf("error input\n");
	}

	if (rxlen > 0)
	{
		p = pbuf_alloc(PBUF_RAW, rxlen, PBUF_POOL);  //length - 4
		{
			if (p != 0)
			{
				for (q = p; q != 0; q = q->next) 
				{ 
					ptr = q->payload; 
					// TODO: CHECK: what if q->len is odd? we don't use the last byte? 
					for (i = 0; i < (q->len + 1)/ 2; i++) 
					{ 
						*ptr = CS8900_RTDATA; 
						ptr++; 
					} 
				} 
			
			}
			else
			{
				put_reg(PP_RxCFG,(0x0003U|RxOKiE|0x0040U));
				rxlen = 0;
			}
		}
	}
	return p;
}

/* Send a data block via Ethernet. */
static err_t ETH_output(struct netif *netif, struct pbuf *p)
{
	struct pbuf *q;
	u16 *ptr =(u16 *)p->payload; 
	u16_t length = p->tot_len; 
	unsigned long sent_bytes; 
	u16 status;
	volatile unsigned short *addr;
	int tmo,i;
	unsigned short s;
	int tries = 0; 
	printf("output\n");
	for(i=0;i<200;i++)
	{
		udelay(1000);
	}
	/* initiate a transmit sequence */
	put_reg(PP_TxCMD,0x00C9U);
	put_reg(PP_TxLength,(p->tot_len < MIN_PACKET_SIZE ? MIN_PACKET_SIZE: p->tot_len));
	
	status = get_reg (PP_BusST);
	if ((status & TxBidErr)) {
		printf ("Invalid frame size %d!\n",status);
		return;
	}

        /* not ready for transmission and still within 50 retries? */
	while (((get_reg(CS8900_PDATA) & 0x0100U/*Rdy4TxNOW*/) == 0) && (tries++ < 50)) 
	{ 
		/* throw away the last committed received frame */
		put_reg(PP_RxCFG,(0x0003U|RxOKiE|0x0040U));
	} 

	/* Write the contents of the packet */
	/* assume even number of bytes */
	if (status & 0x0100U /*PP_BusSTAT_TxRDY*/) 
	{
		sent_bytes=0;
		for (q = p; q != NULL; q = q->next) 
		{ 
			for (i = 0; i < q->len; i += 2) 
			{ 
				CS8900_RTDATA= *ptr;
				ptr++; 
				sent_bytes += 2; 
			} 

#if (ETHIF_STATS > 0)
			((struct ETHIF *)netif->state)->sentbytes += q->len;
#endif
			
#if (ETHIF_STATS > 0)
			((struct ETHIF *)netif->state)->sentpackets++;
#endif
		} 
		
		while (sent_bytes < MIN_PACKET_SIZE) 
		{ 
			CS8900_RTDATA=0x0000; 
			sent_bytes += 2; 
		} 
	}
	return ERR_OK;
}

static void cs8900_e2prom_ready(void)
{
	while(get_reg(PP_SelfST) & SI_BUSY);
}

/***********************************************************/
/* read a 16-bit word out of the EEPROM                    */
/***********************************************************/

static int cs8900_e2prom_read(unsigned char addr, unsigned short *value)
{
	cs8900_e2prom_ready();
	put_reg(PP_EECMD, EEPROM_READ_CMD | addr);
	cs8900_e2prom_ready();
	*value = get_reg(PP_EEData);

	return 0;
}


/***********************************************************/
/* write a 16-bit word into the EEPROM                     */
/***********************************************************/

static int cs8900_e2prom_write(unsigned char addr, unsigned short value)
{
	cs8900_e2prom_ready();
	put_reg(PP_EECMD, EEPROM_WRITE_EN);
	cs8900_e2prom_ready();
	put_reg(PP_EEData, value);
	put_reg(PP_EECMD, EEPROM_WRITE_CMD | addr);
	cs8900_e2prom_ready();
	put_reg(PP_EECMD, EEPROM_WRITE_DIS);
	cs8900_e2prom_ready();

	return 0;
}

/**
* Writing an IP packet (to be transmitted) to the ethif of jz4730.
*
* Before writing a frame to the ethif of jz4730, the ARP module is asked to resolve the
* Ethernet MAC address. The ARP module might undertake actions to resolve the
* address first, and queue this packet for later transmission.
*
* @param netif The lwIP network interface data structure belonging to this device.
* @param p pbuf to be transmitted (or the first pbuf of a chained list of pbufs).
* @param ipaddr destination IP address.
*/
err_t ETHIF_output(struct netif *netif, struct pbuf *p, struct ip_addr *ipaddr)
{
  struct cs8900if *cs8900if = netif->state;
  err_t result;
  /* resolve the link destination hardware address */
  p = etharp_output(netif, ipaddr, p);
  /* network hardware address obtained? */
  if (p != NULL)
  {
    /* send out the packet */
    result = ETH_output(netif, p);
    p = NULL;
  }
  /* { p == NULL } */
  else
  {
    /* we cannot tell if the packet was sent, the packet could have been queued */
    /* on an ARP entry that was already pending. */
    return ERR_OK;
  }
  return result;
}
#if 0
static err_t ETHIF_output(struct netif *netif, struct pbuf *p, struct ip_addr *ipaddr)
{
 
    /* resolve hardware address, then send (or queue) packet */
    return etharp_output(netif, ipaddr, p);

}
#endif
/*
* Read a received packet from the jz4730.
*
* This function should be called when a packet is received by the  jz4730
* and is fully available to read. It moves the received packet to a pbuf
* which is forwarded to the IP network layer or ARP module. It transmits
* a resulting ARP reply or queued packet.
*
* @param netif The lwIP network interface to read from.
*
*/
static void ETHIF_input(struct netif *netif)
{
    struct ETHIF *ETHIF = netif->state;
    struct eth_hdr *ethhdr = NULL;
    struct pbuf *p= NULL,*q = NULL;

        p = ETH_Input(netif);
    
	/* no packet could be read */
	if(p == NULL) {
	    /* silently ignore this */
	    return;
	}

	/* points to packet payload, which starts with an Ethernet header */
	ethhdr = p->payload;

#ifdef Print_Header
	printf("#ETH packet src addr: %x.%x.%x.%x.%x.%x \n",ethhdr->src.addr[0],ethhdr->src.addr[1],ethhdr->src.addr[2],ethhdr->src.addr[3],ethhdr->src.addr[4],ethhdr->src.addr[5]);
	printf("#ETH packet dest addr: %x.%x.%x.%x.%x.%x \n",ethhdr->dest.addr[0],ethhdr->dest.addr[1],ethhdr->dest.addr[2],ethhdr->dest.addr[3],ethhdr->dest.addr[4],ethhdr->dest.addr[5]);
	printf("#ETH packet type: %x \n",htons(ethhdr->type));
#endif

	struct etharp_hdr *arphdr = p->payload;
	q = NULL;
	switch(htons(ethhdr->type)) {
	case ETHTYPE_IP: 	/* IP packet? */
	    etharp_ip_input(netif, p); /* update ARP table, obtain first queued packet */
	    pbuf_header(p, -14); /* skip Ethernet header */
	    netif->input(p, netif); /* pass to network layer */
	    break;
		
	case ETHTYPE_ARP:	/* ARP packet? */
#ifdef Print_Header
        printf("#ARP packet proto: %x. hwtype %x \n",htons(arphdr->proto),arphdr->hwtype);
        printf("#ARP packet src ip: %lu. %lu \n",arphdr->sipaddr.addrw[0],arphdr->sipaddr.addrw[1]);
        printf("#ARP packet dest ip: %lu. %lu \n",arphdr->dipaddr.addrw[0],arphdr->dipaddr.addrw[1]);
	printf("#ARP packet src hwaddr: %x.%x.%x.%x.%x.%x \n",arphdr->shwaddr.addr[0],arphdr->shwaddr.addr[1],arphdr->shwaddr.addr[2],arphdr->shwaddr.addr[3],arphdr->shwaddr.addr[4],arphdr->shwaddr.addr[5]);
	printf("#ARP packet dest hwaddr: %x.%x.%x.%x.%x.%x \n",arphdr->dhwaddr.addr[0],arphdr->dhwaddr.addr[1],arphdr->dhwaddr.addr[2],arphdr->dhwaddr.addr[3],arphdr->dhwaddr.addr[4],arphdr->dhwaddr.addr[5]);
#endif
	  etharp_arp_input(netif, (struct eth_addr *)&netif->hwaddr, p);

//	etharp_arp_input(netif, ((struct ETHIF *)netif->state)->ethaddr, p);
	break;
		
	default:			/* unsupported Ethernet packet type */
	    pbuf_free(p);
	    p=NULL;
	    break;
	}
  /* send out the ARP reply or ARP queued packet */
  if (q != NULL) {
	  /* q pbuf has been succesfully sent? */
	  if (ETH_output(netif, q) == ERR_OK)
	  {
		  pbuf_free(q);
		  q = NULL;
	  }
	  else
	  {
		  /* TODO: re-queue packet in the ARP cache here (?) */
		  pbuf_free(q);
		  q = NULL;
	  }
  }
}

static void
arp_timer(void *arg)
{
    etharp_tmr();
    sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
}

static void ETH_Handler(unsigned int arg)
{
	u16 status;
	__gpio_mask_irq(IRQ_ETH);
	while(status = get_reg(PP_ISQ))
	{
		switch (RegNum (status)) {
		case RxEvent:
			ETHIF_input((struct netif *)arg);
			break;

		case TxEvent:
			printf("TxEvent\n");
			break;

		case BufEvent:
			printf("BufEvent\n");
			break;
			
		case TxCOL:
			printf("TxCOL\n");
			break;

		case RxMISS:
			printf("RxMISS\n");
			break;
		default: break;
		}
	}
	__gpio_unmask_irq(IRQ_ETH);

}

err_t
ETHIF_Init(struct netif *netif)
{
	struct ETHIF *ETHIF;
	u32 reg;
#define RD_N_PIN (32 + 29)
#define WE_N_PIN (32 + 30)
#define CS4_PIN (32 + 28)
	__gpio_as_func0(CS4_PIN);
	__gpio_as_func0(RD_N_PIN);
	__gpio_as_func0(WE_N_PIN);

//	REG_EMC_SMCR4 |= (1 << 6);       //16bit

	reg = REG_EMC_SMCR4;
	reg = (reg & (~EMC_SMCR_BW_MASK)) | EMC_SMCR_BW_16BIT;
	REG_EMC_SMCR4 = reg;
	JZ_StartTimer();	

	ETHIF = mem_malloc(sizeof(struct ETHIF));
	if (ETHIF == NULL) return ERR_MEM;
	
	netif->name[0] = IFNAME0;
	netif->name[1] = IFNAME1;
	netif->output = ETHIF_output;
	netif->linkoutput = ETH_output;
	
    // initialize ETH specific interface structure
	netif->state = ETHIF;
	
    /* maximum transfer unit */
	netif->mtu = 1500;
	
    /* broadcast capability */
	netif->flags = NETIF_FLAG_BROADCAST;
	
	/* hardware address length */
	netif->hwaddr_len = 6;
//	int i;
//	for(i=0; i < netif->hwaddr_len; i++)
//		netif->hwaddr[i]=0x68;
//	/* mike not needed*/ ETHIF->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
	
	ETHIF->needs_service = 0;
	ETHIF->use_polling = 0;
	
#if (ETHIF_STATS > 0)
	// number of interrupts (vector calls)
	ETHIF->interrupts = 0;
	ETHIF->missed = 0;
	ETHIF->dropped = 0;
	ETHIF->sentpackets = 0;
	ETHIF->sentbytes = 0;
#endif
	/* make up an MAC address. */ 
	netif->hwaddr[0] = (u8_t)ETHADDR0; 
	netif->hwaddr[1] = (u8_t)ETHADDR1; 
	netif->hwaddr[2] = (u8_t)ETHADDR2; 
	netif->hwaddr[3] = (u8_t)ETHADDR3; 
	netif->hwaddr[4] = (u8_t)ETHADDR4; 
	netif->hwaddr[5] = (u8_t)ETHADDR5; 

	
	request_irq(IRQ_GPIO_0 + ETH_GPIO, ETH_Handler,netif);
	__gpio_as_irq_high_level(ETH_GPIO);    //irq
        //__gpio_as_irq_rise_edge(ETH_GPIO);    //irq  
	__gpio_disable_pull(ETH_GPIO);         //disable pull
	REG_EMC_SMCR4 |= (1 << 6); 
        //__gpio_ack_irq(ETH_GPIO);
        //__gpio_mask_irq(ETH_GPIO); 
        low_level_init(netif); 
	sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
	__gpio_unmask_irq(ETH_GPIO);
	return ERR_OK;
}
