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
*/
#include <dospart.h>
void dumppartEntry(PARTENTRY* pPart)
{
	printf("\nPARTENTRY:\n");
	printf("Part_FirstHead = %d\n",pPart->Part_FirstHead);       
	printf("Part_FirstSector = %d\n",pPart->Part_FirstSector);       
	printf("Part_FirstTrack = %d\n",pPart->Part_FirstTrack);       
	printf("Part_FileSystem = %d\n",pPart->Part_FileSystem);       
	printf("Part_LastHead = %d\n",pPart->Part_LastHead);       
	printf("Part_LastSector = %d\n",pPart->Part_LastSector);      
	printf("Part_LastTrack = %d\n",pPart->Part_LastTrack);      
	printf("Part_StartSector = %d\n",pPart->Part_StartSector);      
	printf("Part_TotalSectors = %d\n",pPart->Part_TotalSectors);      
 
}

unsigned char GetDOSPartition(unsigned char* sect, unsigned int totalsectt, int ext, PPARTENTRY pPart)
{
	unsigned short sig = (unsigned short)sect[VALIDPARTOFFSET] + 
												((unsigned short)sect[VALIDPARTOFFSET + 1] << 8);
	int i;
	unsigned char ret = INVALID;
	int partCount = MAX_PARTTABLE_ENTRIES;
	PPARTTABLE pTable = (PPARTTABLE)&sect[PARTTABLE_OFFSET];
    PPARTENTRY ptPart = NULL;
    PPARTENTRY pextPart = NULL;
    PARTENTRY mpart;
	ptPart = &mpart;
	//printf("sig = %04x\n",sig);
	if(sig != BOOTSECTRAILSIGH)
		return INVALID;
	if(ext)
		partCount = 2;	

//printf("total: %08x\n", totalsectt);

    for(i = 0; i < partCount;i++)
    {
        memcpy(ptPart,&(pTable->PartEntry[i]),sizeof(PARTENTRY));
		//dumppartEntry(ptPart);
		if (!ptPart->Part_TotalSectors) {
            // skip empty table entries
            continue;            
		}
		if (!ptPart->Part_StartSector)
            break;
        if(ptPart->Part_StartSector > totalsectt)
            break;
/*
        if(ptPart->Part_TotalSectors > totalsectt)
            break;
        if((ptPart->Part_TotalSectors + ptPart->Part_StartSector) > totalsectt)
            break;
*/
        if((ptPart->Part_FileSystem != PART_EXTENDED) && (ptPart->Part_FileSystem != PART_DOSX13X))
		{
			memcpy(pPart ,ptPart,sizeof(PARTENTRY));
			return ptPart->Part_FileSystem;
		}
        else
		{	
			if(ext)
			{
                pextPart = &(pTable->PartEntry[i]);
                continue;
			}
			memcpy(pPart ,ptPart,sizeof(PARTENTRY));
			return PART_EXTENDED;        
		}
	}

	if((ext)&&(pextPart))
	{
		memcpy(pPart ,pextPart,sizeof(PARTENTRY));
		return PART_EXTENDED;
	}	
//printf("ret here\n");
    return ret;
}
 
