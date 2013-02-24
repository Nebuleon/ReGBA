/*
 *  linux/include/linux/mtd/nand.h
 *
 *  Copyright (c) 2000 David Woodhouse <dwmw2@mvhi.com>
 *		       Steven J. Hill <sjhill@realitydiluted.com>
 *		       Thomas Gleixner <tglx@linutronix.de>
 *
 * $Id: nand.h,v 1.1.1.1 2007/07/30 05:29:19 dsqiu Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Info:
 *   Contains standard defines and IDs for NAND flash devices
 *
 *  Changelog:
 *   01-31-2000 DMW	Created
 *   09-18-2000 SJH	Moved structure out of the Disk-On-Chip drivers
 *			so it can be used by other NAND flash device
 *			drivers. I also changed the copyright since none
 *			of the original contents of this file are specific
 *			to DoC devices. David can whack me with a baseball
 *			bat later if I did something naughty.
 *   10-11-2000 SJH	Added private NAND flash structure for driver
 *   10-24-2000 SJH	Added prototype for 'nand_scan' function
 *   10-29-2001 TG	changed nand_chip structure to support
 *			hardwarespecific function for accessing control lines
 *   02-21-2002 TG	added support for different read/write adress and
 *			ready/busy line access function
 *   02-26-2002 TG	added chip_delay to nand_chip structure to optimize
 *			command delay times for different chips
 *   04-28-2002 TG	OOB config defines moved from nand.c to avoid duplicate
 *			defines in jffs2/wbuf.c
 *   08-07-2002 TG	forced bad block location to byte 5 of OOB, even if
 *			CONFIG_MTD_NAND_ECC_JFFS2 is not set
 *   08-10-2002 TG	extensions to nand_chip structure to support HW-ECC
 *
 *   08-29-2002 tglx	nand_chip structure: data_poi for selecting
 *			internal / fs-driver buffer
 *			support for 6byte/512byte hardware ECC
 *			read_ecc, write_ecc extended for different oob-layout
 *			oob layout selections: NAND_NONE_OOB, NAND_JFFS2_OOB,
 *			NAND_YAFFS_OOB
 *  11-25-2002 tglx	Added Manufacturer code FUJITSU, NATIONAL
 *			Split manufacturer and device ID structures
 *
 *  02-08-2004 tglx	added option field to nand structure for chip anomalities
 *  05-25-2004 tglx	added bad block table support, ST-MICRO manufacturer id
 *			update of nand_chip structure description
 */
#ifndef __LINUX_MTD_NAND_H
#define __LINUX_MTD_NAND_H



/* This constant declares the max. oobsize / page, which
 * is supported now. If you add a chip with bigger oobsize/page
 * adjust this accordingly.
 */
#define NAND_MAX_OOBSIZE	64

/*
 * Constants for hardware specific CLE/ALE/NCE function
*/
/* Select the chip by setting nCE to low */
#define NAND_CTL_SETNCE		1
/* Deselect the chip by setting nCE to high */
#define NAND_CTL_CLRNCE		2
/* Select the command latch by setting CLE to high */
#define NAND_CTL_SETCLE		3
/* Deselect the command latch by setting CLE to low */
#define NAND_CTL_CLRCLE		4
/* Select the address latch by setting ALE to high */
#define NAND_CTL_SETALE		5
/* Deselect the address latch by setting ALE to low */
#define NAND_CTL_CLRALE		6
/* Set write protection by setting WP to high. Not used! */
#define NAND_CTL_SETWP		7
/* Clear write protection by setting WP to low. Not used! */
#define NAND_CTL_CLRWP		8

/*
 * Standard NAND flash commands
 */
#define NAND_CMD_READ0		0
#define NAND_CMD_READ1		1
#define NAND_CMD_PAGEPROG	0x10
#define NAND_CMD_READOOB	0x50
#define NAND_CMD_ERASE1		0x60
#define NAND_CMD_STATUS		0x70
#define NAND_CMD_STATUS_MULTI	0x71
#define NAND_CMD_SEQIN		0x80
#define NAND_CMD_READID		0x90
#define NAND_CMD_ERASE2		0xd0
#define NAND_CMD_RESET		0xff

/* Extended commands for large page devices */
#define NAND_CMD_READSTART	0x30
#define NAND_CMD_CACHEDPROG	0x15

/* Status bits */
#define NAND_STATUS_FAIL	0x01
#define NAND_STATUS_FAIL_N1	0x02
#define NAND_STATUS_TRUE_READY	0x20
#define NAND_STATUS_READY	0x40
#define NAND_STATUS_WP		0x80

/*
 * Constants for ECC_MODES
 */

/* No ECC. Usage is not recommended ! */
#define NAND_ECC_NONE		0
/* Software ECC 3 byte ECC per 256 Byte data */
#define NAND_ECC_SOFT		1
/* Hardware ECC 3 byte ECC per 256 Byte data */
#define NAND_ECC_HW3_256	2
/* Hardware ECC 3 byte ECC per 512 Byte data */
#define NAND_ECC_HW3_512	3
/* Hardware ECC 3 byte ECC per 512 Byte data */
#define NAND_ECC_HW6_512	4
/* Hardware ECC 8 byte ECC per 512 Byte data */
#define NAND_ECC_HW8_512	6
/* Hardware ECC 12 byte ECC per 2048 Byte data */
#define NAND_ECC_HW12_2048	7

/*
 * Constants for Hardware ECC
*/
/* Reset Hardware ECC for read */
#define NAND_ECC_READ		0
/* Reset Hardware ECC for write */
#define NAND_ECC_WRITE		1
/* Enable Hardware ECC before syndrom is read back from flash */
#define NAND_ECC_READSYN	2

/* Option constants for bizarre disfunctionality and real
*  features
*/
/* Chip can not auto increment pages */
#define NAND_NO_AUTOINCR	0x00000001
/* Buswitdh is 16 bit */
#define NAND_BUSWIDTH_16	0x00000002
/* Device supports partial programming without padding */
#define NAND_NO_PADDING		0x00000004
/* Chip has cache program function */
#define NAND_CACHEPRG		0x00000008
/* Chip has copy back function */
#define NAND_COPYBACK		0x00000010
/* AND Chip which has 4 banks and a confusing page / block
 * assignment. See Renesas datasheet for further information */
#define NAND_IS_AND		0x00000020
/* Chip has a array of 4 pages which can be read without
 * additional ready /busy waits */
#define NAND_4PAGE_ARRAY	0x00000040

/* Options valid for Samsung large page devices */
#define NAND_SAMSUNG_LP_OPTIONS \
	(NAND_NO_PADDING | NAND_CACHEPRG | NAND_COPYBACK)

/* Macros to identify the above */

#endif /* __LINUX_MTD_NAND_H */
