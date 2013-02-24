/********************************************
 *
 * File: drivers/ssfdc/jz_nand.c
 * 
 * Description: Actual hardware IO routies for the nand flash device on JZ 
 *
 * History: 27 May 2003 - new
 *          27 Mar 2007 - by Peter & Celion, Ingenic Ltd    
 ********************************************/
#include <bsp.h>
#include <jz4740.h>
#include <header.h>
#include <jz_nand.h>
#define __author__              "Peter, Celion"
#define __description__         "JZ nand I/O driver"
#define __driver_name__         "jz_nand"
#define __license__             "GPL"

//#define JZ_DEBUG
//#define JZ_HW_TEST

/* global mutex between yaffs2 and ssfdc */
#define DMA_ENABLE 1
#define DMA_INTERRUPT_ENABLE 1

#if DMA_ENABLE
#define DMA_CHANNEL 4
#define PHYSADDR(x) ((x) & 0x1fffffff)
#endif



void rs_correct(unsigned char *buf, int idx, int mask);

#define NAND_INTERRUPT 1
#if NAND_INTERRUPT
#include "ucos_ii.h"

static OS_EVENT * rb_sem = NULL;
static OS_EVENT * dma_sem = NULL;

#define RB_GPIO_GROUP 2
#define RB_GPIO_BIT   30
#define RB_GPIO_PIN (32 * RB_GPIO_GROUP + RB_GPIO_BIT)  
#define NAND_CLEAR_RB() (REG_GPIO_PXFLGC(RB_GPIO_GROUP) = (1 << RB_GPIO_BIT))
#define nand_wait_ready()                     \
do{                                           \
    unsigned short timeout = (1000 * 1000 / OS_TICKS_PER_SEC);             \
	while((!(REG_GPIO_PXFLG((RB_GPIO_GROUP) & (1 << RB_GPIO_BIT))))& timeout--);\
    REG_GPIO_PXFLGC(RB_GPIO_GROUP) = (1 << RB_GPIO_BIT); \
}while(0)

#define READDATA_TIMEOUT (100 * 1000 / OS_TICKS_PER_SEC)
#define WRITEDATA_TIMEOUT (800 * 1000 / OS_TICKS_PER_SEC)
#define PAGE_TIMEOUT (100 * 1000 / OS_TICKS_PER_SEC)
#else
static inline void nand_wait_ready(void)
{
  unsigned int timeout = 100;
  while ((REG_GPIO_PXPIN(2) & 0x40000000) && timeout--);
  while (!(REG_GPIO_PXPIN(2) & 0x40000000));
}
#endif
static unsigned long NANDFLASH_BASE = 0xB8000000;

#define REG_NAND_DATA  (*((volatile unsigned char *) NANDFLASH_BASE))
#define REG_NAND_CMD   (*((volatile unsigned char *) (NANDFLASH_BASE + NANDFLASH_CLE)))
#define REG_NAND_ADDR  (*((volatile unsigned char *) (NANDFLASH_BASE + NANDFLASH_ALE)))

#define MULTI_TASK_SUPPORT 1
#if MULTI_TASK_SUPPORT
static  OS_EVENT  *SemDeviceOps;
#define OP_NAND_LOCK() do{ \
													unsigned char  err; \
													OSSemPend(SemDeviceOps, 0, &err); \
												}while(0)
 #define OP_NAND_UNLOCK() do{ \
													unsigned char  err; \
													OSSemPost(SemDeviceOps); \
												}while(0)
#else
#define OP_NAND_LOCK()
#define OP_NAND_UNLOCK()
#endif

static inline int nand_send_readaddr(unsigned int pageaddr,unsigned int offset)
{
    unsigned char err;
//	printf("pageaddr = 0x%x %d\r\n",pageaddr,offset);
	
	REG_NAND_CMD = 0x00;
	
	REG_NAND_ADDR = (unsigned char)((offset & 0x000000FF) >> 0);
	REG_NAND_ADDR = (unsigned char)((offset & 0x0000FF00) >> 8);
	
	REG_NAND_ADDR = (unsigned char)((pageaddr & 0x000000FF) >> 0);
	REG_NAND_ADDR = (unsigned char)((pageaddr & 0x0000FF00) >> 8);
	if(CONFIG_SSFDC_NAND_ROW_CYCLE >= 3 )
		REG_NAND_ADDR = (unsigned char)((pageaddr & 0x00FF0000) >> 16);
#if NAND_INTERRUPT	
	NAND_CLEAR_RB();
	__gpio_unmask_irq(RB_GPIO_PIN);
#endif

    REG_NAND_CMD = 0x30;
#if NAND_INTERRUPT	
	OSSemPend(rb_sem,READDATA_TIMEOUT,&err);
	if(err)
	{
		printf("timeout\n");
        printf("pageaddr=%d, offset=%d\n", pageaddr, offset);
		
		return -1;
		
		//	goto write_error;		
	}
#else
	nand_wait_ready();
#endif
	
	return 0;
	
//	nand_wait_ready();
	
}

static void inline nand_send_readcacheaddr(unsigned short offset)
{
    REG_NAND_CMD = 0x05;
	REG_NAND_ADDR = (unsigned char)((offset & 0x000000FF) >> 0);
	REG_NAND_ADDR = (unsigned char)((offset & 0x0000FF00) >> 8);
	REG_NAND_CMD = 0xe0;
}


#if DMA_ENABLE
static inline void nand_write_memcpy(void *target,void *source,unsigned int len)
{
    int ch = DMA_CHANNEL;
    unsigned char err;
	//printf("target = 0x%08x source = 0x%08x\r\n",target,source);
	
	if(((unsigned int)source < 0xa0000000) && len)
		 dma_cache_wback_inv(((unsigned long)source - 31) / 32 * 32, len + 64);
	
	if(((unsigned int)target < 0xa0000000) && len)
		dma_cache_wback_inv(((unsigned long)target - 31) / 32 * 32, len + 64);
	

	REG_DMAC_DSAR(ch) = PHYSADDR((unsigned long)source);       
	REG_DMAC_DTAR(ch) = PHYSADDR((unsigned long)target);       
	REG_DMAC_DTCR(ch) = len / 4;	            	    
	REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_AUTO;	        	
	#if DMA_INTERRUPT_ENABLE
		REG_DMAC_DCMD(ch) = DMAC_DCMD_SAI| DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_8|DMAC_DCMD_DS_32BIT | DMAC_DCMD_TIE;
	#else
		REG_DMAC_DCMD(ch) = DMAC_DCMD_SAI| DMAC_DCMD_SWDH_32 
												| DMAC_DCMD_DWDH_8|DMAC_DCMD_DS_32BIT;
	#endif
	REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
    #if DMA_INTERRUPT_ENABLE
		OSSemPend(dma_sem, 0, &err);
	#else
	  while (REG_DMAC_DTCR(ch));
	#endif
}
static inline void nand_read_memcpy(void *target,void *source,unsigned int len)
{
    int ch = DMA_CHANNEL;
    unsigned char err;
//	printf("target = 0x%08x source = 0x%08x\r\n",target,source);
	
	if(((unsigned int)source < 0xa0000000) && len)
	{
       
		dma_cache_wback_inv(((unsigned long)source - 31) / 32 * 32, len + 64);
		
		
	}
	
	if(((unsigned int)target < 0xa0000000) && len)
	{
		dma_cache_wback_inv(((unsigned long)target - 31) / 32 * 32, len + 64);
		
    }
	
	REG_DMAC_DSAR(ch) = PHYSADDR((unsigned long)source);       
	REG_DMAC_DTAR(ch) = PHYSADDR((unsigned long)target);       
	REG_DMAC_DTCR(ch) = len / 4;	            	    
	REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_AUTO;	        	
	
#if DMA_INTERRUPT_ENABLE
			REG_DMAC_DCMD(ch) = DMAC_DCMD_DAI | DMAC_DCMD_SWDH_8 |
												  DMAC_DCMD_DWDH_32|DMAC_DCMD_DS_32BIT| DMAC_DCMD_TIE;
  #else
  		REG_DMAC_DCMD(ch) = DMAC_DCMD_DAI | DMAC_DCMD_SWDH_8 |
												  DMAC_DCMD_DWDH_32|DMAC_DCMD_DS_32BIT;
  #endif
	REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;	
#if DMA_INTERRUPT_ENABLE
	 	OSSemPend(dma_sem, 0, &err);
	#else
		while (REG_DMAC_DTCR(ch));
	#endif
	
}
#endif
static inline int nand_read_oob(int page,nand_page_info_t *info)
{
   	unsigned char *ptr;
    unsigned int i;
  if(nand_send_readaddr(page,CONFIG_SSFDC_NAND_PAGE_SIZE))
    	    return -1;
	ptr = (unsigned char *)info;	
  	for ( i = 0; i < NAND_OOB_SIZE; i++) 
  	{
  		unsigned char d;
  		d =NAND_IO_ADDR;
    	*ptr++ = d;
    }
    return 0;
	
}
static int nand_rs_correct(unsigned char *tmpbuf)
{
	unsigned int stat = REG_EMC_NFINTS;
	//printf("stat = 0x%x\n", stat);
	if (stat & EMC_NFINTS_ERR) {
		if (stat & EMC_NFINTS_UNCOR) {
			printf("Uncorrectable error occurred\n");
			printf("stat = 0x%x\n", stat);
			return -EIO;
		}
		else {
			printf("Correctable error occurred\n");
			
			unsigned int errcnt = (stat & EMC_NFINTS_ERRCNT_MASK) >> EMC_NFINTS_ERRCNT_BIT;
//Note:rs correct one bit each time,so take away the first three break -by dsqiu
			printf("stat = 0x%x error count = %d\n", stat,errcnt);
			switch (errcnt) {
			case 4:
				rs_correct(tmpbuf, (REG_EMC_NFERR3 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR3 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);			
			case 3:
				rs_correct(tmpbuf, (REG_EMC_NFERR2 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR2 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);				
			case 2:
				rs_correct(tmpbuf, (REG_EMC_NFERR1 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR1 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
			case 1:
				rs_correct(tmpbuf, (REG_EMC_NFERR0 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR0 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				break;
			default:
				
				break;
			}
		}
		
	}
	return 0;	
}

#if 0
int jz_nand_scan_id (void *dev_id, nand_chip_info_t *info)
{
#ifdef JZ_DEBUG
	printf("%s()\n", __FUNCTION__);
#endif

	JZ_NAND_SELECT;
    REG_NAND_CMD = NAND_CMD_READ_ID1;
	REG_NAND_ADDR = NAND_CMD_READ1_00;
	info->make = NAND_IO_ADDR;
	info->device = NAND_IO_ADDR;
	JZ_NAND_DESELECT;
	

	return 0;
}
#endif
int jz_nand_read_page_info (void *dev_id, int page, nand_page_info_t *info)
{
	int ret;
	OP_NAND_LOCK();
#ifdef JZ_DEBUG    
	printf("%s() page: 0x%x\n", __FUNCTION__, page);
#endif

	JZ_NAND_SELECT;
	ret = nand_read_oob(page,info);
	JZ_NAND_DESELECT;
	OP_NAND_UNLOCK();
	return ret;
}
int jz_nand_read_page (void *dev_id, int page, unsigned char *data,nand_page_info_t *info)
{
	int i,j,ret = 0;
	
	unsigned char *ptr;
  volatile unsigned char *paraddr = (volatile unsigned char*)EMC_NFPAR0;
	unsigned char *par;
	unsigned char *tmpbuf;
	unsigned int stat;
	
#ifdef JZ_DEBUG
	printf("%s() page: 0x%x\n", __FUNCTION__, page);
#endif
  //printf("%s() page: 0x%x\n", __FUNCTION__, page);
	OP_NAND_LOCK();
	
	JZ_NAND_SELECT;
   
    if(nand_read_oob(page,info))
    {
			JZ_NAND_DESELECT;
			OP_NAND_UNLOCK();
			return -EIO;    
    }
 
	//nand_send_readcacheaddr(0);
	unsigned int pagecontinue = -1;
	ptr = (unsigned char *) data;
	par = (unsigned char *)&info->ecc_field;
	
	for(j = 0; (j < JZSOC_ECC_STEPS) && (ret == 0); j++){
		
		if(((dev_id == 0)&&((info->page_status & (1 << j)) == 0)) ||
			 ((dev_id !=0) && (
			info->ecc_field[j*9 + 0] & info->ecc_field[j*9 + 1] & info->ecc_field[j*9 + 2] &
			info->ecc_field[j*9 + 3] & info->ecc_field[j*9 + 4] & info->ecc_field[j*9 + 5] &
			info->ecc_field[j*9 + 6] & info->ecc_field[j*9 + 7] & info->ecc_field[j*9 + 8]) != 0xff
			))
		{
				if(j != pagecontinue)
					    nand_send_readcacheaddr(j*JZSOC_ECC_BLOCK);
				pagecontinue++;	    
			tmpbuf = ptr;
			REG_EMC_NFINTS = 0x0;
			__nand_ecc_rs_decoding();
				
	#if DMA_ENABLE
			nand_read_memcpy(ptr,(void *)&NAND_IO_ADDR,JZSOC_ECC_BLOCK);
			ptr += JZSOC_ECC_BLOCK;
			
	#else		
			for(i = 0; i < JZSOC_ECC_BLOCK; i++)
			{
				unsigned int dat;
				dat = NAND_IO_ADDR;
				*ptr++ = dat;
			
			}
			
	#endif
			//printf("\r\nread par:\r\n");
			 volatile unsigned char *paraddr = EMC_NFPAR0;
			for(i = 0; i < PAR_SIZE; i++)
			{
				//printf("0x%02x ",*par);
				*paraddr++ = *par++;
			}
			REG_EMC_NFECR |= EMC_NFECR_PRDY;
			__nand_ecc_decode_sync();
		  __nand_ecc_disable();
			
				  	  
				ret =  nand_rs_correct(tmpbuf);
				if(ret == -EIO)
				{
					printf("%s page = 0x%x,sector = %d,%x\n",__FUNCTION__,page,j,info->page_status);
				  #if 0
				  ptr -= JZSOC_ECC_BLOCK;
				  
				  unsigned int i1,i2;
				  printf("data:\n");
				  for(i2 = 0; i2 < JZSOC_ECC_BLOCK / 16;i2++)
				  {
				  	for(i1 = 0; i1 < 16;i1++)
				    {
				    	printf("%2x ",*tmpbuf++);
				    }
				    printf("\n");
				  }
				  printf("ecc:\n");
				  par -= PAR_SIZE;
				  for(i = 0; i < PAR_SIZE; i++)
					{
				     printf("0x%02x ",*par++);
			    }
			    printf("\n");	
			    #endif
				}
		}else
		{
	  	  par += PAR_SIZE;
	  	  ptr += JZSOC_ECC_BLOCK;
		}	
	}
	JZ_NAND_DESELECT;
	
	OP_NAND_UNLOCK();
	return ret;
}
int jz_nand_multiread(void *dev_id, unsigned int page,unsigned int pagecount,
					  unsigned char *mask,unsigned char *data,nand_page_info_t *info)
{
  int i,j,ret = 0;
	
 unsigned char *ptr;
  volatile unsigned char *paraddr = (volatile unsigned char*)EMC_NFPAR0;
  unsigned char *par;
  unsigned char *tmpbuf;
  unsigned int stat;
  unsigned int continue_page;
#define NAND_PAGE_SECT 4
#define NAND_PAGE_MASK ((1 << NAND_PAGE_SECT) - 1)
	
#ifdef JZ_DEBUG
  printf("%s() page: 0x%x\n", __FUNCTION__, page);
#endif
 //printf("+++%s()\n", __FUNCTION__);   
  OP_NAND_LOCK();
  JZ_NAND_SELECT;
  ptr = (unsigned char *) data;
  
  while((pagecount--) && (ret == 0))
  {
	  //printf("%s() page: 0x%x 0x%x\n", __FUNCTION__, page,*mask);
	  if(*mask != NAND_PAGE_MASK)
	  {
		  
		  if(nand_read_oob(page,info))
		  {
			  JZ_NAND_DESELECT;
			  printf("%s page 0x%x oob error!",__FUNCTION__,page);
			  return -EIO;    
		  }
		  //printf("page = 0x%x %x\n",page,info->page_status);
		  
		  continue_page = -1;
		  
		  if(info->page_status == 0xff)
		  {
			  //printf("data not found page %x\n",page);
			  //nand_memset(ptr,0xff,JZSOC_ECC_BLOCK * JZSOC_ECC_STEPS);
			  ptr += JZSOC_ECC_BLOCK * JZSOC_ECC_STEPS;
			  //continue;
		  }else{
   		  
			  
			  par = (unsigned char *)&(info->ecc_field);
			  for(j = 0; (j < JZSOC_ECC_STEPS) && (ret == 0); j++){
				  if(((*mask & (1 << j)) == 0)&&((info->page_status & (1<<j)) == 0)){
							tmpbuf = ptr;
							if(j != continue_page)
								nand_send_readcacheaddr(j * JZSOC_ECC_BLOCK);
						
							 continue_page = j;	
							REG_EMC_NFINTS = 0x0;
							__nand_ecc_rs_decoding();
#if DMA_ENABLE
							nand_read_memcpy(ptr,(void *)&NAND_IO_ADDR,JZSOC_ECC_BLOCK);
							ptr += JZSOC_ECC_BLOCK;
							
#else		
							for(i = 0; i < JZSOC_ECC_BLOCK; i++)
							{
							  unsigned int dat;
							  dat = NAND_IO_ADDR;
							  *ptr++ = dat;
							}
#endif
							//printf("\r\nread par:\r\n");
							volatile unsigned char *paraddr = (volatile unsigned char*)EMC_NFPAR0;
							for(i = 0; i < PAR_SIZE; i++)
							{
							  //printf("0x%02x ",*par);
							  *paraddr++ = *par++;
							}
							//	printf("\r\n");
							REG_EMC_NFECR |= EMC_NFECR_PRDY;
							
							__nand_ecc_decode_sync();
							__nand_ecc_disable();
							ret =  nand_rs_correct(tmpbuf);
							if(ret == -EIO){
								  printf("%s page = 0x%x,sector = %d",__FUNCTION__,page,j);								  
							}
							*mask |= (1 << j);
				
		      }else
		      {
					  //printf("read page %x sector %d mask = 0x%x\n",page,j,*mask);
				
					  ptr += JZSOC_ECC_BLOCK;
					  par += PAR_SIZE;
					  
  	      }			
			  }
			  
		  }
	  }else{
		  ptr += JZSOC_ECC_BLOCK * JZSOC_ECC_STEPS;
	  }
	  
	  page++;
	  mask++;
	  info++;
	  
  }
  
  JZ_NAND_DESELECT;
//  printf("---%s()\n", __FUNCTION__);
  OP_NAND_UNLOCK();
  return ret;
}

void rs_correct(unsigned char *buf, int idx, int mask)
{
	int i, j;
	unsigned short d, d1, dm;

	i = (idx * 9) >> 3;
	j = (idx * 9) & 0x7;

	i = (j == 0) ? (i - 1) : i;
	j = (j == 0) ? 7 : (j - 1);
    if(i >= 512)
		return;
	
	d = (buf[i] << 8) | buf[i - 1];

	d1 = (d >> j) & 0x1ff;
	d1 ^= mask;

	dm = ~(0x1ff << j);
	d = (d & dm) | (d1 << j);
    
	buf[i - 1] = d & 0xff;
	buf[i] = (d >> 8) & 0xff;
	
	
}

static void nand_send_writeaddr(unsigned int pageaddr,unsigned int offset)
{
    REG_NAND_CMD = NAND_CMD_PAGE_PROGRAM_START; 
	REG_NAND_ADDR = (unsigned char)((offset >> 0) & 0x0FF);
	REG_NAND_ADDR = (unsigned char)((offset >> 8) & 0x0FF);
	REG_NAND_ADDR = (unsigned char)((pageaddr >> 0) & 0x0FF);
	REG_NAND_ADDR = (unsigned char)((pageaddr >> 8) & 0x0FF);    
	if(CONFIG_SSFDC_NAND_ROW_CYCLE >= 3 )
		REG_NAND_ADDR = (unsigned char)((pageaddr >> 16) & 0x0FF);

}
#define NAND_CMD_PAGE_RANDOM_PROGRAM_START 0x85
static void nand_send_writecacheaddr(unsigned int offset)
{
//	printf("writecacheaddr = 0x%x\n",offset);
	
	REG_NAND_CMD = 0x85; //NAND_CMD_PAGE_RANDOM_PROGRAM_START; 
	REG_NAND_ADDR = (unsigned char)((offset >> 0) & 0x0FF);
	REG_NAND_ADDR = (unsigned char)((offset >> 8) & 0x0FF);	
}
int jz_nand_write_page (void *dev_id, int page, unsigned char *data, nand_page_info_t *info)
{
	int i;
	unsigned char *ptr, status;
	
#ifdef JZ_DEBUG
	printf("!!!!!!!gt!!! %s() page: 0x%x 0x%x 0x%x\n", __FUNCTION__, page, *data, *(data+1));
#endif
	printf("!!!!!!!gt!!! %s() page: 0x%x 0x%08x 0x%x\n", __FUNCTION__, page, data, *(unsigned int *)data );
	OP_NAND_LOCK();
	JZ_NAND_SELECT;
	nand_send_writeaddr(page,0);
	ptr = (unsigned char *) data;

	int j;
	unsigned char *par = (unsigned char *)&(info->ecc_field);

	for(j = 0;j<JZSOC_ECC_STEPS; j++){
		REG_EMC_NFINTS = 0;
		__nand_ecc_rs_encoding();
#if DMA_ENABLE
        nand_write_memcpy((void *)&NAND_IO_ADDR,ptr,JZSOC_ECC_BLOCK);
		ptr += JZSOC_ECC_BLOCK;	
#else
		for(i =0; i < JZSOC_ECC_BLOCK; i++)	
			NAND_IO_ADDR = *ptr++;
#endif   
		__nand_ecc_encode_sync();
        
		//	printf("par:\n[");
        volatile unsigned char *paraddr = (unsigned char *)EMC_NFPAR0; 
		for (i = 0; i < PAR_SIZE; i++){
			//		printf(" 0x%x", *paraddr);
			*par++ = *paraddr++;
		}
		//	printf("]\n");
		__nand_ecc_disable();
	}
	ptr = (unsigned char *) info;
	for (i = 0; i < NAND_OOB_SIZE; i++)
		NAND_IO_ADDR = *ptr++;
#if NAND_INTERRUPT	
	NAND_CLEAR_RB();
	__gpio_unmask_irq(RB_GPIO_PIN);
#endif
    REG_NAND_CMD = NAND_CMD_PAGE_PROGRAM_STOP;
#if NAND_INTERRUPT
	unsigned char err;

	OSSemPend(rb_sem,PAGE_TIMEOUT,&err);
	if(err)
	{
		printf("!!!!!!!error!!! %s() page: 0x%x \n", __FUNCTION__, page);
		JZ_NAND_DESELECT;
		goto write_error;
		
	}
#else
	nand_wait_ready();
#endif
	REG_NAND_CMD = NAND_CMD_READ_STATUS;
    i = NAND_OOB_SIZE * JZ_WAIT_STAUTS_DELAY * 1000;
	while((NAND_IO_ADDR & 0x01) &&(i--)) ;
	JZ_NAND_DESELECT;
	if(i > 0)
	{	
		OP_NAND_UNLOCK();
		return 0;
	}
 write_error:
	printf("write page: 0x%x failed\n", page);
	OP_NAND_UNLOCK();
	return -EIO;
}
#if 1


int jz_nand_multiwrite (void *dev_id, unsigned int page, unsigned int pagecount,unsigned char *mask,unsigned char *data, nand_page_info_t *info)
{
	int i,j;
	unsigned char *ptr,*pOOB, status;
	unsigned int continue_page;
	
	
#ifdef JZ_DEBUG
	printf("!!!!!!!gt!!! %s() page: 0x%x 0x%x 0x%x\n", __FUNCTION__, page, *data, *(data+1));
#endif
	
	//printf("!!!!!!!gt!!! %s() page: 0x%x pagecount = 0x%x 0x%08x 0x%x\n", __FUNCTION__, page, pagecount,data, *(unsigned int *)(data));
	
	OP_NAND_LOCK();
	ptr = (unsigned char *) data;
	JZ_NAND_SELECT;
	
	while(pagecount--){
	   //printf("!!!!!!!gt!!! %s() page: 0x%x pagecount = 0x%x 0x%x\n", __FUNCTION__, page, pagecount,*mask);	
		if(*mask != 0){
			unsigned char *par = (unsigned char *)&(info->ecc_field);
			nand_send_writeaddr(page,0);
			continue_page = 0;
			for(j = 0;j<JZSOC_ECC_STEPS; j++){
				//printf("continue %x %x mask = %x\n",j,continue_page,*mask);
				
				if(((*mask) & (1 << j)) != 0){
					if(continue_page != j)
					{
						nand_send_writecacheaddr(JZSOC_ECC_BLOCK * j);
						continue_page = j;
						//printf("nand_send_writecacheaddr = %x\n",JZSOC_ECC_BLOCK * j);
						
					}
					continue_page++;
					
					REG_EMC_NFINTS = 0;
					__nand_ecc_rs_encoding();
                    #if DMA_ENABLE
					nand_write_memcpy((void *)&NAND_IO_ADDR,ptr,JZSOC_ECC_BLOCK);
					ptr += JZSOC_ECC_BLOCK;	
                    #else
					for(i =0; i < JZSOC_ECC_BLOCK; i++)	
						NAND_IO_ADDR = *ptr++;
                    #endif   
					__nand_ecc_encode_sync();
					volatile unsigned char *paraddr = (unsigned char *)EMC_NFPAR0;
					
					for (i = 0; i < PAR_SIZE; i++){
						//		printf(" 0x%x", *paraddr);
						*par++ = *paraddr++;
					}
					__nand_ecc_disable();
				}else
				{
					ptr += JZSOC_ECC_BLOCK;
					par += PAR_SIZE;
				}			  
			}
			if(continue_page != JZSOC_ECC_STEPS)
			{
				nand_send_writecacheaddr(JZSOC_ECC_BLOCK *  JZSOC_ECC_STEPS);
				//printf("nand_send_writecacheaddr2 = %x\n",JZSOC_ECC_BLOCK * JZSOC_ECC_STEPS);
			}
			//printf("page = 0x%x %x\n",page,info->page_status);
			pOOB = (unsigned char *) info;

			for (i = 0; i < NAND_OOB_SIZE; i++)
				NAND_IO_ADDR = *pOOB++;
            #if NAND_INTERRUPT	
			NAND_CLEAR_RB();
			__gpio_unmask_irq(RB_GPIO_PIN);
            #endif
			REG_NAND_CMD = NAND_CMD_PAGE_PROGRAM_STOP;
            #if NAND_INTERRUPT
			unsigned char err;
			OSSemPend(rb_sem,PAGE_TIMEOUT,&err);
			if(err)
			{
				printf("!!!!!!!error!!! %s() page: 0x%x \n", __FUNCTION__, page);
				JZ_NAND_DESELECT;
				goto write_error;
				
			}
            #else
			nand_wait_ready();
            #endif
			REG_NAND_CMD = NAND_CMD_READ_STATUS;
			i = NAND_OOB_SIZE * JZ_WAIT_STAUTS_DELAY * 1000;
			while((NAND_IO_ADDR & 0x01) &&(i--)) ;
            if(i <= 0)
				goto write_error;
			
			
		}else{
			ptr += JZSOC_ECC_BLOCK * JZSOC_ECC_STEPS;
		}
		mask++;
		info++;
		page++;
		
	}
	JZ_NAND_DESELECT;
	OP_NAND_UNLOCK();
	return 0;
	
 write_error:
 	
	JZ_NAND_DESELECT;
	OP_NAND_UNLOCK();
	printf("write page: 0x%x failed\n", page);
	return -EIO;	
}
#endif
int jz_nand_write_page_info (void *dev_id, unsigned int page, nand_page_info_t *info)
{
	int i;
	unsigned char *ptr, status;
	OP_NAND_LOCK();

	JZ_NAND_SELECT;

	nand_send_writeaddr(page,CONFIG_SSFDC_NAND_PAGE_SIZE);	
	ptr = (unsigned char *) info;
	for (i = 0; i < NAND_OOB_SIZE; i++) {
		NAND_IO_ADDR = *ptr++;
	}

	//NAND_CLEAR_RB();
#if NAND_INTERRUPT	
	NAND_CLEAR_RB();
	__gpio_unmask_irq(RB_GPIO_PIN);
#endif
    REG_NAND_CMD = NAND_CMD_PAGE_PROGRAM_STOP;
	
#if NAND_INTERRUPT
	unsigned char err;
	OSSemPend(rb_sem,PAGE_TIMEOUT,&err);
	if(err)
	{
		printf("!!!!!!!error!!! %s() page: 0x%x \n", __FUNCTION__, page);
		JZ_NAND_DESELECT;
		goto write_error;
	}
#else
	nand_wait_ready();
#endif
	REG_NAND_CMD = NAND_CMD_READ_STATUS;

	i = NAND_OOB_SIZE * JZ_WAIT_STAUTS_DELAY * 1000;
	while((NAND_IO_ADDR & 0x01) &&(i--)) ;
	
	JZ_NAND_DESELECT;
	if(i > 0)
	{
		OP_NAND_UNLOCK();
		return 0;
	}
 write_error:
	OP_NAND_UNLOCK();
	printf("write page : 0x%x 's info failed\n", page);
	return -EIO;
}

#define JZ_NAND_ERASE_TIME (30 * 1000)

int jz_nand_erase_block (void *dev_id, unsigned int block)
{
	int i;
	unsigned char status;
	unsigned long addr = block * CONFIG_SSFDC_NAND_PAGE_PER_BLOCK;
#ifdef JZ_DEBUG
	printf("$$$$$$$$$$$ %s() block: 0x%x addr=0x%x\n", __FUNCTION__, block, addr);
#endif
	OP_NAND_LOCK();
	JZ_NAND_SELECT;
	REG_NAND_CMD = NAND_CMD_BLOCK_ERASE_START;
	
	REG_NAND_ADDR = (unsigned char)((addr & 0xFF));
	REG_NAND_ADDR = (unsigned char)((addr >> 8) & 0xFF);
	if(CONFIG_SSFDC_NAND_ROW_CYCLE >= 3 )
		REG_NAND_ADDR = (unsigned char)((addr >> 16) & 0xFF);
	
#if NAND_INTERRUPT	
	NAND_CLEAR_RB();
	__gpio_mask_irq(RB_GPIO_PIN);
#endif
	REG_NAND_CMD = NAND_CMD_BLOCK_ERASE_CONFIRM;

	nand_wait_ready();
	REG_NAND_CMD = NAND_CMD_READ_STATUS;
	i = JZ_NAND_ERASE_TIME / 2 ;
	while((NAND_IO_ADDR & 0x01) && (i--));
	JZ_NAND_DESELECT;
	if(i > 0)
	{
		OP_NAND_UNLOCK();	
		return 1;
	}
 erase_error:
	printf("erase block: 0x%x failed\n", block);
	OP_NAND_UNLOCK();
	return 0;
}
#if NAND_INTERRUPT
static void jz_rb_handler(unsigned int arg)
{
    __gpio_mask_irq(arg);
	OSSemPost(rb_sem);	
}
#endif
#if DMA_INTERRUPT_ENABLE
void jz_nand_dma_handler(unsigned int arg)
{
    REG_DMAC_DCCSR(arg) = 0;
    OSSemPost(dma_sem);
}
#endif

void jz_nand_hw_init (void)
{
    unsigned int dat;
	
	REG_EMC_NFCSR |= EMC_NFCSR_NFE1;// | EMC_NFCSR_NFCE1;
    dat = REG_EMC_SMCR1;
	dat &= ~0x0fffff00;
	//REG_EMC_SMCR1 = 0xfff7700;
	REG_EMC_SMCR1 = dat | 0x0C221200;
	//REG_EMC_SMCR1 = dat | 0xfff7700;
	
	//REG_EMC_SMCR1 = dat | 0x00336000;
	printf("REG_EMC_SMCR1 = 0x%08x\r\n",REG_EMC_SMCR1);
#if DMA_ENABLE
	__cpm_start_dmac();
	__dmac_enable_module();
	#if DMA_INTERRUPT_ENABLE
	if(dma_sem == NULL)
        dma_sem = OSSemCreate(0);
    
        request_irq(IRQ_DMA_0 + DMA_CHANNEL, jz_nand_dma_handler,DMA_CHANNEL);
	#endif	
#endif
#if NAND_INTERRUPT	
	//__gpio_mask_irq(RB_GPIO_PIN);
	
    
	
    NAND_CLEAR_RB();
	__gpio_as_irq_rise_edge(RB_GPIO_PIN);
	__gpio_disable_pull(RB_GPIO_PIN);
    if(rb_sem == NULL)
		rb_sem = OSSemCreate(0);
	request_irq(IRQ_GPIO_0 + RB_GPIO_PIN, jz_rb_handler,RB_GPIO_PIN);
#endif
#if MULTI_TASK_SUPPORT
	SemDeviceOps  = OSSemCreate(1);
#endif

}

int jz_nand_init (void)
{
	int ret;	
	jz_nand_hw_init();
	return 0;
}
int jz_nand_get_info(pflash_info_t pflashinfo)
{

	pflashinfo->dwBytesPerBlock = 128 * 2048;
	pflashinfo->dwSectorsPerBlock = 128;
	pflashinfo->dwDataBytesPerSector = 2048;

	
	pflashinfo->dwFSStartBlock = 512;
	pflashinfo->dwFSTotalBlocks = 4096 - 512;
	pflashinfo->dwFSTotalSectors = 512 * 128;
	pflashinfo->dwMaxBadBlocks = 100;

}