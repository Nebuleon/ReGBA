

#include "lwip/netif.h"

/* ********************************************************************* */
/* Module configuration */

/* interface statistics gathering
 * such as collisions, dropped packets, missed packets
 * 0 = no statistics, minimal memory requirements, no overhead 
 * 1 = statistics on, but some have large granularity (0x200), very low overhead
 * 2 = statistics on, updated on every call to cs8900_service(), low overhead
 */
#define ETHIF_STATS             2

#define Index_Writeback_Inv_D   0x01             // jz4730/cache.c  asm/cacheops.h
#define CFG_DCACHE_SIZE		16384            // pmp.h
#define CFG_CACHELINE_SIZE	32

/* ********************************************************************* */
/* Interface macro & data definition */

struct ETHIF
{
  struct eth_addr *ethaddr;
  U8 needs_service;
  U8 use_polling;
#if (ETHIF_STATS > 0)
  U32 interrupts; // number of interrupt requests of cs8900
  U32 missed; // #packets on medium that could not enter cs8900a chip due to buffer shortage
  U32 dropped; // #packets dropped after they have been received in chip buffer
  U32 collisions; // #collisions on medium when transmitting packets 
  U32 sentpackets; // #number of sent packets
  U32 sentbytes; // #number of sent bytes
#endif

  /* Add whatever per-interface state that is needed here. */
};


/* ********************************************************************* */
/* Interface function definition */

err_t
ETHIF_Init(struct netif *netif);

void ETH_Handler(unsigned int arg);
/* ********************************************************************* */


//"addrspace.h"  in U-boot
#ifndef __ASSEMBLY__
#define PHYSADDR(a)		(((unsigned long)(a)) & 0x1fffffff)
#else
#define PHYSADDR(a)		((a) & 0x1fffffff)
#endif

// "io.h"  in U-boot
extern inline unsigned long virt_to_phys(volatile void * address)  
{
  return PHYSADDR(address);
}


//"jz47xx.h"  in U-boot
#define cache_unroll(base,op)	        	\
	__asm__ __volatile__("	         	\
		.set noreorder;		        \
		.set mips3;		        \
		cache %1, (%0);	                \
		.set mips0;			\
		.set reorder"			\
		:				\
		: "r" (base),			\
		  "i" (op));



