/* Jz ethernet support
 *
 *  Copyright (c) 2005
 *  Ingenic Semiconductor, <lhhuang@ingenic.cn>
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

#include "regs.h"
#include "ops.h"  // for __intc_mask_irq(n)

#include "lwip/debug.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "netif/etharp.h"

#include "jz_eth.h"
#include "ethif_jz4730.h"

/* ********************************************************************* */
/* File local definitions */

/* Define those to better describe your network interface. */

#define IFNAME0 'e'
#define IFNAME1 'n'

/* ********************************************************************* */

#define MAX_WAIT      1000
#define PKTSIZE_ALIGN 1536
#define PKTALIGN      32
#define PKTBUFSRX     16

volatile u8_t  PktBuf[(PKTBUFSRX+1) * PKTSIZE_ALIGN + PKTALIGN];
volatile u8_t *NetTxPacket = 0; 
volatile u8_t *NetRxPackets[PKTBUFSRX];

/* Tx and Rx Descriptor */
typedef struct {
    u32_t status;
    u32_t ctrl;
    u32_t addr;
    u32_t next;
} eth_desc_t;

#define NUM_RX_DESCS PKTBUFSRX
#define NUM_TX_DESCS 4

static eth_desc_t rx_desc[NUM_RX_DESCS];
static eth_desc_t tx_desc[NUM_TX_DESCS];

static int next_rx;
static int next_tx;

static u32_t full_duplex, phy_mode;

/* ********************************************************************* */

static inline void reset_eth(void)
{
    int i;

    jz_writel(ETH_BMR, jz_readl(ETH_BMR) | BMR_SWR);

    for (i = 0; i < MAX_WAIT; i++) {
	if(!(jz_readl(ETH_BMR) & BMR_SWR))
	    break;
	udelay(1);
    }

    if (i == MAX_WAIT)
	printf("Reset eth timeout\n");
}

static inline void enable_eth(void)
{
    jz_writel(ETH_OMR, jz_readl(ETH_OMR) | OMR_ST | OMR_SR);
}
	
static inline void disable_eth(void)
{
    jz_writel(ETH_OMR, jz_readl(ETH_OMR) & ~(OMR_ST | OMR_SR));
}

#define MII_CMD_ADDR(id, offset) (((id) << 11) | ((offset) << 6))
#define MII_CMD_READ(id, offset) (MII_CMD_ADDR(id, offset))
#define MII_CMD_WRITE(id, offset) (MII_CMD_ADDR(id, offset) | 0x2)

static u32_t mii_read(int phy_id, int offset)
{
    int i;
    u32_t mii_cmd = MII_CMD_READ(phy_id, offset);

    jz_writel(ETH_MIAR, mii_cmd);

    /* wait for completion */
    for (i = 0; i < MAX_WAIT; i++) {
	if (!(jz_readl(ETH_MIAR) & 0x1))
	    break;
	udelay(1);
    }
    
    if (i == MAX_WAIT) {
	printf("MII wait timeout\n");
	return 0;
    }
    
    return jz_readl(ETH_MIDR) & 0x0000ffff;
}

static int autonet_complete(int phy_id)
{
    int i;

    for (i = 0; i < MAX_WAIT; i++) {
	if (mii_read(phy_id, MII_SR) & 0x0020) 
	    break;  //auto negotiation completed
	udelay(500);
    }

    if (i == MAX_WAIT)
	return -1;     //auto negotiation  error
    else
	return 0;
}

static int search_phy(int phy_id)
{
    unsigned int r;
    r = mii_read(phy_id, 1);
    if (r!=0 && r!=0xffff)
	return 1;
    return 0;
}

static void config_phy(int phy_id)
{
    u32_t mii_reg5;

    full_duplex = 0;

    mii_reg5 = mii_read(phy_id, MII_ANLPA);

    if (mii_reg5 != 0xffff) {
	mii_reg5 = mii_read(phy_id, MII_ANLPA);
	if ((mii_reg5 & 0x0100) || (mii_reg5 & 0x01C0) == 0x0040)
	    full_duplex = 1;

	phy_mode = mii_reg5 >> 5;

	printf("ETH: setting %s %s-duplex based on MII tranceiver #%d\n",
	       (phy_mode & MII_ANLPA_100M) ? "100Mbps" : "10Mbps",
	       full_duplex ? "full" : "half", phy_id);
    }
}

static void config_mac(struct netif *netif)
{
    u32_t omr, mcr;

    /* Set MAC address */
#define ea netif->hwaddr
    jz_writel(ETH_MALR, (ea[3] << 24) | (ea[2] << 16) | (ea[1] <<  8) | ea[0]);
    jz_writel(ETH_MAHR, (ea[5] <<  8) | ea[4]);

    jz_writel(ETH_HTLR, 0);
    jz_writel(ETH_HTHR, 0);

    /* Assert the MCR_PS bit in CSR */
    if (phy_mode & MII_ANLPA_100M)
	omr = OMR_SF;
    else
	omr = OMR_TTM | OMR_SF;

    mcr = MCR_TE | MCR_RE | MCR_DBF | MCR_LCC; 
    
    if (full_duplex)
	mcr |= MCR_FDX;

    /* Set the Operation Mode (OMR) and Mac Control (MCR) registers */
    jz_writel(ETH_OMR, omr);
    jz_writel(ETH_MCR, mcr);
	
    /* Set the Programmable Burst Length (BMR.PBL, value 1 or 4 is validate) */
    jz_writel(ETH_BMR, DMA_BURST << 8);

    /* Reset csr8 */
    jz_readl(ETH_MFCR); // missed frams counter
}

/*---------------------------------------------------------------------------
 * ETH interface routines
 *--------------------------------------------------------------------------*/


static err_t ETH_output(struct netif *netif, struct pbuf *p)
{
    // q traverses through linked list of pbuf's
    struct pbuf *q;
		
    volatile u8_t *ptr = NetTxPacket; 
    u16_t length = p->tot_len; 

    for(q = p; q != NULL; q = q->next){
	memcpy((void *)ptr,q->payload, q->len); 
	ptr += q->len;
		
#if (ETHIF_STATS > 0)
		((struct ETHIF *)netif->state)->sentbytes += q->len;
#endif
		
#if (ETHIF_STATS > 0)
		((struct ETHIF *)netif->state)->sentpackets++;
#endif
    }
	

    volatile eth_desc_t *desc = 
	(volatile eth_desc_t *)((unsigned int)(tx_desc + next_tx) | 0xa0000000);
    int i;

    /* tx fifo should always be idle */
    desc->addr = virt_to_phys(NetTxPacket);
    desc->ctrl |= TD_LS | TD_FS | length;
    desc->status = T_OWN;

    jz_flush_dcache();
    jz_sync();

    /* Start the tx */
    jz_writel(ETH_TPDR, 1);

    i = 0;
    while (desc->status & T_OWN) {
	if(i > MAX_WAIT) {
	    printf("ETH TX timeout\n");
	    break;
	}
	udelay(1);
	i++;
    }

    /* Clear done bits */
    jz_writel(ETH_SR,  DMA_TX_DEFAULT);
    
    desc->status = 0;
    desc->addr = 0;
    desc->ctrl &= ~(TD_LS | TD_FS);

    next_tx++;
    if (next_tx >= NUM_TX_DESCS){
	next_tx=0;
    }

    return ERR_OK;
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
static err_t ETHIF_output(struct netif *netif, struct pbuf *p, struct ip_addr *ipaddr)
{
 
    /* resolve hardware address, then send (or queue) packet */
    return etharp_output(netif, ipaddr, p);

}


/**
* Move a received packet from the ethif of jz4730 into a new pbuf.
*
* This function copies a frame from the ethif of jz4730.
* It is designed failsafe:
* - It does not assume a frame is actually present.
* - It checks for non-zero length
* - It does not overflow the frame buffer
*/
static struct pbuf *ETH_Input(struct netif *netif)
{
 
    struct pbuf *p = NULL, *q = NULL;
	       
    volatile eth_desc_t *desc;
    int length;
    u32_t status;
    volatile u8_t *ptr = NetRxPackets[next_rx];

    //for(;;) {
    desc = (volatile eth_desc_t *)((unsigned int)(rx_desc + next_rx) | 0xa0000000);
    status = desc->status;

    if (status & R_OWN) {
	return 0;
    }
    
    length = ((status & RD_FL) >> 16); /* with 4-byte CRC value */
    
    if (status & RD_ES) {
	return 0;
    }
    else {
	if (length > 0)
	    {
		// allocate a pbuf chain with total length 'len' 
		p = pbuf_alloc(PBUF_RAW, length - 4, PBUF_POOL);  //length - 4
		if (p != 0) {
		    for (q = p; q != 0; q = q->next) {
			memcpy(q->payload,(void *)ptr, q->len); 
			ptr += q->len;
		    }
		}
		else { 
		    // could not allocate a pbuf
		    // skip received frame
		    // TODO: maybe do not skip the frame at this point in time?
#if (ETHIF_STATS > 0)
		    ((struct ETHIF *)netif->state)->dropped++;
#endif

		    /* ------------------------------------- Add stats >>>>>>>>>>>>>>>>>>>>>>>>>>>> no pbuf */
		    return 0;
		}
	    }
	// length was zero
	else {
	    /* ----------------------------------------- Add stats >>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
	}
	
    }// if(status & RD_E)

    /* Clear done bits */
    jz_writel(ETH_SR,  DMA_RX_DEFAULT| DMA_INT_NI); //DMA_INT_NI must be clear by software!
    desc->status = R_OWN;

    jz_flush_dcache();
    jz_sync();

    next_rx++;
    if (next_rx >= NUM_RX_DESCS) {
	next_rx = 0;
    }
    //} /* for */
    return p;
}


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
    struct pbuf *p;

    while(1) {
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

	etharp_arp_input(netif, ((struct ETHIF *)netif->state)->ethaddr, p);
	break;
		
	default:			/* unsupported Ethernet packet type */
	    pbuf_free(p);
	    break;
	}
    } //while(1)
}


static void
low_level_init(struct netif *netif)
{

    int i, phyid = -1;
	
    /*Setup packet buffers, aligned correctly.    //net.c */
    NetTxPacket = &PktBuf[0] + (PKTALIGN - 1);
    NetTxPacket -= (ulong)NetTxPacket % PKTALIGN;
    for (i = 0; i < PKTBUFSRX; i++) {
	NetRxPackets[i] = NetTxPacket + (i+1)*PKTSIZE_ALIGN;
    }
    
    /* Reset ethernet unit */
    reset_eth();

    /*set bit 16 NI and bit 6 RI */
    jz_writel(ETH_IER, 0x00010040); 

    for (i=0;i<32;i++)
	if (search_phy(i)) {
	    phyid = i;
	    break;
	}

    if (phyid == -1)
	printf("Can't locate any PHY\n");

    /* Start Auto Negotiation of PHY 0 and check it */
    if (autonet_complete(phyid))
	printf("ETH Auto-Negotiation failed\n");

    /* Configure PHY */
    config_phy(phyid);

    /* Configure MAC */
    config_mac(netif);

    /* Setup the Rx&Tx descriptors */
    for (i = 0; i < NUM_RX_DESCS; i++) {
	rx_desc[i].status  = R_OWN;
	rx_desc[i].ctrl    = PKTSIZE_ALIGN | RD_RCH;
	rx_desc[i].addr    = virt_to_phys(NetRxPackets[i]);
	rx_desc[i].next    = virt_to_phys(rx_desc + i + 1);
	
	//printf("rx_desc[i].addr=%lx  NetRxPackets[i]=%lx\n", rx_desc[i].addr,(unsigned long)(NetRxPackets[i]));
	}
    //	rx_desc[NUM_RX_DESCS - 1].next = virt_to_phys(rx_desc); // The last links to the first	
    rx_desc[NUM_RX_DESCS - 1].ctrl |=  RD_RER;
    
    for (i = 0; i < NUM_TX_DESCS; i++) {
	tx_desc[i].status  = 0;
	tx_desc[i].ctrl    = TD_TCH;
	tx_desc[i].addr    = 0;
	tx_desc[i].next    = virt_to_phys(tx_desc + i + 1);
    }
    //	tx_desc[NUM_TX_DESCS - 1].next = virt_to_phys(tx_desc); // The last links to the first
    tx_desc[NUM_TX_DESCS - 1].ctrl |= TD_TER;  // Set the Transmit End Of Ring flag

    jz_flush_dcache();
    
    jz_writel(ETH_RAR, virt_to_phys(rx_desc));
    jz_writel(ETH_TAR, virt_to_phys(tx_desc));

    next_rx = next_tx = 0;

    /* Enable ETH */
    enable_eth();
}

static void
arp_timer(void *arg)
{
    etharp_tmr();
    sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
}

void ETH_Handler(unsigned int arg)
{

    __intc_mask_irq(IRQ_ETH);

    ETHIF_input((struct netif *)arg);

    __intc_unmask_irq(IRQ_ETH);
}


err_t
ETHIF_Init(struct netif *netif)
{
    struct ETHIF *ETHIF;
	
    ETHIF = mem_malloc(sizeof(struct ETHIF));
    if (ETHIF == NULL) return ERR_MEM;
	
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    netif->output = ETHIF_output;
    netif->linkoutput = ETH_output;
	
    // initialize ETH specific interface structure
    netif->state = ETHIF;
	
#if 0
    /* maximum transfer unit */
    netif->mtu = 1500;
	
    /* broadcast capability */
    netif->flags = NETIF_FLAG_BROADCAST;
#endif
	
    /* hardware address length */
    netif->hwaddr_len = 6;
    int i;
    for(i=0; i < netif->hwaddr_len; i++)
	netif->hwaddr[i]=0x68;
    /* mike not needed*/ ETHIF->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

    ETHIF->needs_service = 1;
    ETHIF->use_polling = 0;
	
#if (ETHIF_STATS > 0)
    // number of interrupts (vector calls)
    ETHIF->interrupts = 0;
    ETHIF->missed = 0;
    ETHIF->dropped = 0;
    ETHIF->sentpackets = 0;
    ETHIF->sentbytes = 0;
#endif

    low_level_init(netif);
    request_irq(IRQ_ETH, ETH_Handler,netif);
    sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);

    return ERR_OK;
}
