/*
 *  JZ4740  dospart  Project  V1.0.0
 *  Copyright (C) 2006 - 2007 Ingenic Semiconductor Inc.
 *  Author: <dsqiu@ingenic.cn>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  2007-12-06, dsqiu
 *  Created.
 *
*
*/
#ifndef __DOSPART_H__
#define __DOSPART_H__
#include "fs_api.h"
#define UNALIGNED  __attribute__ ((packed))

typedef struct _PARTENTRY {
        unsigned char   Part_BootInd;           // If 80h means this is boot partition
        unsigned char   Part_FirstHead;         // Partition starting head based 0
        unsigned char   Part_FirstSector;       // Partition starting sector based 1
        unsigned char   Part_FirstTrack;        // Partition starting track based 0
        unsigned char   Part_FileSystem;        // Partition type signature field
        unsigned char   Part_LastHead;          // Partition ending head based 0
        unsigned char   Part_LastSector;        // Partition ending sector based 1
        unsigned char   Part_LastTrack;         // Partition ending track based 0
        unsigned int    Part_StartSector;       // Physical starting sector based 0
        unsigned int    Part_TotalSectors;      // Total physical sectors in partition
} PARTENTRY;
typedef PARTENTRY UNALIGNED *PPARTENTRY;
#define BOOTSECTRAILSIGH 0xaa55
#define VALIDPARTOFFSET (512 - 2)




// Part_FileSystem

#define PART_UNKNOWN            0
#define PART_DOS2_FAT           0x01    // legit DOS partition
#define PART_DOS3_FAT           0x04    // legit DOS partition
#define PART_EXTENDED           0x05    // legit DOS partition
#define PART_DOS4_FAT           0x06    // legit DOS partition
#define PART_DOS32              0x0B    // legit DOS partition (FAT32)
#define PART_DOS32X13           0x0C    // Same as 0x0B only "use LBA"
#define PART_DOSX13             0x0E    // Same as 0x06 only "use LBA"
#define PART_DOSX13X            0x0F    // Same as 0x05 only "use LBA"


// Legacy definitions for Part_FileSystem

#define PRIMARY                 0x00
#define NEC_98_INVALID          0x00
#define DOS12                   PART_DOS2_FAT   /* MS-DOS 12-bit FAT */
#define XENIX1                  0x02
#define XENIX2                  0x03
#define DOS16                   PART_DOS3_FAT   /* MS-DOS original 16-bit FAT */
#define EXTENDED                PART_EXTENDED   /* extended partition */
#define LOGICAL                 PART_EXTENDED
#define DOSNEW                  PART_DOS4_FAT   /* MS-DOS 16-bit FAT > 32Meg partition */
#define HPFS                    0x07            /* Also used by NTFS OFS and other IFS file systems */
#define PARTID_IFS              HPFS
#define COMMODOREid             0x08
#define BM_PART                 0x0A            /* OS/2 Boot Manager (BM) partition */
#define DOS32                   PART_DOS32      /* MS-DOS (32bit FAT) */
#define DOS32X13                PART_DOS32X13   /* MS-DOS (32bit FAT EXTENDED) */
#define DOSX32                  DOS32X13
#define DOSX13                  PART_DOSX13     /* Ext INT 13 drive comment out this line to turn off X13 sup. */
#define DOS1024                 DOSX13
#define DOSX13X                 PART_DOSX13X    /* EXTENDED partition XI13 */
#define DOSX1024                DOSX13X

#define NEC_98_DOS16            0x11            /* NEC MS-DOS (16bit FAT) */

#define BM_FAT12_PART           0x11            /* OS/2 Boot Manager (BM) partition */
#define L_EDGEid                0x11
#define BM_FAT16_PART           0x14            /* OS/2 Boot Manager (BM) partition */
#define ASTid                   0x14
#define BM_NEW_PART             0x16            /* OS/2 Boot Manager (BM) partition */
#define BM_HPFS_PART            0x17            /* OS/2 Boot Manager (BM) partition */


#define NEC_98_DOSNEW           0x21            /* NEC MS-DOS (large partition) */


#define OTHR3DPARTYI            0x23
#define NECid                   0x24
#define OTHR3DPARTYK            0x26
#define OTHR3DPARTYL            0x31
#define OTHR3DPARTYM            0x33
#define OTHR3DPARTYN            0x34
#define OTHR3DPARTYO            0x36
#define IBM_PPC_PREP            0x41        /* IBM PowerPC prep partition */
#define VERITASVXVM1            0x42        /* Veritas Volume Manager 1 partition */
#define VERITASVXVM2            0x43        /* Veritas Volume Manager 2 partition */
#define DM_DRIVERid1            0x50
#define DM_DRIVERid2            0x51
#define ATNTid                  0x56
#define OTHR3DPARTY1            0x61
#define OTHR3DPARTY2            0x63
#define NOVELL                  0x64
#define OTHR3DPARTY4            0x66
#define OTHR3DPARTY5            0x71
#define OTHR3DPARTY6            0x73
#define OTHR3DPARTY7            0x74
#define PCIX                    0x75
#define OTHR3DPARTY8            0x76
#define PLAN9                   0x9B            /* Bell Labs (Dennis Richie!!!) */
#define OTHR3DPARTYP            0xA1
#define OTHR3DPARTYQ            0xA3
#define OTHR3DPARTYR            0xA4
#define OTHR3DPARTYS            0xA6
#define OTHR3DPARTYT            0xB1
#define OTHR3DPARTYU            0xB3
#define OTHR3DPARTYV            0xB4
#define OTHR3DPARTYW            0xB6
#define PP_FAT12_PART           0xC1            /* DR-DOS Password Protected */
#define PP_FAT16_PART           0xC4            /* DR-DOS Password Protected */
#define PP_EXT_PART             0xC5            /* DR-DOS Password Protected */
#define PP_NEW_PART             0xC6            /* DR-DOS Password Protected */
#define CPM                     0xDB
#define OTHR3DPARTY9            0xE1
#define OTHR3DPARTYA            0xE3
#define OTHR3DPARTYB            0xE4
#define TANDYid                 0xE5
#define OTHR3DPARTYC            0xE6
#define OTHR3DPARTYD            0xF1
#define UNISYSid                0xF2
#define OTHR3DPARTYE            0xF3
#define OTHR3DPARTYF            0xF4
#define OTHR3DPARTYG            0xF6
#define INVALID                 0xFF            /* invalid or not used */
#define MAX_PARTTABLE_ENTRIES   4
#define SIZE_PARTTABLE_ENTRIES  16
#define DEFAULT_SECTOR_SIZE     512

#define PARTTABLE_OFFSET        (DEFAULT_SECTOR_SIZE - 2 - (SIZE_PARTTABLE_ENTRIES * MAX_PARTTABLE_ENTRIES))

typedef struct _PARTTABLE {
        PARTENTRY       PartEntry[MAX_PARTTABLE_ENTRIES];
} PARTTABLE;
typedef PARTTABLE UNALIGNED *PPARTTABLE;
#endif //__DOS_PART_H__
