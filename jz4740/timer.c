/*
 * timer.c
 *
 * Perform the system ticks.
 *
 * Author: Seeger Chin
 * e-mail: seeger.chin@gmail.com
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <bsp.h>
#include <jz4740.h>

#define USE_RTC_CLOCK 0
void timerHander(unsigned int arg)
{
	OSTimeTick();
}

void JZ_StartTicker(unsigned int tps)
{
	unsigned int latch;
	__cpm_start_tcu();
	
	__tcu_disable_pwm_output(0);
	__tcu_mask_half_match_irq(0); 
	__tcu_unmask_full_match_irq(0);

#if USE_RTC_CLOCK
	__tcu_select_rtcclk(0);
	__tcu_select_clk_div1(0);
    latch = (__cpm_get_rtcclk() + (tps>>1)) / tps;
	
#else	
	__tcu_select_extalclk(0);
	__tcu_select_clk_div4(0);
	
	latch = (EXTAL_CLK / 4 + (tps>>1)) / tps;
#endif
	REG_TCU_TDFR(0) = latch;
	REG_TCU_TDHR(0) = latch;
	__tcu_set_count(0,0);
	__tcu_clear_full_match_flag(0);
	__tcu_start_counter(0);
	
    dgprintf("TCSR = 0x%04x\r\n",*(volatile u16 *)0xb000204C);
	
	request_irq(IRQ_TCU0,timerHander,0);

}

void JZ_StopTicker(void)
{
	__tcu_stop_counter(0);
}


static volatile unsigned int jz_timer_h = 0; 
void JZ_timerHander(unsigned int arg)
{
  jz_timer_h++; 
	__tcu_clear_full_match_flag(1);
}

void JZ_StartTimer()
{
	cli();
	__tcu_disable_pwm_output(1);
	__tcu_select_extalclk(1);
	__tcu_select_clk_div4(1);
	
	__tcu_mask_half_match_irq(1); 
	__tcu_unmask_full_match_irq(1);

	REG_TCU_TDFR(1) = 60000;
	REG_TCU_TDHR(1) = -1;
	__tcu_set_count(1,0);
	__tcu_clear_full_match_flag(1);
	__tcu_start_counter(1);
	jz_timer_h = 0;
	request_irq(IRQ_TCU1,JZ_timerHander,0);
  sti();
	
}

void JZ_StopTimer(void)
{
	__tcu_stop_counter(1);
	__tcu_clear_full_match_flag(1);
}
unsigned int *JZ_DiffTimer(unsigned int *tm3, unsigned int *tm1,unsigned int *tm2)
{
	unsigned int d1,d2,d3,d4;
	d1 = *tm1;
	d2 = *tm2;
	d3 = *(tm1 + 1);
	d4 = *(tm2 + 1);
	if(d1 > d2)
	{
		*tm3 = d1 - d2;
		*(tm3 + 1) = d3 - d4;
		return tm1;
	}else
	{
		*tm3 = 1000000 + d1 - d2;
		*(tm3 + 1) = d3 - d4 - 1;
		return tm1;
	}
	return 0;

}
void JZ_GetTimer_20ms(unsigned int *tm)
{
	unsigned int dh,dl,iflag1,iflag2;
	unsigned int savesr;
	savesr = spin_lock_irqsave();
	dh = jz_timer_h;
	do
	{
		iflag1 =__tcu_full_match_flag(1);
		dl = __tcu_get_count(1);
		iflag2 = __tcu_full_match_flag(1);
	}while(iflag1 != iflag2);	
	if(iflag1)
		dh++;
	spin_unlock_irqrestore(savesr);
	tm[1] = dh;
	tm[0] = dl / (EXTAL_CLK / 1000000 / 4);
}
void JZ_GetTimer(unsigned int *tm)
{
	
  JZ_GetTimer_20ms(tm);       
	tm[0] /= (EXTAL_CLK / 1000000 / 4);
  tm[0] += (tm[1] % 50) * 20000;
}
unsigned int JZ_GetSecTime()
{
	return jz_timer_h / 50;
}
unsigned int JZ_GetuSecClock()
{
	unsigned int dh,dl;
	dh = jz_timer_h;
	dl = __tcu_get_count(1);
	//if(dl == -1)
	{
		if(dh != jz_timer_h)
		{
      dh = jz_timer_h;
			dl = __tcu_get_count(1);
		}
	}
	dl /= (EXTAL_CLK / 1000000 / 4);
  dl += (dh % 50) * 20000; 
	return dl;
}

//#define TEST_TIME
#ifdef TEST_TIME
void JZ_timerHander_test(unsigned int arg)
{
	__tcu_clear_full_match_flag(arg);
	printf("arg = %d!\n",arg);
	
}
void JZ_StartTimer_test(unsigned int d)
{
	cli();
	__tcu_disable_pwm_output(d);
	__tcu_select_extalclk(d);
	__tcu_select_clk_div4(d);
	
	__tcu_mask_half_match_irq(d); 
	__tcu_unmask_full_match_irq(d);

	REG_TCU_TDFR(d) = 60000;
	REG_TCU_TDHR(d) = 60000;

	__tcu_clear_full_match_flag(d);
	__tcu_start_counter(d);
	request_irq(IRQ_TCU2,JZ_timerHander_test,d);
  sti();
	
}

#endif