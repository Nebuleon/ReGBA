/*
 * (C) Copyright 2006
 * Ingenic Semiconductor, <jlwei@ingenic.cn>
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

/*
 * This file contains the configuration parameters for the virgo board.
 */

#ifndef __CONFIG_H
#define __CONFIG_H


#define JZ4740_NANDBOOT_CFG	JZ4740_NANDBOOT_B8R3	/* NAND Boot config code */

#define CFG_NAND_UCOS_OFFS   (256 * 1024)    
#define CFG_NAND_UCOS_START  (0x80100000)
#define CFG_NAND_UCOS_DST    (0x80100000)
#define CFG_NAND_UCOS_SIZE   ( 2 * 1024 *1024)


//#define CFG_CPU_SPEED		336000000	/* CPU clock: 336 MHz */
#define CFG_CPU_SPEED		144000000	/* CPU clock: 336 MHz */
#define CFG_EXTAL		12000000	/* EXTAL freq: 12 MHz */
#define	CFG_HZ			(CFG_EXTAL/256) /* incrementer freq */

#define CFG_UART_BASE  		UART0_BASE	/* Base of the UART channel */

#define CONFIG_BAUDRATE		57600
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }


/*-----------------------------------------------------------------------
 * NAND Info.
 */
//-----------------------------------------------------------------------

#define NAND_BLOCK_SIZE  (256 * 1024)
#define NAND_PAGE_PER_BLOCK (128)
//#define NAND_BLOCK_SIZE  (128 * 1024)
//#define NAND_PAGE_PER_BLOCK (64)

/*-----------------------------------------------------------------------
 * SDRAM Info.
 */
#define CONFIG_NR_DRAM_BANKS	1

// SDRAM paramters
#define SDRAM_BW16		1	/* Data bus width: 0-32bit, 1-16bit */
#define SDRAM_BANK4		1	/* Banks each chip: 0-2bank, 1-4bank */
#define SDRAM_ROW		13	/* Row address: 11 to 13 */
#define SDRAM_COL		9	/* Column address: 8 to 12 */
#define SDRAM_CASL		2	/* CAS latency: 2 or 3 */

// SDRAM Timings, unit: ns
#define SDRAM_TRAS		45	/* RAS# Active Time */
#define SDRAM_RCD		20	/* RAS# to CAS# Delay */
#define SDRAM_TPC		20	/* RAS# Precharge Time */
#define SDRAM_TRWL		7	/* Write Latency Time */
#define SDRAM_TREF	        7812	/* Refresh period: 4096 refresh cycles/64ms */
//#define SDRAM_TREF	        15625	/* Refresh period: 4096 refresh cycles/64ms */

/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CFG_DCACHE_SIZE		16384
#define CFG_ICACHE_SIZE		16384
#define CFG_CACHELINE_SIZE	32

/*-----------------------------------------------------------------------
 * GPIO definition
 */

#define GPIO_SD_VCC_EN_N	115 /* GPD19 */	
#define GPIO_SD_CD_N		116 /* GPD20 */

#define GPIO_USB_DETE		114 /* GPD18 */
#define GPIO_DC_DETE_N		120 /* GPD24 */

#define GPIO_DISP_OFF_N		118 /* GPD22 */
//#define GPIO_CHARG_STAT_N	121 /* GPD25 */
//#define GPIO_SD_WP	        110 /* GPD17 */
#endif	/* __CONFIG_H */
