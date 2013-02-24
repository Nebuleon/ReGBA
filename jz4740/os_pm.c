/*
 * linux/arch/mips/jz4740/common/pm.c
 * 
 * JZ4740 Power Management Routines
 * 
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 * Author: <jlwei@ingenic.cn>
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 */


#if (DM==1)
#include "jz4740.h"
#include "includes.h"

static struct pll_opt
{
	unsigned int cpuclock;
	int div;
};
static struct pll_opt opt_pll[4];
static int currentlevel;
void  StatHookInit (void)
{
	unsigned int pllout;
	opt_pll[0].cpuclock=96000000;
	opt_pll[0].div=3;
	opt_pll[1].cpuclock=192000000;
	opt_pll[1].div=2;
	opt_pll[2].cpuclock=288000000;
	opt_pll[2].div=3;
	opt_pll[3].cpuclock=384000000;
	opt_pll[3].div=3;

	pllout = (__cpm_get_pllm() + 2)* EXTAL_CLK / (__cpm_get_plln() + 2);
	if(pllout<96000000)
	{
		currentlevel=0;
	}
	if(pllout>96000000 && pllout<192000000 )
	{
		currentlevel=1;
	}
	if(pllout>192000000 && pllout<288000000 )
	{
		currentlevel=2;
	}
	if(pllout>288000000)
	{
		currentlevel=3;
	}

}
int count_sec=0;

void   OSTaskStatHook_jz4740 (void)
{
#if 0
	count_sec=count_sec+1;
	if(count_sec>10)
	{
	printf("hook22222 %d\n",OSCPUUsage);
		if(OSCPUUsage > 76)
		{
			if(currentlevel<3)
			{
				currentlevel=currentlevel+1;
				jz_pm_pllconvert(opt_pll[currentlevel].cpuclock,opt_pll[currentlevel].div);
			}
		}
		if(OSCPUUsage < 70)
		{
			if(currentlevel>0)
			{
				currentlevel=currentlevel-1;
				jz_pm_pllconvert(opt_pll[currentlevel].cpuclock,opt_pll[currentlevel].div);
			}
		}
		count_sec=0;
	}
#endif

}

OSTaskIdleHook_jz4740()
{
	jz_pm_idle();
}
#endif
