/*
 * JZ4740 Driver Management Routines
 * Copyright (C) 2007 Ingenic Semiconductor Inc.
 * Author: <jgao@ingenic.cn>
 */

#include <jz4740.h>
#include <dm.h>
#if(DM==1)
#define driver_number 10
struct dm_jz4740_t dm[driver_number];
int driver_count;
int jz_dm_init(void)
{
	memset(dm, 0, driver_number*sizeof(struct dm_jz4740_t));
	driver_count=0;
#if(LCDTYPE==1)
	mng_init_lcd();
#endif
//      mng_init_mmc();
#if(CODECTYPE==1)
	mng_init_codec();
#endif
	return 0;
}

int dm_register(int dev_id, struct dm_jz4740_t  *dm_in)
{
	unsigned long flags;
	dm[driver_count].name= dm_in->name; 
	dm[driver_count].init = dm_in->init;  
	dm[driver_count].poweron = dm_in->poweron;
	dm[driver_count].poweroff = dm_in->poweroff;
	dm[driver_count].convert = dm_in->convert;
	dm[driver_count].preconvert = dm_in->preconvert;
	driver_count++;
	return 0;
}

int dm_all_convert(void)
{
	int i;
//	printf("all convert!\n");
	for(i=0;i<driver_count;i++)
	{
		dm[i].convert();
	}
	return 0;
}

int dm_pre_convert(void)
{
	int i,ret;
//	printf("pre convert!\n");
	for(i=0;i<driver_count;i++)
	{
		ret=dm[i].preconvert();
		if(ret==0)
		{
			return 1;
		}
	}
	return 0;
}
#endif
