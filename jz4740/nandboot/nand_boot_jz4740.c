/*
 * Copyright (C) 2007 Ingenic Semiconductor Inc.
 * Author: Peter <jlwei@ingenic.cn>
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

#include <nand.h>
#if JZ4740_PAVO
#include <pavo.h>
#else
#if JZ4740_VIRGO
#include <virgo.h>
#endif
#endif
#include <jz4740.h>
#define uchar u8
/*
 * NAND flash definitions
 */

#define NAND_DATAPORT	0xb8000000
#define NAND_ADDRPORT	0xb8010000
#define NAND_COMMPORT	0xb8008000

#define ECC_BLOCK	512
#define ECC_POS		6
#define PAR_SIZE	9

#define __nand_enable()		(REG_EMC_NFCSR |= EMC_NFCSR_NFE1 | EMC_NFCSR_NFCE1)
#define __nand_disable()	(REG_EMC_NFCSR &= ~(EMC_NFCSR_NFCE1))
#define __nand_ecc_rs_encoding() \
	(REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_RS | EMC_NFECR_RS_ENCODING)
#define __nand_ecc_rs_decoding() \
	(REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_RS | EMC_NFECR_RS_DECODING)
#define __nand_ecc_disable()	(REG_EMC_NFECR &= ~EMC_NFECR_ECCE)
#define __nand_ecc_encode_sync() while (!(REG_EMC_NFINTS & EMC_NFINTS_ENCF))
#define __nand_ecc_decode_sync() while (!(REG_EMC_NFINTS & EMC_NFINTS_DECF))

static inline void __nand_dev_ready(void)
{
	unsigned int timeout = 10000;
	while ((REG_GPIO_PXPIN(2) & 0x40000000) && timeout--);
	while (!(REG_GPIO_PXPIN(2) & 0x40000000));
}

#define __nand_cmd(n)		(REG8(NAND_COMMPORT) = (n))
#define __nand_addr(n)		(REG8(NAND_ADDRPORT) = (n))
#define __nand_data8()		REG8(NAND_DATAPORT)
#define __nand_data16()		REG16(NAND_DATAPORT)

/*
 * NAND flash parameters
 */
static int bus_width = 8;
static int page_size = 2048;
static int oob_size = 64;
static int ecc_count = 4;
static int row_cycle = 3;
static int page_per_block = NAND_PAGE_PER_BLOCK;
static int bad_block_pos = 0;

static int block_size = NAND_BLOCK_SIZE;//131072;

static unsigned char oob_buf[128] = {0};

/*
 * External routines
 */
extern void flush_cache_all(void);
extern int serial_init(void);
extern void serial_puts(const char *s);
extern void sdram_init(void);
extern void pll_init(void);

/*
 * NAND flash routines
 */

static inline void nand_read_buf16(void *buf, int count)
{
	int i;
	u16 *p = (u16 *)buf;

	for (i = 0; i < count; i += 2)
		*p++ = __nand_data16();
}

static inline void nand_read_buf8(void *buf, int count)
{
	int i;
	u8 *p = (u8 *)buf;
	u8 d;
	for (i = 0; i < count; i++)
		*p++ =  __nand_data8();
		
	
}

static inline void nand_read_buf(void *buf, int count, int bw)
{
	
	if (bw == 8)
		nand_read_buf8(buf, count);
	else
		nand_read_buf16(buf, count);
}

static void rs_correct(unsigned char *buf, int idx, int mask)
{
	int i, j;
	unsigned short d, d1, dm;

	i = (idx * 9) >> 3;
	j = (idx * 9) & 0x7;

	i = (j == 0) ? (i - 1) : i;
	j = (j == 0) ? 7 : (j - 1);

	d = (buf[i] << 8) | buf[i - 1];

	d1 = (d >> j) & 0x1ff;
	d1 ^= mask;

	dm = ~(0x1ff << j);
	d = (d & dm) | (d1 << j);

	buf[i - 1] = d & 0xff;
	buf[i] = (d >> 8) & 0xff;
}

static int nand_read_oob(int page_addr, uchar *buf, int size)
{
	int col_addr;

	if (page_size == 2048)
		col_addr = 2048;
	else
		col_addr = 0;

	if (page_size == 2048)
		/* Send READ0 command */
		__nand_cmd(NAND_CMD_READ0);
	else
		/* Send READOOB command */
		__nand_cmd(NAND_CMD_READOOB);

	/* Send column address */
	__nand_addr(col_addr & 0xff);
	if (page_size == 2048)
		__nand_addr((col_addr >> 8) & 0xff);

	/* Send page address */
	__nand_addr(page_addr & 0xff);
	__nand_addr((page_addr >> 8) & 0xff);
	if (row_cycle == 3)
		__nand_addr((page_addr >> 16) & 0xff);

	/* Send READSTART command for 2048 ps NAND */
	if (page_size == 2048)
		__nand_cmd(NAND_CMD_READSTART);

	/* Wait for device ready */
	__nand_dev_ready();

	/* Read oob data */
	nand_read_buf(buf, size, bus_width);

	return 0;
}

static int nand_read_page(int block, int page, uchar *dst, uchar *oobbuf)
{
	int page_addr = page + block * page_per_block;
	uchar *databuf = dst, *tmpbuf;
	int i, j;

	/*
	 * Read oob data
	 */
	nand_read_oob(page_addr, oobbuf, oob_size);

	/*
	 * Read page data
	 */

	/* Send READ0 command */
	__nand_cmd(NAND_CMD_READ0);

	/* Send column address */
	__nand_addr(0);
	if (page_size == 2048)
		__nand_addr(0);

	/* Send page address */
	__nand_addr(page_addr & 0xff);
	__nand_addr((page_addr >> 8) & 0xff);
	if (row_cycle == 3)
		__nand_addr((page_addr >> 16) & 0xff);

	/* Send READSTART command for 2048 ps NAND */
	if (page_size == 2048)
		__nand_cmd(NAND_CMD_READSTART);

	/* Wait for device ready */
	//serial_puts("1\r\n");
	__nand_dev_ready();
	/* Read page data */
	tmpbuf = databuf;

	for (i = 0; i < ecc_count; i++) {
		volatile unsigned char *paraddr = (volatile unsigned char *)EMC_NFPAR0;
		unsigned int stat;
	       
		/* Enable RS decoding */
		REG_EMC_NFINTS = 0x0;
		__nand_ecc_rs_decoding();

		/* Read data */
		nand_read_buf((void *)tmpbuf, ECC_BLOCK, bus_width);

		/* Set PAR values */
		for (j = 0; j < PAR_SIZE; j++) {
			*paraddr++ = oobbuf[ECC_POS + i*PAR_SIZE + j];
		}

		/* Set PRDY */
 		REG_EMC_NFECR |= EMC_NFECR_PRDY;
		
               /* Wait for completion */
		
		__nand_ecc_decode_sync();
		
		/* Disable decoding */
		__nand_ecc_disable();

		/* Check result of decoding */
		stat = REG_EMC_NFINTS;
		if (stat & EMC_NFINTS_ERR) {
			/* Error occurred */
			if (stat & EMC_NFINTS_UNCOR) {
				/* Uncorrectable error occurred */
			}
			else {
				unsigned int errcnt, index, mask;

				errcnt = (stat & EMC_NFINTS_ERRCNT_MASK) >> EMC_NFINTS_ERRCNT_BIT;
				switch (errcnt) {
				case 4:
					index = (REG_EMC_NFERR3 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT;
					mask = (REG_EMC_NFERR3 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT;
					rs_correct(databuf, index, mask);
				case 3:
					index = (REG_EMC_NFERR2 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT;
					mask = (REG_EMC_NFERR2 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT;
					rs_correct(databuf, index, mask);
				case 2:
					index = (REG_EMC_NFERR1 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT;
					mask = (REG_EMC_NFERR1 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT;
					rs_correct(databuf, index, mask);
				case 1:
					index = (REG_EMC_NFERR0 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT;
					mask = (REG_EMC_NFERR0 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT;
					rs_correct(databuf, index, mask);
					break;
				default:
					break;
				}
			}
		}

		tmpbuf += ECC_BLOCK;
	}

	return 0;
}

static int nand_load(int offs, int uboot_size, uchar *dst)
{
	int block;
	int blockcopy_count;
	int page;

	__nand_enable();
       
	/*
	 * offs has to be aligned to a block address!
	 */
	block = offs / block_size;
	blockcopy_count = 0;
	
	while (blockcopy_count < (uboot_size / block_size)) {
		for (page = 0; page < page_per_block; page++) {
			
			nand_read_page(block, page, dst, oob_buf);
			
			if (page == 0) {
				if (oob_buf[bad_block_pos] != 0xff) {
					block++;

					/*
					 * Skip bad block
					 */
					continue;
				}
			}

			dst += page_size;

		}

		block++;
		blockcopy_count++;
	}

	__nand_disable();

	return 0;
}

static void gpio_init(void)
{
	/*
	 * Initialize SDRAM pins
	 */
	__gpio_as_sdram_32bit();

	/*
	 * Initialize UART0 pins
	 */
	__gpio_as_uart0();
}

void nand_boot(void)
{
	int boot_sel, ret;
	void (*uboot)(void);
    unsigned int c;
  
	/*
	 * Init hardware
	 */
    #if 0
	#define PIN (32 * 3 + 6)
	__gpio_as_input(PIN);
	__gpio_disable_pull(PIN);
	while(1);
	
	#endif
	gpio_init();
	serial_init();

	serial_puts("\n\nNAND Secondary Program Loader\n\n");

	pll_init();
	sdram_init();
	serial_puts("REG_EMC_DMCR = :");
	serial_puts_hex(REG_EMC_DMCR);
    #if 0
	for(c = 0xa0000000; c < 0xa0000000 + 64 * 1024 *1024; c+=4)
		*(volatile unsigned int *) c = 0;
	for(c = 0xa0000000; c < 0xa0000000 + 64 * 1024 *1024; c+=4)
		if(*(volatile unsigned int *) c != 0)
		{
			serial_puts("sdram 0 error: data = ");
			serial_puts_hex(*(volatile unsigned int *) c);
			serial_puts("addr = ");
			serial_puts_hex(c);

		}
	
	for(c = 0xa0000000; c < 0xa0000000 + 64 * 1024 *1024; c+=4)
		
			*(volatile unsigned int *) c = -1;
		
	
	for(c = 0xa0000000; c < 0xa0000000 + 64 * 1024 *1024; c+=4)
		if(*(volatile unsigned int *) c != -1)
		{
			serial_puts("sdram 1 error: data = ");
			serial_puts_hex(*(volatile unsigned int *) c);
			serial_puts("addr = ");
			serial_puts_hex(c);
	    }
	
	#endif
    #if 0
	for(ret = 0;ret < 16 * 1024 * 1024 ;ret++)
	{
		//serial_puts_hex(ret);
		//serial_puts("\n");	    
		*(volatile unsigned int *)(0xa0000000 + (ret* 4)) = 0xaaaaaaaa;
		c = *(volatile unsigned int *)(0xa0000000 + (ret* 4));
        if(c != 0xaaaaaaaa)
		{
			serial_puts("aa error data:");
			serial_puts_hex((unsigned int)c);
			serial_puts("\n");
		}
		*(volatile unsigned int *)(0xa0000000 + (ret*4)) = 0x55555555;
		c = *(volatile unsigned int *)(0xa0000000 + (ret*4));
        if(c != 0x55555555)
		{
			serial_puts("55 error data:");
			serial_puts_hex((unsigned int)c);
			serial_puts("\n");
		}
		
		
	}
	#endif
	#if 0
	for(ret = 0;ret < 16 * 1024 * 1024 ;ret++)
		*(volatile unsigned int *)(0xa0000000 + (ret* 4)) = ret;

	for(ret = 0;ret < 16 * 1024 * 1024 ;ret++)
	{
		c = *(volatile unsigned int *)(0xa0000000 + (ret*4));
        if(c != ret)
		{
			serial_puts_hex(ret);
			serial_puts(" error data: ");
			serial_puts_hex((unsigned int)c);
			serial_puts("\n");
			//while(1);
  		}
	}
	#endif
		
	#if 0
	serial_puts("\n\nWrite Ram 0x00555555");
    *(volatile unsigned int *)(0xa0555555) = 0xaaaaaaaa;
	serial_puts("\n\nWrite Ram 0x00aaaaaa");
	*(volatile unsigned int *)(0xa0aaaaaa) = 0x55555555;
    serial_puts("\n\nRead Ram 0x00555555");
	ret = *(volatile unsigned int *)(0xa0555555);
    serial_puts_hex(ret);
	serial_puts("\n\nRead Ram 0x00aaaaaa");
	ret = *(volatile unsigned int *)(0xa0aaaaaa);
    serial_puts_hex(ret);
	#endif
	
	/*
	 * Decode boot select
	 */

	boot_sel = REG_EMC_BCR >> 30;

	if (boot_sel == 0x2)
		page_size = 512;
	else
		page_size = 2048;

#if (JZ4740_NANDBOOT_CFG == JZ4740_NANDBOOT_B8R3)
	bus_width = 8;
	row_cycle = 3;
#endif

#if (JZ4740_NANDBOOT_CFG == JZ4740_NANDBOOT_B8R2)
	bus_width = 8;
	row_cycle = 2;
#endif

#if (JZ4740_NANDBOOT_CFG == JZ4740_NANDBOOT_B16R3)
	bus_width = 16;
	row_cycle = 3;
#endif

#if (JZ4740_NANDBOOT_CFG == JZ4740_NANDBOOT_B16R2)
	bus_width = 16;
	row_cycle = 2;
#endif

	page_per_block = (page_size == 2048) ? 64 : 32;
	bad_block_pos = (page_size == 2048) ? 0 : 5;
	oob_size = page_size / 32;
	block_size = page_size * page_per_block;
	ecc_count = page_size / ECC_BLOCK;

	__gpio_as_output(3 * 32 +28);
	__gpio_clear_pin(3 * 32 +28);
	/*
	 * Load U-Boot image from NAND into RAM
	 */
   
	serial_puts("\n\nLoad UCOS image from NAND into RAM\n\n"); 
	
	ret = nand_load(CFG_NAND_UCOS_OFFS, CFG_NAND_UCOS_SIZE,
			(uchar *)CFG_NAND_UCOS_DST);

	uboot = (void (*)(void))CFG_NAND_UCOS_START;

	serial_puts("Starting UCOS ...\n");

	/*
	 * Flush caches
	 */
//	jz_flush_dcache();
//	jz_flush_icache();
	/*
	 * Jump to U-Boot image
	 */
    c = REG_INTC_IPR;
	REG_INTC_IMSR = -1;
	(*uboot)();
}
void serial_puts_hex(unsigned int d)
{
	unsigned char c[12];
	char i;
	for(i = 0; i < 8;i++)
	{
		c[i] = (d >> ((7 - i) * 4)) & 0xf;
		if(c[i] < 10)
			c[i] += 0x30;
		else
			c[i] += (0x41 - 10);
	}
	c[8] = '\n';
	c[9] = 0;
	serial_puts(c);

}
