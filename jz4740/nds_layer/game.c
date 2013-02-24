/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007, 2010 fengGuojin(fgjnew@163.com)
 */ 
/*  
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/cdev.h> 
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/ctype.h>
#include <linux/pagemap.h>
#include <asm-mips/mach-jz4740/regs.h>
#include <asm-mips/mach-jz4740/ops.h>
#include <asm-mips/mach-jz4740/dma.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/soundcard.h>

#include <linux/fb.h>

#include <asm/system.h>
#include <asm/addrspace.h>
#include <asm/jzsoc.h>
*/
#define T()                   //   printf("__________%s; %d; %s;\n",__FILE__,__LINE__,__FUNCTION__)          // TRACE
 
//#define printf(...) 

#if 1
#define malloc alloc
#define free   deAlloc

#define virt_to_phys(vaddr) ((vaddr) & 0x1fffffff)
#define printk printf

#define irqreturn_t
#define IRQ_HANDLED 0

#define DMA_ID_AUTO  5
#define copy_to_user memcpy
#define copy_from_user memcpy
#define dma_cache_wback dma_cache_wback_inv 

#define EFAULT -1
#endif

#define HTW_MP4_NDS 1
#define  AUDIO_PRI_FISRT 1

#define enable_cache 1
#define enable_initfpga 1
#include "game.h"

#include "mipsregs.h"

//MODULE_AUTHOR("game");
//MODULE_LICENSE("Dual BSD/GPL");

#define HTW_NDS_MP4
#define fpga_fifo_size 1024

struct MP4_dev* MP4_devices=NULL;
static unsigned char MP4_inc=0;

static u32 cmd_buf32[2];

static int nds_iqe_len; 
static int nds_iqe_addr;
static u32 cpu_iqe_num; 
static u32 mm_check; 
 u32 kmalloc_ptr;

 u32 get_kmalloc_ptr(){return kmalloc_ptr;}
 u32 kmalloc_ptr_uncache;

struct rtc * pnds_sys_rtc = NULL;
static int dma_chan;
static struct main_buf *pmain_buf;

static struct nds_iqe_list_st nds_iqe_list[0x10];
static int nds_iqe_list_w;
static int nds_iqe_list_r;

const unsigned int mmdata10[]={
0x59a65077,0x969e2070,0x05a236c5,0xe266f727,0x47c42e3d,0x7702e0c7,0x386e4922,0x6fc94d48,0xc2e67712,0x21cd84c0,0x8cc57bd1,0x64c4b77a,0x4cde0879,0xc1fdc29f,0x24282977,0xea172048,
0x89286382,0xc0cdcda6,0x07795ff0,0xb71e7f79,0xde760be6,0x03a71aaa,0x32ffd611,0xd1893538,0x4cdce727,0x5f51cc6b,0x35e84f48,0xd9f41b30,0xe767921f,0x0f93a56b,0xea5acf79,0x2c16c530,
0xc9e5cfbd,0x67f20c2b,0xd538c21f,0x5befe661,0x2502cad0,0xf90b8ed6,0x7dfdcee1,0x79c37c7c,0x5fe48ae2,0x72a9b1e1,0x5495c852,0x133ef240,0x205e87c0,0xfb428f89,0x39570533,0x0cc40eac,
0x1dc471e0,0xf08ad743,0xc7ee7198,0xeb9b526f,0x3f15b6be,0x8b559e96,0xe84b0510,0x14fbf3f7,0x43fa3c6f,0xaf115a3d,0x5e55d8f5,0x209c0a0a,0xf33cd3bc,0x6cb644f4,0xc3ab45a4,0x8bea517f,
0xa843a8c3,0x141c5a4e,0x8cfb2b1c,0xa78fece4,0x7a3c6854,0x9e95687c,0x6e869daa,0x969e5572,0xffb58b23,0xd3c9a0d2,0x26ffc502,0x0df87997,0x629db680,0xc959aa90,0xba7cd136,0x4078db4b,
0x22b53f84,0xa8164dd6,0x7b01795c,0xf6c2a401,0x00819755,0x79046bd5,0xeae74254,0x8e6a2c0d,0x0c2a257e,0x7caaadf8,0xa4aea7e6,0xf58385f6,0x90f2ed1e,0x0e908aba,0xf8bf6e98,0xe0b0ae4a,
0x39b014b1,0x24dc0c3e,0xbed92dfc,0x1b4b97be,0x40afc581,0x02e58f1c,0x222ff046,0xbeb97ff3,0xd2700850,0x8264ec40,0x42ad9401,0xa3ed3f2c,0x54b76f3d,0x1fc41b0a,0x2d42db97,0x88ef3629,
0xe171d4cf,0x25adafb4,0xa50d10d7,0xd9157376,0xa4ef30fd,0x88e55cbf,0x52608d99,0x1bcc2cde,0xee786b99,0xe8f370d6,0x6e1c3361,0xbc020225,0xf4425f68,0x9375f75b,0x5bd624ef,0x7733a892,
0x097619ef,0x64fb17a8,0x70fec00d,0xcf1b6d30,0x064e7068,0xdb3f8e73,0xfdba3ec9,0x7bef0b47,0x701380fa,0xbf5bf403,0xd398ea7f,0xdfe9059f,0x810a0623,0x991b5466,0x121bc87e,0xcb544ed6,
0xd7ca8714,0x5bdb4a42,0xaf7fdc1f,0x5bdf20a3,0xa8eb5af1,0x8f085cc4,0x3b1e3970,0x346e9e96,0x7b1ee130,0x7f047e1e,0x512d3141,0x9f040c6a,0x494e32b6,0xa316fc7e,0xc9f17d2f,0x3be035b3,
0x0a70cc91,0x9766b1c0,0x9adcc874,0x9eb6c5da,0x744eeded,0xab4f081d,0x908b8ba7,0xa5592e32,0xf1df5fb9,0xa87ba103,0xa4828f2b,0x286795fe,0x19c98698,0x8b80d3df,0xc9a253d6,0x900ec28b,
0x4c935faf,0x05b9080c,0x8eab2674,0x08cde155,0xd3f80f48,0x06319a7b,0x1e5d44d6,0xbe33151f,0x734ec23b,0x6cbf5234,0xe6769150,0x1b5d8f69,0x861890f6,0xc01686d1,0xa525b1e7,0xe4b97ea5,
0x7316b2bc,0x7781a657,0x4aeab51d,0xd00e78bc,0xca9f1d48,0x82ab86b1,0x6a60acde,0xb0bce8e6,0xa4a114ca,0xc71ba28f,0xe95229ab,0x38211680,0xe8027b91,0x3e1c3e72,0xd03d1385,0x4b4307d9,
0x3ca1e4af,0x6a146027,0xab80004d,0x37a8d8cc,0x2d256df8,0x525efb06,0xc4c72cf6,0x2ace633c,0xe291721e,0xfd8ade68,0x33e02470,0xa39e1804,0xb6c4e531,0x63f0c555,0x2a8f96d1,0xb9e97c3f,
0xe37f33d4,0xba17c763,0xe9c586ef,0x76493483,0x2bd6c47b,0xd68fdcfe,0x903cb69c,0x4a55827d,0x891d7db1,0x014a2e25,0x66206c73,0x589f55f3,0xc4808b86,0xb0b8013a,0x715435bd,0x7a6a9c0b,
0x3bc6c48d,0xbb6f2402,0x36e52c30,0x39cafd06,0x09ef1596,0x8e8a4e05,0x5a1c8bc6,0xcc58c398,0x8e37603b,0xa0433f6f,0x86c72c14,0xd3f2d9e6,0x00957ebe,0x621a0a62,0xc31c0923,0x6f99005b

};

#ifdef HTW_MP4_NDS
 //static __DECLARE_SEMAPHORE_GENERIC(MP4_keybd_cnt, 0);
 //extern unsigned long volatile __jiffy_data jiffies;
unsigned int nds_updat_tim = 0;
 
#endif
static int MP4_open(struct inode *inode, struct file *filp)
{
#if 0
	struct MP4_dev *dev;
#ifndef HTW_MP4_NDS
	if(MP4_inc>0)return -ERESTARTSYS;
#endif	
	MP4_inc++;
 
	dev = container_of(inode->i_cdev, struct MP4_dev, cdev);
	filp->private_data = dev;
	return 0;
#endif	
}

static int MP4_release(struct inode *inode, struct file *filp)
{
#if 0
	MP4_inc--;
	return 0;
#endif	
}

static ssize_t MP4_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos)
{
	return 0;
}

static ssize_t MP4_write(struct file *filp, const char __user *buf, size_t count,loff_t *f_pos)
{
	return 0;
}
#if  0
#define Index_Invalidate_I      0x00
#define Index_Writeback_Inv_D   0x01
#define Index_Load_Tag_I	0x04
#define Index_Load_Tag_D	0x05
#define Index_Store_Tag_I	0x08
#define Index_Store_Tag_D	0x09
#define Hit_Invalidate_I	0x10
#define Hit_Invalidate_D	0x11
#define Hit_Writeback_Inv_D	0x15
#define Hit_Writeback_I		0x18
#define Hit_Writeback_D		0x19

#define CACHE_SIZE		16*1024
#define CACHE_LINE_SIZE		32
#define KSEG0			0x80000000

#define K0_TO_K1()				\
do {						\
	unsigned long __k0_addr;		\
						\
	__asm__ __volatile__(			\
	"la %0, 1f\n\t"				\
	"or	%0, %0, %1\n\t"			\
	"jr	%0\n\t"				\
	"nop\n\t"				\
	"1: nop\n"				\
	: "=&r"(__k0_addr)			\
	: "r" (0x20000000) );			\
} while(0)

#define K1_TO_K0()				\
do {						\
	unsigned long __k0_addr;		\
	__asm__ __volatile__(			\
	"nop;nop;nop;nop;nop;nop;nop\n\t"	\
	"la %0, 1f\n\t"				\
	"jr	%0\n\t"				\
	"nop\n\t"				\
	"1:	nop\n"				\
	: "=&r" (__k0_addr));			\
} while (0)

#define INVALIDATE_BTB()			\
do {						\
	unsigned long tmp;			\
	__asm__ __volatile__(			\
	".set mips32\n\t"			\
	"mfc0 %0, $16, 7\n\t"			\
	"nop\n\t"				\
	"ori %0, 2\n\t"				\
	"mtc0 %0, $16, 7\n\t"			\
	"nop\n\t"				\
	".set mips2\n\t"			\
	: "=&r" (tmp));				\
} while (0)

#define SYNC_WB() __asm__ __volatile__ ("sync")

#define cache_op(op,addr)						\
	__asm__ __volatile__(						\
	"	.set	noreorder		\n"			\
	"	.set	mips32\n\t		\n"			\
	"	cache	%0, %1			\n"			\
	"	.set	mips0			\n"			\
	"	.set	reorder"					\
	:										\
	: "i" (op), "m" (*(unsigned char *)(addr)))

void __dcache_writeback_all(void)
{
	u32 i;
	for (i=KSEG0;i<KSEG0+CACHE_SIZE;i+=CACHE_LINE_SIZE)
		cache_op(Index_Writeback_Inv_D, i);
	SYNC_WB();
}
void __flush_dcache_line(unsigned long addr)
{
	cache_op(Hit_Writeback_Inv_D, addr);
	SYNC_WB();
}

void dma_cache_wback_inv_my(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (size >= CACHE_SIZE) {
		__dcache_writeback_all();
	}
	else {
		unsigned long dc_lsize = CACHE_LINE_SIZE;

		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		while (1) {
			__flush_dcache_line(a);	/* Hit_Writeback_Inv_D */
			if (a == end)
				break;
			a += dc_lsize;
		}
	}
}
#endif
    /*
static irqreturn_t jz4740_dma_irq1(int irq, void *dev_id)
{
	REG_DMAC_DCCSR(dma_chan) &= ~DMAC_DCCSR_EN;  
	if (__dmac_channel_transmit_halt_detected(dma_chan)) {
		__dmac_channel_clear_transmit_halt(dma_chan);
	}

	if (__dmac_channel_address_error_detected(dma_chan)) {
		__dmac_channel_clear_address_error(dma_chan);
	}

	if (__dmac_channel_descriptor_invalid_detected(dma_chan)) {
		__dmac_channel_clear_descriptor_invalid(dma_chan);
	}

	if (__dmac_channel_count_terminated_detected(dma_chan)) {
		__dmac_channel_clear_count_terminated(dma_chan);
	}

	if (__dmac_channel_transmit_end_detected(dma_chan)) {
		__dmac_channel_clear_transmit_end(dma_chan);
	}
    return IRQ_HANDLED;
}
    */

static int dma_nodesc_write(u32 s,u32 d,int len,int enable_iqe)
{
	REG_DMAC_DCCSR(dma_chan) = 0;
	REG_DMAC_DRSR(dma_chan) = DMAC_DRSR_RS_AUTO;
#if enable_cache
	REG_DMAC_DSAR(dma_chan) = (u32)virt_to_phys(s);
#else
	REG_DMAC_DSAR(dma_chan) =s & 0x1fffffff;// (u32)virt_to_phys((void *)s);
#endif
	REG_DMAC_DTAR(dma_chan) = d & 0x1fffffff;
	REG_DMAC_DTCR(dma_chan) = len / 32;
    //if (enable_iqe ==1)
    //{
	//REG_DMAC_DCMD(dma_chan) = DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BYTE | DMAC_DCMD_TIE;
    //}
    //else
    //{
        REG_DMAC_DCMD(dma_chan) = DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BYTE ;//| DMAC_DCMD_TIE;
    //}
	REG_DMAC_DCCSR(dma_chan) = DMAC_DCCSR_NDES | DMAC_DCCSR_EN;
	REG_DMAC_DMACR = DMAC_DMACR_DMAE; /* global DMA enable bit */
    return 0;
}

static void Enable_game_data_interrup(void)
{
__gpio_as_irq_rise_edge(fpga_data_port + nds_data_iqe_pinx);    //data
__gpio_unmask_irq(fpga_data_port + nds_data_iqe_pinx);
}

static void Disenable_game_data_interrup(void)
{
__gpio_as_disenable_irq(fpga_data_port + nds_data_iqe_pinx);

}

static void Enable_game_cmd_interrup(void)
{
__gpio_as_irq_fall_edge(fpga_cmd_port + nds_cmd_iqe_pinx);
__gpio_unmask_irq(fpga_cmd_port + nds_cmd_iqe_pinx);

}
static void Disenable_game_cmd_interrup(void)
{
__gpio_as_disenable_irq(fpga_cmd_port + nds_cmd_iqe_pinx);

}


extern void check_reg(int);

static irqreturn_t cmd_line_interrupt(int irq, void *dev_id)
{
    u8 nds_iqe_cmd;
    int i;
    u32 tempw,tempr;
    u8 isc1cmd;
    struct key_buf* keyp;

    u8* cmd_buf=(u8*)&cmd_buf32[0];

    cmd_buf32[0]=*(vu32*)cpld_fifo_read_ndswcmd_addr;
    cmd_buf32[1]=*(vu32*)cpld_fifo_read_ndswcmd_addr;
    nds_iqe_cmd=cmd_buf[0];
    isc1cmd= mm_check ==0 ? 0: (1<< enable_fix_video_rgb_bit);
    switch ( nds_iqe_cmd )
    {
        case 0xc0://∑µªÿcpu «Î«Û÷–∂œ
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit) |  (1<<fifo_clear_bit);
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit);
            *(fpgaport*)cpld_state_addr = 0;
            i = nds_iqe_list_w - nds_iqe_list_r;
            if (i != 0 && nds_iqe_list_w > nds_iqe_list_r)
            {
                dma_nodesc_write(nds_iqe_list[nds_iqe_list_r&0xf].c0_addr ,cpld_fifo_write_ndsrdata_addr,512,0);
                
                if (nds_iqe_list[nds_iqe_list_r&0xf].num == buf_c0_set)
                {
                    pmain_buf->buf_st_list[buf_c0_set].isused=0;
                };

                nds_iqe_list_r++;
            }
            else
            {
                dma_nodesc_write(nds_iqe_list[nds_iqe_list_r&0xf].c0_addr ,cpld_fifo_write_ndsrdata_addr,512,0);
                dgprintf( "nds_iqe c0 error" );
            }
        
            break;
        case 0xc1://NDS∂¡ 512*n
            isc1cmd = cmd_buf[7] & (( 1 << enable_fix_video_bit) | ( 1 << enable_fix_video_rgb_bit));
            
        case 0xc2://NDS∂¡ 512*n
            *(fpgaport*)write_addr_cmp_addr = (0x400-0x380) ; //(0x400-0x300) ;
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit) |  (1<<fifo_clear_bit) | isc1cmd;
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit) | isc1cmd;

             nds_iqe_addr=(cmd_buf[1]<<24) + (cmd_buf[2]<<16) +(cmd_buf[3]<<8)+(cmd_buf[4]<<0);
             nds_iqe_len=(cmd_buf[5]<<16) + (cmd_buf[6]<<8) +( cmd_buf[7]<<0);
             if (nds_iqe_len >= 0x800000)
             {
              *(vu32*)cpld_fifo_write_ndsrdata_addr = 0;
                 nds_iqe_len &= 0xff;
                 if (((u32)nds_iqe_len) >= buf_max_num) 
                 {
                     dgprintf( "buf_max_num error %x" ,nds_iqe_len);
                     nds_iqe_len=buf_max_num;
                 }
#if 0//def HTW_MP4_NDS
		if(nds_iqe_len == buf_video_up_0 || nds_iqe_len == buf_video_down_0){
		pmain_buf->buf_st_list[nds_iqe_len].isused--;
//printf("IN NDS 1 %08x\n", pmain_buf->buf_st_list[nds_iqe_len].isused);
		}
		else {
		pmain_buf->buf_st_list[nds_iqe_len].isused=0;
//printf("IN NDS 2 %08x\n", pmain_buf->buf_st_list[nds_iqe_len].isused);
		}
#else
		pmain_buf->buf_st_list[nds_iqe_len].isused=0;
#endif
                Disenable_game_data_interrup();
    
             }
             else
             {
                nds_iqe_addr += kmalloc_ptr_uncache;
                dma_nodesc_write(nds_iqe_addr ,cpld_fifo_write_ndsrdata_addr,fpga_fifo_size,0);
                nds_iqe_len &= ~0x1ff;
                
                nds_iqe_addr+=fpga_fifo_size;
                nds_iqe_len-=fpga_fifo_size;
                if(nds_iqe_len>0)
                {
                    Enable_game_data_interrup();
                }
                else
                {
                    Disenable_game_data_interrup();
                }
            }
            break;
        case 0xc3://NDS key
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit) |  (1<<fifo_clear_bit);
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit);
		    *(vu32*)cpld_fifo_write_ndsrdata_addr = 0;
            i= pmain_buf->key_write_num  - pmain_buf->key_read_num;

            if (i!= 0 && ((i& KEY_MASK) ==0))//full
            {                 
                pmain_buf->key_write_num--;                
                T();
            }
            else 
            {
                T();
#ifdef HTW_MP4_NDS    

            os_SemaphorePost(MP4_devices->sem4key);
#endif            
            }
            keyp =(struct key_buf*)(kmalloc_ptr_uncache + pmain_buf->key_buf_offset + ((pmain_buf->key_write_num & KEY_MASK) * sizeof(struct key_buf)));
            keyp->x=(cmd_buf[1]<<8) + cmd_buf[2];
            keyp->y=(cmd_buf[3]<<8) + cmd_buf[4];
            keyp->key=(cmd_buf[5]<<8) + cmd_buf[6];
            keyp->brightness=cmd_buf[7];

            mm_check =((cmd_buf[7] &( ((1)<<6) | ((1)<<7))) != ( ((1)<<6) | ((1)<<7)))?1:0;

            pmain_buf->key_write_num++;
			
            break;
        case 0xc5://NDS timer
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit) |  (1<<fifo_clear_bit);
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit);
            *(vu32*)cpld_fifo_write_ndsrdata_addr = 0;
            memcpy(&pmain_buf->nds_rtc,&cmd_buf[1],7);

            break;
        case 0xc4://NDS iqe
        *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit) |  (1<<fifo_clear_bit);
        *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit);
		tempw=nds_iqe_list_w - nds_iqe_list_r;
		*(vu32*)cpld_fifo_write_ndsrdata_addr = tempw | 0xa5a50000; 
		if(irq == 0x5a5a5a5a ) break;
            
             tempw=(cmd_buf[1]<<16) +(cmd_buf[2]<<8)+(cmd_buf[3]<<0);
             tempr=(cmd_buf[4]<<16) +(cmd_buf[5]<<8)+(cmd_buf[6]<<0);
		
            if(cmd_buf[7] == 1)
            {
                if (pmain_buf->nds_video_up_w < tempw)
                {

                    pmain_buf->nds_video_up_w=tempw;
                }
                pmain_buf->nds_video_up_r=tempr;
            }else if(cmd_buf[7] == 2)
            {
                if(pmain_buf->nds_audio_w < tempw)
                {
                    pmain_buf->nds_audio_w=tempw;
                }
                if(pmain_buf->nds_audio_r != tempr)
                {             
                pmain_buf->nds_audio_r=tempr;
#ifdef HTW_MP4_NDS
                    nds_updat_tim = os_TimeGet();
#endif                
                }
            }else if(cmd_buf[7] == 3)
            {
                if (pmain_buf->nds_video_down_w < tempw)
                {
                    pmain_buf->nds_video_down_w=tempw;
                }
                pmain_buf->nds_video_down_r=tempr;
            }
	   else if(cmd_buf[7] == 0xf0)//debug
            {
                dgprintf( "nds_video_up_w=%x\n" ,pmain_buf->nds_video_up_w);
                dgprintf( "nds_video_up_r=%x\n" ,pmain_buf->nds_video_up_r);
                dgprintf( "nds_video_down_w=%x\n" ,pmain_buf->nds_video_down_w);
                dgprintf( "nds_video_down_r=%x\n" ,pmain_buf->nds_video_down_r);
                dgprintf( "nds_audio_w=%x\n" ,pmain_buf->nds_audio_w);
                dgprintf( "nds_audio_r=%x\n" ,pmain_buf->nds_audio_r);
                dgprintf( "nds_iqe_list_r=%x\n" ,nds_iqe_list_r);
                dgprintf( "nds_iqe_list_w=%x\n" ,nds_iqe_list_w);
                
                dgprintf("PC= %08x\n", read_c0_epc());
            }
 
#if 0//enable_cache
            {
            int i = nds_iqe_list_w - nds_iqe_list_r;
            if (i != 0 && nds_iqe_list_w > nds_iqe_list_r)
            {
                    struct nds_iqe_st *nds_iqe_st_p = (struct nds_iqe_st *)nds_iqe_list[nds_iqe_list_r&0xf].c0_addr;
		    dma_cache_wback_inv_my((unsigned long)(nds_iqe_st_p), (unsigned long)512);
			if(nds_iqe_st_p->iqe_cmd == 0xc2)//==0xc0 ≥ˆ¥Ì
			{
		        	dma_cache_wback_inv_my((unsigned long)(nds_iqe_st_p->iqe_cpuaddr + kmalloc_ptr), (unsigned long)nds_iqe_st_p->iqe_len);
			}
	    }	
	    }
#endif 
            break;
         case 0x5d://º”√‹
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit) |  (1<<fifo_clear_bit) | (1<<7);
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit)| (1<<7);
            *(fpgaport*)cpld_spi_Count_addr = 0x1234;
            dma_nodesc_write( (u32)mmdata10 ,cpld_fifo_write_ndsrdata_addr,1024,0);
            break;


        case 0x5e://º”√‹
             *(fpgaport*)cpld_ctr_addr =(1<<fpga_mode_bit) | (1<<fifo_clear_bit) |BIT(key2_init_enable_bit);
            *(fpgaport*)cpld_ctr_addr =(1<<fpga_mode_bit) | BIT(key2_init_enable_bit);
            *(volatile unsigned short int*)cpld_x0_addr=0;
            *(volatile unsigned short int*)cpld_x1_addr=0;
            *(volatile unsigned short int*)cpld_x4_addr=0;
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit) |BIT(fifo_clear_bit);
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit) |BIT(enable_fix_video_bit) | BIT(enable_fix_video_rgb_bit);
            i= (*(vu16*)cpld_state_addr) & BIT(10);
            if (i!=0)
            {
                i= 1;
            }
            else
            {
                i=0;
            }


            #define RGB15_T(r,g,b)  (((r)|((g)<<5)|((b)<<10)))
            
            *(vu32*)cpld_fifo_write_ndsrdata_addr = ((RGB15_T(0x4,i,0x7))<<16) | RGB15_T(0xc,i,0x5);

            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit) | BIT(key2_init_enable_bit);

            *(volatile unsigned short int*)cpld_x0_addr=0x6bf3;
            *(volatile unsigned short int*)cpld_x1_addr=0xf0c2;
            *(volatile unsigned short int*)cpld_x4_addr=0x9252;
            *(volatile unsigned short int*)cpld_ctr_addr =(1<<fpga_mode_bit);
            break;
            
        default:
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit) |  (1<<fifo_clear_bit);
            *(fpgaport*)cpld_ctr_addr = (1<<fpga_mode_bit);
            *(fpgaport*)cpld_state_addr = 0;
            *(vu32*)cpld_fifo_write_ndsrdata_addr = 0;
            break;
    } 

	return IRQ_HANDLED;
}

static irqreturn_t data_line_interrupt(int irq, void *dev_id)
{
	u8 nds_iqe_cmd=cmd_buf32[0] & 0xff;
    switch (nds_iqe_cmd )
    {
        case 0xc1://NDS∂¡ 512*n
        case 0xc2://NDS∂¡ 512*n
            dma_nodesc_write(nds_iqe_addr ,cpld_fifo_write_ndsrdata_addr,fpga_fifo_size,0);
            nds_iqe_addr+=fpga_fifo_size;
            nds_iqe_len-=fpga_fifo_size;
            if (nds_iqe_len<=0)
            {
                Disenable_game_data_interrup();
            }
            break;
    }

	return IRQ_HANDLED;
}

//static
 int __do_MP4_ioctl(struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg)
{
    u32 temp32;//i;
    u32 temp32buf[10];
//    u32 tempw,tempr;
    struct nds_iqe_st *nds_iqe_st_p;

	  if(cmd==SET_CTR)
	  {
            *(fpgaport*)cpld_ctr_addr = *(fpgaport*)cpld_ctr_addr | arg;
           return 0;
	  }
      else if (cmd==CLR_CTR)
      {
            *(fpgaport*)cpld_ctr_addr = *(fpgaport*)cpld_ctr_addr & (~arg);
           return 0;
      }
      else if (cmd==READ_STATE)
      {
          temp32=*(fpgaport*)cpld_state_addr;
          copy_to_user((void*)arg, &temp32, 4);
          return 0;

      }
      else if (cmd==READ_IO)
      {
          copy_from_user(temp32buf,(void*)arg,2*4);
          temp32=*(fpgaport*)temp32buf[0];
          copy_to_user((void*)temp32buf[1], &temp32, 4);
          return 0;

      }
      else if (cmd==WRITE_IO)
      {
          copy_from_user(temp32buf,(void*)arg,2*4);
          *(fpgaport*)temp32buf[0]=temp32buf[1];
          return 0;

      }
      /*
      else if (cmd==IQE_WRITE)//c1 c2
      {
                copy_from_user(temp32buf,(void*)arg,5*4);
                temp32 = nds_iqe_list_w - nds_iqe_list_r;
                if ((temp32 == 0) || ((temp32 & 0xf) !=0 ))
                {
                    if (arg >= buf_max_num)
                    {
                        printk( "buf_max_num2 error %x" ,arg);
                        arg=buf_max_num;
                    }
                    if (pmain_buf->buf_st_list[arg].isused == 1)
                    {
                        printk( "buf_max_num2 isused error %x" ,isused);
                        return 0;
                    }

                    nds_iqe_list[nds_iqe_list_w&0xf].num = arg;
                    nds_iqe_st_p = (struct nds_iqe_st *)nds_iqe_list[nds_iqe_list_w&0xf].c0_addr;
                    nds_iqe_st_p->iqe_cmd = pmain_buf->buf_st_list[arg].nds_cmd;
                    nds_iqe_st_p->iqe_ndsaddr = 0;
                    nds_iqe_st_p->iqe_cpuaddr = pmain_buf->buf_st_list[arg].offset;
                    nds_iqe_st_p->iqe_datatype = pmain_buf->buf_st_list[arg].type;
                    nds_iqe_st_p->iqe_len = pmain_buf->buf_st_list[arg].use_len;

                    cpu_iqe_num++;
                    nds_iqe_st_p->iqe_num = cpu_iqe_num;
                    pmain_buf->buf_st_list[arg].isused=1;
                    nds_iqe_list_w++;

                }
                else
                {
                    return -EFAULT;
                }
                if(nds_iqe_st_p->iqe_cmd == 0xc2) 
                {
                    tempw = (u32)nds_iqe_st_p; tempw += 0x30;
                    tempr = nds_iqe_st_p->iqe_cpuaddr + kmalloc_ptr_uncache; 
                    for (i=0;i<nds_iqe_st_p->iqe_len ; i+=512)
                    {
                        *(u16*)tempw= *(u16*)(tempr + i + 512 -2);
                        *(u16*)(tempr + i + 512 -2) = 0x5a5a;
                        tempw+=2;
                    }                                    

                    switch (nds_iqe_st_p->iqe_datatype &0xffff)
                    {
                        case VIDEO_UP:
                            pmain_buf->nds_video_up_w++;
                            break;
                        case VIDEO_DOWN:
                            pmain_buf->nds_video_down_w++;
                            break;
                        case AUDIO_G:
				
                            pmain_buf->nds_audio_w++;
                            
                            break;
                    }
                }
                else if(nds_iqe_st_p->iqe_cmd == 0xc0)
               {
                    memcpy((char*)(nds_iqe_st_p), (char*)(nds_iqe_st_p->iqe_cpuaddr + kmalloc_ptr_uncache),512);
               }
#if enable_cache

		        dma_cache_wback_inv_my((unsigned long)(nds_iqe_st_p), (unsigned long)512);
		        dma_cache_wback_inv_my((unsigned long)(nds_iqe_st_p->iqe_cpuaddr + kmalloc_ptr), (unsigned long)nds_iqe_st_p->iqe_len);
#endif
                temp32=*(fpgaport*)cpld_ctr_addr;
                temp32 |= BIT(nds_iqe_out_bit);
                *(fpgaport*)cpld_ctr_addr = temp32;
                temp32 &=~(BIT(nds_iqe_out_bit));
                udelay(8);
                *(fpgaport*)cpld_ctr_addr = temp32;
               return 0;

      }
      */
      else if (cmd==IQE_UPDATE)
      {
        temp32 = nds_iqe_list_w - nds_iqe_list_r;
        if ((temp32 == 0) || ((temp32 & 0xf) !=0 ))
        {
            if (((u32)arg) >= buf_max_num)
            {
                dgprintf( "buf_max_num2 error %x" ,(u32)arg);
                arg=buf_max_num;
                return -EFAULT;
            }
            if (pmain_buf->buf_st_list[arg].isused != 0)
            {
#ifdef HTW_MP4_NDS
                if(arg == buf_video_up_0 || arg == buf_video_down_0)
                {
                    if(pmain_buf->buf_st_list[arg].isused >= 2)
                    {
                        //printk(  " overlay %s:%d:%d\n", __FILE__, __LINE__,pmain_buf->buf_st_list[arg].isused);
                        temp32=*(fpgaport*)cpld_ctr_addr;
                        temp32 |= BIT(nds_iqe_out_bit);
                        *(fpgaport*)cpld_ctr_addr = temp32;
                        return 0;
                    }
                }
                else 
#endif
                {
                    dgprintf( "buf_max_num2 isused error %x" ,pmain_buf->buf_st_list[arg].isused);
                    return -EFAULT;
                }
            }

            nds_iqe_list[nds_iqe_list_w&0xf].num = arg;
            nds_iqe_st_p = (struct nds_iqe_st *)nds_iqe_list[nds_iqe_list_w&0xf].c0_addr;
                    
            nds_iqe_st_p->iqe_cmd = pmain_buf->buf_st_list[arg].nds_cmd;
            nds_iqe_st_p->iqe_ndsaddr = 0;
            nds_iqe_st_p->iqe_cpuaddr = pmain_buf->buf_st_list[arg].offset;
            nds_iqe_st_p->iqe_datatype = pmain_buf->buf_st_list[arg].type;
            nds_iqe_st_p->iqe_len = pmain_buf->buf_st_list[arg].use_len;

            cpu_iqe_num++;
            nds_iqe_st_p->iqe_num = cpu_iqe_num;
        }
        else
        {
            return -EFAULT;
        }

        if(((nds_iqe_st_p->iqe_cmd&0xff) == 0xc2)|| ((nds_iqe_st_p->iqe_cmd&0xff) == 0xc1)) 
        {
#if 0
                    tempw = (u32)nds_iqe_st_p; tempw += 0x30;
                    tempr = nds_iqe_st_p->iqe_cpuaddr + kmalloc_ptr_uncache; 
                    for (i=0;i<nds_iqe_st_p->iqe_len ; i+=512)
                    {
                        *(u16*)tempw= *(u16*)(tempr + i + 512 -2);
                        *(u16*)(tempr + i + 512 -2) = 0x5a5a;
                        tempw += 2;
                    }                                    
#endif
            switch ((nds_iqe_st_p->iqe_cmd>>8) & 0xff)
            {
                case VIDEO_UP:
                    pmain_buf->nds_video_up_w++;
                    break;
                case VIDEO_DOWN:
                    pmain_buf->nds_video_down_w++;
                    break;
                case AUDIO_G:                    
#if 0 //def AUDIO_PRI_FISRT
                            __gpio_mask_irq(fpga_cmd_port + nds_cmd_iqe_pinx);
                            {
                            int i,n;
                            for(i=nds_iqe_list_r; i < nds_iqe_list_w; i++){
                                n = nds_iqe_list[i&0xf].num;
                                if( n < buf_audio_0){
                                    struct nds_iqe_list_st tmp = nds_iqe_list[i&0xf];
                                    nds_iqe_list[i&0xf] = nds_iqe_list[nds_iqe_list_w&0xf];
                                    nds_iqe_list[nds_iqe_list_w&0xf] = tmp;
                                    break;
                                }else if(n == buf_audio_0 || n == buf_audio_1)
                                {
                                    break;
                                }
                            }
                            }
                            __gpio_unmask_irq(fpga_cmd_port + nds_cmd_iqe_pinx);
#endif
                    if(1)	
                        pmain_buf->nds_audio_w++;
                    break;
            }
        }
        else if((nds_iqe_st_p->iqe_cmd&0xff) == 0xc0)
        {
            //printk("iqe_cmd c0 %x\n",nds_iqe_st_p->iqe_cpuaddr);
            memcpy((char*)(nds_iqe_st_p), (char*)(nds_iqe_st_p->iqe_cpuaddr + kmalloc_ptr_uncache),512);
        }
        else
        {
            dgprintf("iqe_cmd error %x\n",nds_iqe_st_p->iqe_cmd);
            return -EFAULT;
        }

#if enable_cache
#if 0
            __dcache_writeback_all();
#else
        dma_cache_wback_inv((unsigned long)(nds_iqe_st_p), (unsigned long)512);
        if(((nds_iqe_st_p->iqe_cmd&0xff) == 0xc2)|| ((nds_iqe_st_p->iqe_cmd&0xff) == 0xc1)) 
        {
            dma_cache_wback_inv((unsigned long)(nds_iqe_st_p->iqe_cpuaddr + kmalloc_ptr), (unsigned long)nds_iqe_st_p->iqe_len);
        }
#endif			
#endif
//		if(nds_iqe_st_p->iqe_cmd == 0xc0)
//               {
//	
//                    printk("iqe_cmd dma_cache ok \n");
//               }
        pmain_buf->buf_st_list[arg].isused++;
        nds_iqe_list_w++;
        *(fpgaport*)cpld_state_addr = BIT(nds_iqe_out_bit);
        //udelay(8);
        //*(fpgaport*)cpld_ctr_addr = temp32;
        return 0;
      }
      else if (cmd==IQE_READ)//c8
      {
        return -EFAULT;
      }
      else if (cmd==COMPLETE_STATE)
      {
          
          if (nds_cmd_iqe_pin !=0 )
          {
              temp32=1;
          }
          else
          {
            temp32=0;
          }
          if(nds_iqe_list_w == nds_iqe_list_r)
          {
              temp32 |= 2;
          }

          copy_to_user((void*)arg,&temp32,4);
          return 0;
      }
      else if (cmd==ENABLE_FIXRGB_UPVIDEO)//c8
      {
                if (arg != 0)
                {
                   pmain_buf->buf_st_list[buf_video_up_0].nds_cmd = (((1 <<enable_fix_video_bit ) |(1 <<enable_fix_video_rgb_bit ))<<24) |(buf_video_up_0<<16) | (VIDEO_UP <<8) | 0xc1;
                   pmain_buf->buf_st_list[buf_video_up_1].nds_cmd = (((1 <<enable_fix_video_bit )|(1 <<enable_fix_video_rgb_bit ))<<24) | (buf_video_up_1<<16) | (VIDEO_UP <<8) | 0xc1;
                }
                else
                {
                   pmain_buf->buf_st_list[buf_video_up_0].nds_cmd = (((1 <<enable_fix_video_bit ) )<<24) |(buf_video_up_0<<16) | (VIDEO_UP <<8) | 0xc1;
                   pmain_buf->buf_st_list[buf_video_up_1].nds_cmd = (((1 <<enable_fix_video_bit ) )<<24) |(buf_video_up_1<<16) | (VIDEO_UP <<8) | 0xc1;
                }


               return 0;

      }
      else if (cmd==ENABLE_FIXRGB_DOWNVIDEO)//c8
      {
                if (arg != 0)
                {
                   pmain_buf->buf_st_list[buf_video_down_0].nds_cmd = (((1 <<enable_fix_video_bit ) |(1 <<enable_fix_video_rgb_bit ))<<24) |(buf_video_down_0<<16) | (VIDEO_DOWN <<8) | 0xc1;
                   pmain_buf->buf_st_list[buf_video_down_1].nds_cmd = (((1 <<enable_fix_video_bit ) |(1 <<enable_fix_video_rgb_bit ))<<24) |(buf_video_down_1<<16) | (VIDEO_DOWN <<8) | 0xc1;
                }
                else
                {
                   pmain_buf->buf_st_list[buf_video_down_0].nds_cmd = (((1 <<enable_fix_video_bit ) )<<24) |(buf_video_down_0<<16) | (VIDEO_DOWN <<8) | 0xc1;
                   pmain_buf->buf_st_list[buf_video_down_1].nds_cmd = (((1 <<enable_fix_video_bit ) )<<24) |(buf_video_down_1<<16) | (VIDEO_DOWN <<8) | 0xc1;
                }


               return 0;

      }
          return -EFAULT;
}
#if 1 //def HTW_NDS_MP4
#define __MP4_ioctl_fb -1
#define __MP4_ioctl_dsp -1
#else


static int __MP4_ioctl_fb(struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg)
{
    //int ret = -1;
	        struct fb_var_screeninfo var_tmp;


struct fb_var_screeninfo {
	__u32 xres;			/* visible resolution		*/
	__u32 yres;
	__u32 xres_virtual;		/* virtual resolution		*/
	__u32 yres_virtual;
	__u32 xoffset;			/* offset from virtual to visible */
	__u32 yoffset;			/* resolution			*/

	__u32 bits_per_pixel;		/* guess what			*/
	__u32 grayscale;		/* != 0 Graylevels instead of colors */

	struct fb_bitfield red;		/* bitfield in fb mem if true color, */
	struct fb_bitfield green;	/* else only length is significant */
	struct fb_bitfield blue;
	struct fb_bitfield transp;	/* transparency			*/	

	__u32 nonstd;			/* != 0 Non standard pixel format */

	__u32 activate;			/* see FB_ACTIVATE_*		*/

	__u32 height;			/* height of picture in mm    */
	__u32 width;			/* width of picture in mm     */

	__u32 accel_flags;		/* (OBSOLETE) see fb_info.flags */

	/* Timing: All values in pixclocks, except pixclock (of course) */
	__u32 pixclock;			/* pixel clock in ps (pico seconds) */
	__u32 left_margin;		/* time from sync to picture	*/
	__u32 right_margin;		/* time from picture to sync	*/
	__u32 upper_margin;		/* time from sync to picture	*/
	__u32 lower_margin;
	__u32 hsync_len;		/* length of horizontal sync	*/
	__u32 vsync_len;		/* length of vertical sync	*/
	__u32 sync;			/* see FB_SYNC_*		*/
	__u32 vmode;			/* see FB_VMODE_*		*/
	__u32 rotate;			/* angle we rotate counter clockwise */
	__u32 reserved[5];		/* Reserved for future compatibility */
};
	  struct fb_var_screeninfo var = {.xres = 256,
	                                       .yres = 192,
	                                       .xres_virtual = 256,
	                                       .yres_virtual = 192,
	                                       .xoffset = 0,
	                                       .yoffset = 0,
	                                       .bits_per_pixel = 16,
	                                       .grayscale = 0,
	                                       
	                                       .red.offset = 10,
	                                       .red.length = 5,
	                                       .red.msb_right = 0,
	                                       .green.offset = 5,
	                                       .green.length = 5,
	                                       .green.msb_right = 0,
	                                       .blue.offset = 0,
	                                       .blue.length = 5,
	                                       .blue.msb_right = 0,	
	                                       .transp.offset = 15,
	                                       .transp.length = 0,
	                                       .transp.msb_right = 0,

	                                       .nonstd = 0,
	                                       .activate = FB_ACTIVATE_NOW,

	                                       .height = 256,
	                                       .width = 192,
	                                       .accel_flags = 0,
	                                       
	                                       .pixclock = 0,			/* pixel clock in ps (pico seconds) */
	                                       .left_margin =0,		/* time from sync to picture	*/
	                                       .right_margin =0,		/* time from picture to sync	*/
	                                       .upper_margin =0,		/* time from sync to picture	*/
	                                       .lower_margin =0,
	                                       .hsync_len =0,		/* length of horizontal sync	*/
	                                       .vsync_len =0,		/* length of vertical sync	*/
	                                       .sync =0,			/* see FB_SYNC_*		*/
	                                       .vmode =0,			/* see FB_VMODE_*		*/
	                                       .rotate =0,			/* angle we rotate counter clockwise */
	                                       .reserved = {[0 ... 4] = 0}
	                                      };
struct fb_fix_screeninfo {
	char id[16];			/* identification string eg "TT Builtin" */
	unsigned long smem_start;	/* Start of frame buffer mem */
					/* (physical address) */
	__u32 smem_len;			/* Length of frame buffer mem */
	__u32 type;			/* see FB_TYPE_*		*/
	__u32 type_aux;			/* Interleave for interleaved Planes */
	__u32 visual;			/* see FB_VISUAL_*		*/ 
	__u16 xpanstep;			/* zero if no hardware panning  */
	__u16 ypanstep;			/* zero if no hardware panning  */
	__u16 ywrapstep;		/* zero if no hardware ywrap    */
	__u32 line_length;		/* length of a line in bytes    */
	unsigned long mmio_start;	/* Start of Memory Mapped I/O   */
					/* (physical address) */
	__u32 mmio_len;			/* Length of Memory Mapped I/O  */
	__u32 accel;			/* Indicate to driver which	*/
					/*  specific chip/card we have	*/
	__u16 reserved[3];		/* Reserved for future compatibility */
};	                                      
	  struct fb_fix_screeninfo fix = {.id = {"FB Builtin"},
	                                       .smem_start = kmalloc_ptr_uncache&0x1fffffff,
	                                       .smem_len = LEN,
	                                       .type = FB_TYPE_PACKED_PIXELS,
	                                       .type_aux = 0,
	                                       .visual = FB_VISUAL_TRUECOLOR,
	                                       
	                                       .xpanstep = 0,
	                                       .ypanstep = 0,
	                                       .ywrapstep = 0,
	                                       
	                                       .line_length = 256*2,
	                                       .mmio_start  = 0,
	                                       .mmio_len = 0,
	                                       .accel    = 0,
	                                       .reserved = {[0 ... 2] = 0}
	                                      };
	//static struct fb_con2fbmap con2fb ={0,0};
	//static struct fb_cmap_user cmap = {0,0,0,0,0,0};
	//static struct fb_event event;
	void __user *argp = (void __user *)arg;
	//int i;
	

	switch (cmd) {
	case FBIOGET_VSCREENINFO:
		return copy_to_user(argp, &var,
				    sizeof(var)) ? -EFAULT : 0;
	case FBIOPUT_VSCREENINFO:
	        ;
	        //struct fb_var_screeninfo var_tmp;
		if (copy_from_user(&var_tmp, argp, sizeof(var)))
			return -EFAULT;

		if (copy_to_user(argp, &var, sizeof(var)))
			return -EFAULT;
		return 0;
	case FBIOGET_FSCREENINFO:
		return copy_to_user(argp, &fix,
				    sizeof(fix)) ? -EFAULT : 0;
	case FBIOPUTCMAP:

	case FBIOGETCMAP:

	case FBIOPAN_DISPLAY:

	case FBIO_CURSOR:

	case FBIOGET_CON2FBMAP:

	case FBIOPUT_CON2FBMAP:
		
	case FBIOBLANK:

	default:

	    return -EINVAL;
	}

}

static int __MP4_ioctl_dsp(struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg)
{
    int ret = -1;
         int ticks;// = (int)(jiffies - nds_updat_tim);
         int time_ms;// = jiffies_to_msecs(ticks);

    switch(cmd){
    case SNDCTL_DSP_GETODELAY:
         ;
         ticks = (int)(jiffies - nds_updat_tim);
         time_ms = jiffies_to_msecs(ticks);
         return put_user(time_ms, (int *)arg); 
    default :
         return -EINVAL;
    }
#if 0    
{
	int val,fullc,busyc,unfinish,newfragstotal,newfragsize;
	unsigned int flags;
	struct jz_i2s_controller_info *controller = (struct jz_i2s_controller_info *) file->private_data;
	count_info cinfo;
	audio_buf_info abinfo;
	int id, i;

	val = 0;
	switch (cmd) {
	case OSS_GETVERSION:
		return put_user(SOUND_VERSION, (int *)arg);
	case SNDCTL_DSP_RESET:
		return 0;
	case SNDCTL_DSP_SYNC:
		if (file->f_mode & FMODE_WRITE){
			while(pmain_buf->buf_st_list[buf_audio_0].isused != 0 
			      || pmain_buf->buf_st_list[buf_audio_1].isused != 0);
                }
		return 0;
	case SNDCTL_DSP_SPEED:
		/* set smaple rate */
		if (get_user(val, (int *)arg))
			return -EFAULT;
		if (val >= 0)
			jz_audio_set_speed(controller->dev_audio, val);
		
		return put_user(val, (int *)arg);
	case SNDCTL_DSP_STEREO:
		/* set stereo or mono channel */
		if (get_user(val, (int *)arg))
		    return -EFAULT;
		jz_audio_set_channels(controller->dev_audio, val ? 2 : 1);
	    
		return 0;
	case SNDCTL_DSP_GETBLKSIZE:
		//return put_user(jz_audio_fragsize / jz_audio_b, (int *)arg);
		return put_user(jz_audio_fragsize, (int *)arg);
	case SNDCTL_DSP_GETFMTS:
		/* Returns a mask of supported sample format*/
		return put_user(AFMT_U8 | AFMT_S16_LE, (int *)arg);
	case SNDCTL_DSP_SETFMT:
		/* Select sample format */
		if (get_user(val, (int *)arg))
			return -EFAULT;
		if (val != AFMT_QUERY)
			jz_audio_set_format(controller->dev_audio,val);
		else {
			if (file->f_mode & FMODE_READ)
				val = (jz_audio_format == 16) ?  AFMT_S16_LE : AFMT_U8;
			else
				val = (jz_audio_format == 16) ?  AFMT_S16_LE : AFMT_U8;
		}

		return put_user(val, (int *)arg);
	case SNDCTL_DSP_CHANNELS:
		if (get_user(val, (int *)arg))
			return -EFAULT;
		jz_audio_set_channels(controller->dev_audio, val);
		
		return put_user(val, (int *)arg);
	case SNDCTL_DSP_POST:
		/* FIXME: the same as RESET ?? */
		return 0;
	case SNDCTL_DSP_SUBDIVIDE:
		return 0;
	case SNDCTL_DSP_SETFRAGMENT:
		get_user(val, (long *) arg);
		newfragsize = 1 << (val & 0xFFFF);
		if (newfragsize < 4 * PAGE_SIZE)
			newfragsize = 4 * PAGE_SIZE;
		if (newfragsize > (16 * PAGE_SIZE))
			newfragsize = 16 * PAGE_SIZE;
		
		newfragstotal = (val >> 16) & 0x7FFF;
		if (newfragstotal < 2)
			newfragstotal = 2;
		if (newfragstotal > 32)
			newfragstotal = 32;
		if((jz_audio_fragstotal == newfragstotal) && (jz_audio_fragsize == newfragsize))
			return 0;
		Free_In_Out_queue(jz_audio_fragstotal,jz_audio_fragsize);
		mdelay(500);
		jz_audio_fragstotal = newfragstotal;
		jz_audio_fragsize = newfragsize;
		
		Init_In_Out_queue(jz_audio_fragstotal,jz_audio_fragsize);
		mdelay(10);

		return 0;
	case SNDCTL_DSP_GETCAPS:
		return put_user(DSP_CAP_REALTIME|DSP_CAP_BATCH, (int *)arg);
	case SNDCTL_DSP_NONBLOCK:
		file->f_flags |= O_NONBLOCK;
		return 0;
	case SNDCTL_DSP_SETDUPLEX:
		return -EINVAL;
	case SNDCTL_DSP_GETOSPACE:
	{
		int i, bytes = 0;
		if (!(file->f_mode & FMODE_WRITE))
			return -EINVAL;
		
		spin_lock_irqsave(&controller->ioctllock, flags);
		jz_audio_fragments = elements_in_queue(&out_empty_queue);
		for (i = 0; i < jz_audio_fragments; i++)
			bytes += jz_audio_fragsize / jz_audio_b;
		
		spin_unlock_irqrestore(&controller->ioctllock, flags);
                /* unused fragment amount */
		abinfo.fragments = jz_audio_fragments; 
		/* amount of fragments */
		abinfo.fragstotal = jz_audio_fragstotal; 
                /* fragment size in bytes */
		abinfo.fragsize = jz_audio_fragsize / jz_audio_b; 
		/* write size count without blocking in bytes */
		abinfo.bytes = bytes;

		return copy_to_user((void *)arg, &abinfo, sizeof(abinfo)) ? -EFAULT : 0;
	}
	case SNDCTL_DSP_GETISPACE:
	{
		int i, bytes = 0;
		if (!(file->f_mode & FMODE_READ))
			return -EINVAL;			
		jz_audio_fragments = elements_in_queue(&in_empty_queue);
		for (i = 0; i < jz_audio_fragments; i++)
			bytes += jz_audio_fragsize / jz_audio_b;
	    
		abinfo.fragments = jz_audio_fragments;
		abinfo.fragstotal = jz_audio_fragstotal;
		abinfo.fragsize = jz_audio_fragsize / jz_audio_b;
		abinfo.bytes = bytes;
		
		return copy_to_user((void *)arg, &abinfo, sizeof(abinfo)) ? -EFAULT : 0;
	}
	case SNDCTL_DSP_GETTRIGGER:
		val = 0;
		if (file->f_mode & FMODE_READ && in_dma_buf)
			val |= PCM_ENABLE_INPUT;
		if (file->f_mode & FMODE_WRITE && out_dma_buf)
			val |= PCM_ENABLE_OUTPUT;

		return put_user(val, (int *)arg);	
	case SNDCTL_DSP_SETTRIGGER:
		if (get_user(val, (int *)arg))
			return -EFAULT;
		return 0;
	case SNDCTL_DSP_GETIPTR:
		if (!(file->f_mode & FMODE_READ))
			return -EINVAL;

		spin_lock_irqsave(&controller->ioctllock, flags);
		cinfo.bytes = controller->total_bytes;
		cinfo.blocks = controller->blocks;
		cinfo.ptr = controller->nextIn;
		controller->blocks = 0;
		spin_unlock_irqrestore(&controller->ioctllock, flags);

		return copy_to_user((void *)arg, &cinfo, sizeof(cinfo));		
	case SNDCTL_DSP_GETOPTR:
		if (!(file->f_mode & FMODE_WRITE))
			return -EINVAL;

		spin_lock_irqsave(&controller->ioctllock, flags);
		cinfo.bytes = controller->total_bytes;
		cinfo.blocks = controller->blocks;
		cinfo.ptr = controller->nextOut;
		controller->blocks = 0;
		spin_unlock_irqrestore(&controller->ioctllock, flags);
		
		return copy_to_user((void *) arg, &cinfo, sizeof(cinfo));
	case SNDCTL_DSP_GETODELAY:
		if (!(file->f_mode & FMODE_WRITE))
			return -EINVAL;
		
		spin_lock_irqsave(&controller->ioctllock, flags);
		unfinish = 0;
		fullc = elements_in_queue(&out_full_queue);
		busyc = elements_in_queue(&out_busy_queue);
		for(i = 0;i < fullc ;i ++) {
			id = *(out_full_queue.id + i);
			unfinish += *(out_dma_buf_data_count + id); 
		}
		for(i = 0;i < busyc ;i ++) {
			id = *(out_busy_queue.id + i);
			unfinish += get_dma_residue(controller->dma1);
		}
		spin_unlock_irqrestore(&controller->ioctllock, flags);
		unfinish /= jz_audio_b;
	    
		return put_user(unfinish, (int *) arg);
	case SOUND_PCM_READ_RATE:
		return put_user(jz_audio_rate, (int *)arg);
	case SOUND_PCM_READ_CHANNELS:
		return put_user(jz_audio_channels, (int *)arg);
	case SOUND_PCM_READ_BITS:
		return put_user((jz_audio_format & (AFMT_S8 | AFMT_U8)) ? 8 : 16, (int *)arg);
	case SNDCTL_DSP_MAPINBUF:
	case SNDCTL_DSP_MAPOUTBUF:
	case SNDCTL_DSP_SETSYNCRO:
	case SOUND_PCM_WRITE_FILTER:
	case SOUND_PCM_READ_FILTER:
		return -EINVAL;
	}
}	
#endif	
	return -EINVAL;

    return ret;
}

#endif

#define  MP4_KEYBD_WAIT 0x7755
static int __MP4_ioctl_keybd(struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg)
{
    unsigned char err_tmp;
    if(cmd == MP4_KEYBD_WAIT){
        os_SemaphorePend(MP4_devices->sem4key,0, &err_tmp);
        
	char* keyp =( char*)(kmalloc_ptr_uncache + pmain_buf->key_buf_offset + ((pmain_buf->key_read_num & KEY_MASK) * sizeof(struct key_buf)));
        pmain_buf->key_read_num++;
        copy_to_user((void*)arg, (void*)keyp, sizeof(struct key_buf));
        return 0;
    }
    return -1;
}

 int MP4_ioctl(struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg)
{
    int ret = -1;
#ifdef HTW_NDS_MP4
    ret = __MP4_ioctl_keybd(inode, filp, cmd, arg);
    /*
    if(ret){
        ret = __MP4_ioctl_fb(inode, filp, cmd, arg);
    }
    if(ret){
        ret = __MP4_ioctl_dsp(inode, filp, cmd, arg);
    }    
    */    
#endif

    if(ret){
    unsigned char tmp;
#ifdef HTW_NDS_MP4
    os_SemaphorePend(MP4_devices->mutex, 0, &tmp);
    if(tmp){
        dgprintf("fail in get MP4_devices->mutex");
        ret = tmp;
        goto out;
    }    
#endif    
        ret = __do_MP4_ioctl(inode, filp, cmd, arg);
#ifdef HTW_NDS_MP4
    os_SemaphorePost(MP4_devices->mutex);
#endif        
    }
out:
    return ret;

}
static loff_t MP4_llseek(struct file *filp, loff_t off, int whence)
{
	return 0;
}
#if 0
static int MP4_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long size = vma->vm_end - vma->vm_start;
    if (size>LEN)
    {
            dgprintf("size too big\n");
            return(-ENXIO);
    }
#if  enable_cache

	vma->vm_flags |= VM_LOCKED;
    if (remap_pfn_range(vma,vma->vm_start,
                         virt_to_phys((void*)((unsigned long)kmalloc_ptr))  >> PAGE_SHIFT,
                         size,
                         PAGE_SHARED))
                         //vma->vm_page_prot))
    {
            dgprintf("remap page range failed\n");
            return -ENXIO;
    }
#else
	vma->vm_flags |= VM_IO | VM_RESERVED;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);	


	if (io_remap_pfn_range(vma, vma->vm_start,
                   virt_to_phys((void*)((unsigned long)kmalloc_ptr))  >> PAGE_SHIFT,
			       size,
			       vma->vm_page_prot))
    {
		return -EAGAIN;
	}
#endif



    return(0);
}


static struct file_operations MP4_fops = {
	.owner	=	THIS_MODULE,
	.llseek	=	MP4_llseek,
	.read		=	MP4_read,
	.write	=	MP4_write,
	.readdir	=	NULL,
	.poll		=	NULL,
	.ioctl	=	MP4_ioctl,
	.mmap		=	MP4_mmap,
	.open		=	MP4_open,
	.flush	=	NULL,
	.release	=	MP4_release,
	.fsync	=	NULL,
	.fasync	=	NULL,
	.lock		=	NULL,
};
static void MP4_cleanup_module(void)
{
	dev_t devno = MKDEV(MP4_MAJOR, MP4_MINOR);

	if (MP4_devices) 
	{
		cdev_del(&MP4_devices->cdev);
		kfree(MP4_devices);
	}
	jz_free_dma(dma_chan);
    free_pages(kmalloc_ptr,get_order(LEN));
	unregister_chrdev_region(devno,1);
}
#endif
#define __gpio_as_emc8()				\
do {						\
	REG_GPIO_PXFUNS(0) = 0x000000ff;	\
	REG_GPIO_PXSELC(0) = 0x000000ff;	\
	REG_GPIO_PXFUNS(1) = 0x700001ff;	\
	REG_GPIO_PXSELC(1) = 0x700001ff;	\
} while (0)

static void gpio_init(void)
{
//__gpio_as_emc8();

    __gpio_as_func0(fpga_cs_port + fpga_cs_pinx); //cs3
    __gpio_as_func0(32 + 29); //rd
    __gpio_as_func0(32 + 30); //wd
#if enable_initfpga
	//__gpio_as_irq_fall_edge(fpga_cmd_port + nds_cmd_iqe_pinx);    //cmd
	Disenable_game_cmd_interrup();
#else
	Enable_game_cmd_interrup();
#endif
	__gpio_disable_pull(fpga_cmd_port + nds_cmd_iqe_pinx);         //disable pull

	Disenable_game_data_interrup();
	__gpio_disable_pull(fpga_data_port + nds_data_iqe_pinx);         //disable pull
	
    __gpio_as_func0(64 + 27); //wait

/*
REG_EMC_SMCR3 = ((3<<24) | //strv
                    (3<<20) | //taw r
                    (3<<16) | //tbp w
                    (3<<12) | //tah
                    (3<<8) | //tas
                    (1<<6) | //bw
                    (0<<3) | //bcm  
                    (0<<1) | //bl
                    (0<<0)  //smt
                    ) ;  
    REG_EMC_SACR3 =( (0xc<<8)|//BASE  0xc CS3;0x8 CS4
                    (0xfc<<0));//MASK
*/
INITFPGAPORTTIME();

    *(volatile unsigned short int*)cpld_ctr_addr = BIT(key2_init_enable_bit) ;
    *(volatile unsigned short int*)cpld_x0_addr=0x6bf3;
    *(volatile unsigned short int*)cpld_x1_addr=0xf0c2;
    *(volatile unsigned short int*)cpld_x4_addr=0x9252;
    *(volatile unsigned short int*)cpld_ctr_addr =0;

	udelay(1);
}
static int init_nds_var(void)
{
    u32 *temp32p;
    int offset;
    //int i;
    u32 getbuf;
    int offsetindex;
    //struct nds_iqe_st *nds_iqe_st_p;

    struct video_set *video_setp;
    struct audio_set *audio_setp;

    getbuf= pmain_buf->buf_st_list[buf_c0_set].offset + kmalloc_ptr_uncache;

    temp32p = (u32*)getbuf;

    memset(temp32p,0,512);
    temp32p[0]=0xc0;

    offset = 0x60;
    offsetindex=0x4;


    temp32p[offsetindex/4]=IS_SET_ENABLE_VIDEO;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=0;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_AUDIO;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=0;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_KEY;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=0;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_RTC;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=0;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_CLEAR_VAR;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=1;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_AUDIO;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;//sizeof(audio_set);
    audio_setp =(struct audio_set *)&temp32p[offset/4];
    audio_setp->sample=44100;
    audio_setp->timer_l_data=32768;
    audio_setp->timer_l_ctr=0;
    audio_setp->timer_h_data= 1024;//4096;
    audio_setp->timer_h_ctr=0;
    audio_setp->sample_size= 1024;//4096;
    audio_setp->sample_bit=16;
    audio_setp->data_type=0;
    audio_setp->stereo=1;
    offset+=sizeof(struct audio_set);

    temp32p[offsetindex/4]=IS_SET_VIDEO;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;//sizeof(audio_set);
    video_setp =(struct video_set *)&temp32p[offset/4];
    video_setp->frequence=25;
    video_setp->timer_l_data=32768;
    video_setp->timer_l_ctr=0;
    video_setp->timer_h_data=32768/25;
    video_setp->timer_h_ctr=0;
    video_setp->width=256;
    video_setp->height=192;
    video_setp->data_type =0;
    video_setp->play_buf=0;
    video_setp->swap=0;
    offset+=sizeof(struct video_set);
    __do_MP4_ioctl(0, 0,IQE_UPDATE,buf_c0_set);
     while(nds_cmd_iqe_pin != 0);//c4
#if 0     
     dgprintf( KERN_INFO"  %s:%d:%08x:%08x:%08x:%08x\n", __FILE__, __LINE__,cmd_buf32[0],cmd_buf32[1],nds_iqe_list_w,nds_iqe_list_r);
     dgprintf( KERN_INFO"  %s:%d:%08x:%08x:%08x:%08x\n", __FILE__, __LINE__,cmd_buf32[0],cmd_buf32[1],nds_iqe_list_w,nds_iqe_list_r);
     dgprintf( KERN_INFO"  %s:%d:%08x:%08x:%08x:%08x\n", __FILE__, __LINE__,cmd_buf32[0],cmd_buf32[1],nds_iqe_list_w,nds_iqe_list_r);
     dgprintf( KERN_INFO"  %s:%d:%08x:%08x:%08x:%08x\n", __FILE__, __LINE__,cmd_buf32[0],cmd_buf32[1],nds_iqe_list_w,nds_iqe_list_r);
     dgprintf( KERN_INFO"  %s:%d:%08x:%08x:%08x:%08x\n", __FILE__, __LINE__,cmd_buf32[0],cmd_buf32[1],nds_iqe_list_w,nds_iqe_list_r);
     dgprintf( KERN_INFO"  %s:%d:%08x:%08x:%08x:%08x\n", __FILE__, __LINE__,cmd_buf32[0],cmd_buf32[1],nds_iqe_list_w,nds_iqe_list_r);
     dgprintf( KERN_INFO"  %s:%d:%08x:%08x:%08x:%08x\n", __FILE__, __LINE__,cmd_buf32[0],cmd_buf32[1],nds_iqe_list_w,nds_iqe_list_r);
     dgprintf( KERN_INFO"  %s:%d:%08x:%08x:%08x:%08x\n", __FILE__, __LINE__,cmd_buf32[0],cmd_buf32[1],nds_iqe_list_w,nds_iqe_list_r);
#endif
	cmd_line_interrupt(0x5a5a5a5a, 0);
     while(nds_cmd_iqe_pin == 0);
     //printk( KERN_INFO"  %s:%d:%08x:%08x:%08x:%08x\n", __FILE__, __LINE__,cmd_buf32[0],cmd_buf32[1],nds_iqe_list_w,nds_iqe_list_r);
#if 1    
     while(nds_cmd_iqe_pin != 0);//c0
     //printk( KERN_INFO"  %s:%d\n", __FILE__, __LINE__);
	cmd_line_interrupt(0x5a5a5a5a, 0);
     //printk( KERN_INFO"  %s:%d:%x:%x\n", __FILE__, __LINE__,cmd_buf32[0],cmd_buf32[1]);	
#endif	
	Enable_game_cmd_interrup();

    //printk( "  %s:%d:%x:%x\n", __FILE__, __LINE__,cmd_buf32[0],cmd_buf32[1]);	
    while (pmain_buf->buf_st_list[buf_c0_set].isused == 1);

    __do_MP4_ioctl(0, 0,ENABLE_FIXRGB_UPVIDEO,0);
    __do_MP4_ioctl(0, 0,ENABLE_FIXRGB_DOWNVIDEO,0);
    return 0;
}

static int init_fpga(void)
{

    *(fpgaport*)cpld_ctr_addr = BIT(fifo_clear_bit) | (1<<fpga_mode_bit);
    udelay(200);
    *(fpgaport*)cpld_ctr_addr = 0 | (1<<fpga_mode_bit);
    init_nds_var();
	return 0;
}

 int MP4_init_module(void)
{
	int result;
    u32  virt_addr; 
    struct buf_st buf_st_temp;
    int i;

	//dev_t dev = 0;


    gpio_init();
	
	//dev = MKDEV(MP4_MAJOR, MP4_MINOR);
	//result = register_chrdev_region(dev, 1, "MP4");


	MP4_devices = (struct MP4_dev*)malloc(sizeof(struct MP4_dev));
	if (!MP4_devices)
	{
		result = -1;
		goto fail;
	}
	memset(MP4_devices, 0, sizeof(struct MP4_dev));


	if( request_irq( NDS_CMD_IRQ,cmd_line_interrupt, NULL ) ,0)
	{
		dgprintf(  "[FALLED: Cannot register cmd_line_interrupt!]\n" );
		goto nds_cmd_irq_failed;
	}
	//else
	//{
	//	printk( "[OK]\n" );
	//}

	if( request_irq( NDS_DATA_IRQ,data_line_interrupt, NULL ) ,0)
	{
		dgprintf(  "[FALLED: Cannot register data_line_interrupt!]\n" );
		goto nds_data_irq_failed;
	}
	//else
	//{
	//	printk( "[OK]\n" );
	//}

	//dma_chan = jz_request_dma(DMA_ID_AUTO, "auto", jz4740_dma_irq1,
	dma_chan = DMA_ID_AUTO;
	dma_request(DMA_ID_AUTO, NULL, NULL, NULL, DMAC_DRSR_RS_AUTO);
	if (dma_chan < 0) {
		dgprintf("Setup irq failed\n");
		return -1;
	}
    //printk("dma_chan=%x\n",dma_chan);

//#if ((FRAME_BUFF_ADDRESS + LEN - 1)&0xfff00000) > (FRAME_BUFF_ADDRESS &0xfff00000) 
//    #error "FRAME_BUFF_ADDRESS error"
//#endif
/*
		kmalloc_ptr = FRAME_BUFF_ADDRESS;//0x80F38000;//0x81f00000;//__get_free_pages(GFP_KERNEL | GFP_DMA, get_order(LEN));
        if (kmalloc_ptr == 0)
        {
			printk(" can't allocate required DMA(OUT) buffers.\n");
			return -1;
        }
*/
        kmalloc_ptr = malloc(LEN+32);//     FRAME_BUFF_ADDRESS1;//0x80F38000;//0x81f00000;//__get_free_pages(GFP_KERNEL | GFP_DMA, get_order(LEN));
        
        if (kmalloc_ptr == 0)
        {
			dgprintf(" can't allocate required DMA(OUT) buffers.\n");
			return -1;
        }
        kmalloc_ptr += (32 -1);
        kmalloc_ptr &= ~(32 -1);
 

#if 0
       for (virt_addr=(unsigned long)kmalloc_ptr; virt_addr<((unsigned long)kmalloc_ptr+LEN);
	    virt_addr+=PAGE_SIZE)
       {
	        SetPageReserved(virt_to_page((void*)virt_addr));
       }
#endif       
#if enable_cache
        kmalloc_ptr_uncache=kmalloc_ptr;
#else
       kmalloc_ptr_uncache=(u32)((((unsigned int)virt_to_phys((char*)kmalloc_ptr)) & 0x1fffffff) | 0xa0000000);
#endif
       pmain_buf=(struct main_buf*)kmalloc_ptr_uncache;
	   memset((char*)kmalloc_ptr,0,0x4000);
       pmain_buf->key_buf_offset = 0x4000;
       pmain_buf->key_buf_len = 0x2000;
       pmain_buf->key_write_num = 0;
       pmain_buf->key_read_num = 0;

pnds_sys_rtc = &pmain_buf->nds_rtc;
        buf_st_temp.isused=0;
        buf_st_temp.offset=pmain_buf->key_buf_offset + pmain_buf->key_buf_len;
        buf_st_temp.len=256*192*2;
        buf_st_temp.use_len=256*192*2;
        buf_st_temp.nds_max_len=256*192*2;
        buf_st_temp.nds_cmd=(((1 <<enable_fix_video_bit ) |(1 <<enable_fix_video_rgb_bit ))<<24) | (buf_video_up_0<<16) | (VIDEO_UP <<8) | 0xc1;
        buf_st_temp.type= 0;
        pmain_buf->buf_st_list[buf_video_up_0] = buf_st_temp;

        buf_st_temp.offset=buf_st_temp.offset + buf_st_temp.len;
        buf_st_temp.nds_cmd=(((1 <<enable_fix_video_bit ) |(1 <<enable_fix_video_rgb_bit ))<<24) |  (buf_video_up_1<<16) | (VIDEO_UP <<8) | 0xc1;
        pmain_buf->buf_st_list[buf_video_up_1] = buf_st_temp;

        buf_st_temp.offset=buf_st_temp.offset + buf_st_temp.len;
        buf_st_temp.nds_cmd= (((1 <<enable_fix_video_bit ) |(1 <<enable_fix_video_rgb_bit ))<<24) | (buf_video_down_0<<16) | (VIDEO_DOWN <<8) | 0xc1;
        pmain_buf->buf_st_list[buf_video_down_0] = buf_st_temp;

        buf_st_temp.offset=buf_st_temp.offset + buf_st_temp.len;
        buf_st_temp.nds_cmd= (((1 <<enable_fix_video_bit ) |(1 <<enable_fix_video_rgb_bit ))<<24) | (buf_video_down_1<<16) | (VIDEO_DOWN <<8) | 0xc1;
        pmain_buf->buf_st_list[buf_video_down_1] = buf_st_temp;

        buf_st_temp.offset=buf_st_temp.offset + buf_st_temp.len;
        buf_st_temp.len=48000 *4;
        buf_st_temp.use_len=4096 * 4 ;
        buf_st_temp.nds_max_len=65536;
        buf_st_temp.nds_cmd= (buf_audio_0<<16) | (AUDIO_G<<8) | 0xc2;
        pmain_buf->buf_st_list[buf_audio_0] = buf_st_temp;

        buf_st_temp.offset=buf_st_temp.offset + buf_st_temp.len;
        buf_st_temp.nds_cmd= (buf_audio_1<<16) | (AUDIO_G<<8) | 0xc2;
        pmain_buf->buf_st_list[buf_audio_1] = buf_st_temp;


        buf_st_temp.offset=buf_st_temp.offset + buf_st_temp.len;
        buf_st_temp.len=512;
        buf_st_temp.use_len=512;
        buf_st_temp.nds_max_len=512;
        buf_st_temp.nds_cmd= (buf_c0_set<<16)|0xc0;
        pmain_buf->buf_st_list[buf_c0_set] = buf_st_temp;



       pmain_buf->tempbuff = buf_st_temp.offset + buf_st_temp.len;
       pmain_buf->tempbuff_len =LEN - pmain_buf->tempbuff;


        buf_st_temp.isused=0;
        buf_st_temp.offset=pmain_buf->tempbuff;
        buf_st_temp.len=pmain_buf->tempbuff_len;
        buf_st_temp.use_len=pmain_buf->tempbuff_len;
        buf_st_temp.nds_max_len=pmain_buf->tempbuff_len;
        buf_st_temp.nds_cmd=0;
        buf_st_temp.type= (buf_max_num<<16);
        pmain_buf->buf_st_list[buf_max_num] = buf_st_temp;

       pmain_buf->nds_video_up_w=0;
       pmain_buf->nds_video_up_r=0;
       pmain_buf->nds_video_down_w=0;
       pmain_buf->nds_video_down_r=0;
       pmain_buf->nds_audio_w=0;
       pmain_buf->nds_audio_r=0;

        for (i=0;i< 16;i++ )
        {
            nds_iqe_list[i].num=0;
#if enable_cache
            nds_iqe_list[i].c0_addr = (u32)&pmain_buf->nds_iqe_list_c0[i][0];
#else
           nds_iqe_list[i].c0_addr=(u32)((((unsigned int)virt_to_phys((char*)&pmain_buf->nds_iqe_list_c0[i][0])) & 0x1fffffff) | 0xa0000000);
#endif
        }

        cpu_iqe_num=0;
        nds_iqe_list_w=0;
        nds_iqe_list_r=0;

        mm_check=0;

     //unsigned char err_tmp;
     MP4_devices->mutex = os_SemaphoreCreate(1);
     MP4_devices->sem4key = os_SemaphoreCreate(0);
#if 0
	init_MUTEX(&MP4_devices->sem);
	cdev_init(&MP4_devices->cdev, &MP4_fops);
	MP4_devices->cdev.owner = THIS_MODULE;
	MP4_devices->cdev.ops = &MP4_fops;
	result = cdev_add (&MP4_devices->cdev, dev, 1);
#endif	
	if(0)//(result)
	{
		dgprintf(  "Error %d adding MP4\n", result);
		goto fail;
	}
	//init_waitqueue_head(&MP4_devices->wq_read);
	//init_waitqueue_head(&MP4_devices->wq_write);
#if 0

	dgprintf("MP4_buf =%#08x:%#08x",(__u32)pmain_buf,kmalloc_ptr);
    dgprintf("pmain_buf[0] =%x,%x\n",pmain_buf->buf_st_list[0].offset,pmain_buf->buf_st_list[0].len);
    dgprintf("pmain_buf[1] =%x,%x\n",pmain_buf->buf_st_list[1].offset,pmain_buf->buf_st_list[1].len);
    dgprintf("pmain_buf[2] =%x,%x\n",pmain_buf->buf_st_list[2].offset,pmain_buf->buf_st_list[2].len);
    dgprintf("pmain_buf[3] =%x,%x\n",pmain_buf->buf_st_list[3].offset,pmain_buf->buf_st_list[3].len);
    dgprintf("pmain_buf[4] =%x,%x\n",pmain_buf->buf_st_list[4].offset,pmain_buf->buf_st_list[4].len);
    dgprintf("pmain_buf[5] =%x,%x\n",pmain_buf->buf_st_list[5].offset,pmain_buf->buf_st_list[5].len);
    dgprintf("pmain_buf[6] =%x,%x\n",pmain_buf->buf_st_list[6].offset,pmain_buf->buf_st_list[6].len);
    dgprintf("nds_video_up =%x,%x\n",pmain_buf->nds_video_up_w,pmain_buf->nds_video_up_r);
    dgprintf("nds_video_down =%x,%x\n",pmain_buf->nds_video_down_w,pmain_buf->nds_video_down_r);
    dgprintf("nds_audio =%x,%x\n",pmain_buf->nds_audio_w,pmain_buf->nds_audio_r);


#endif
	
#if enable_initfpga
    init_fpga();
#endif
	//dma_cache_wback_inv_my((unsigned long)(kmalloc_ptr), (unsigned long)1024);

dgprintf("mp4 ok\n");
	return 0;
nds_data_irq_failed:free_irq( NDS_CMD_IRQ );
nds_cmd_irq_failed:return -1;

fail:
	//MP4_cleanup_module();
	dgprintf(" fail in request Semaphore Create");
	return result;
}

//module_init(MP4_init_module);
//module_exit(MP4_cleanup_module);


/*
int getmm1_ok(void)
{
    return (curr_input.brightness >>6) & 1;
}
int getmm2_ok(void)
{
    return (curr_input.brightness >>7) & 1;
}
*/


