/*  JZ4740 PAVO and LYRA board AUO_A030FL01_V1 lcd driver.
 *
 *  Copyright (C) 2006 - 2007 Ingenic Semiconductor Inc.
 *
 *  Author: <zyliu@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */



#if LCDTYPE == 4
#include <jz4740.h>

#define GPIO_DISP_OFF_N   118
/* board pavo */
//#define SPEN	(32*1+18)       //LCD_CS
//#define SPCK	(32*1+17)       //LCD_SCL
//#define SPDA	(32*2+12)       //LCD_SDA
//#define LCD_RET (32*2+23)       //use for lcd reset

/*board lyra */
#define SPEN	(32*3+19)       //LCD_CS
#define SPCK	(32*3+18)       //LCD_SCL
#define SPDA	(32*3+20)       //LCD_SDA
#define LCD_RET (32*3+31)       //use for lcd reset

#define GPIO_PWM    123		/* GP_D27 */

#define __spi_write_reg(reg, val)		\
	do {					\
		unsigned char no;		\
		unsigned short value;		\
		unsigned char a=0;		\
		unsigned char b=0;		\
		__gpio_as_output(SPEN); /* use SPDA */	\
		__gpio_as_output(SPCK); /* use SPCK */	\
		__gpio_as_output(SPDA); /* use SPDA */	\
		a=reg;				\
		b=val;				\
		__gpio_set_pin(SPEN);		\
		__gpio_clear_pin(SPCK);		\
		udelay(50);			\
		__gpio_clear_pin(SPDA);		\
		__gpio_clear_pin(SPEN);		\
		udelay(50);			\
		value=((a<<8)|(b&0xFF));	\
		for(no=0;no<16;no++)		\
		{				\
			if((value&0x8000)==0x8000){	      \
				__gpio_set_pin(SPDA);}	      \
			else{				      \
				__gpio_clear_pin(SPDA); }     \
			udelay(400);			\
			__gpio_set_pin(SPCK);		\
			value=(value<<1);		\
			udelay(50);			\
			__gpio_clear_pin(SPCK);		\
		}					\
		__gpio_set_pin(SPEN);			\
		udelay(400);				\
	} while (0)
#define __spi_read_reg(reg,val)			\
	do{					\
		unsigned char no;		\
		unsigned short value;			\
		__gpio_as_output(SPEN); /* use SPDA */	\
		__gpio_as_output(SPCK); /* use SPCK */	\
		__gpio_as_output(SPDA); /* use SPDA */	\
		value = ((reg << 0) | (1 << 7));	\
		val = 0;				\
		__gpio_as_output(SPDA);			\
		__gpio_set_pin(SPEN);			\
		__gpio_set_pin(SPCK);			\
		udelay(1);				\
		__gpio_clear_pin(SPDA);			\
		__gpio_clear_pin(SPEN);			\
		udelay(1);				\
		for (no = 0; no < 16; no++ ) {		\
			__gpio_clear_pin(SPCK);		\
			udelay(1);			\
			if(no < 8)			\
			{						\
				if (value & 0x80) /* send data */	\
					__gpio_set_pin(SPDA);		\
				else					\
					__gpio_clear_pin(SPDA);		\
				value = (value << 1);			\
				udelay(1);				\
				__gpio_set_pin(SPCK);			\
				udelay(1);				\
			}						\
			else						\
			{						\
				__gpio_as_input(SPDA);			\
				udelay(1);				\
				__gpio_set_pin(SPCK);			\
				udelay(1);				\
				val = (val << 1);			\
				val |= __gpio_get_pin(SPDA);		\
				udelay(1);				\
			}						\
			udelay(400);					\
		}							\
		__gpio_as_output(SPDA);					\
		__gpio_set_pin(SPEN);					\
		udelay(400);						\
	} while(0)
	
#define __lcd_special_pin_init()		\
	do {						\
		__gpio_as_output(SPEN); /* use SPDA */	\
		__gpio_as_output(SPCK); /* use SPCK */	\
		__gpio_as_output(SPDA); /* use SPDA */	\
		__gpio_as_output(LCD_RET);		\
		udelay(50);				\
		__gpio_clear_pin(LCD_RET);		\
		udelay(100);				\
		__gpio_set_pin(LCD_RET);		\
	} while (0)
#define __lcd_special_on()			     \
	do {					     \
		udelay(50);			     \
		__gpio_clear_pin(LCD_RET);	     \
		udelay(100);			     \
		__gpio_set_pin(LCD_RET);	     \
		__spi_write_reg(0x0D, 0x44);	     \
		__spi_write_reg(0x0D, 0x4D);	     \
		__spi_write_reg(0x0B, 0x06);	     \
		__spi_write_reg(0x40, 0xC0);	     \
		__spi_write_reg(0x42, 0x43);	     \
		__spi_write_reg(0x44, 0x28);	     \
		__spi_write_reg(0x0D, 0x4F);	     \
} while (0)

#define __lcd_special_off()			\
	do {					\
		__spi_write_reg(0x04, 0x4C);	\
	} while (0)

#define GPIO_PWM    123		/* GP_D27 */
#define PWM_CHN 4    /* pwm channel */
#define PWM_FULL 101
/* 100 level: 0,1,...,100 */
#define __lcd_set_backlight_level(n)                     \
do {                                                     \
	__gpio_as_pwm(4); \
        __tcu_disable_pwm_output(PWM_CHN);               \
        __tcu_stop_counter(PWM_CHN);                     \
        __tcu_init_pwm_output_high(PWM_CHN);             \
        __tcu_set_pwm_output_shutdown_abrupt(PWM_CHN);   \
        __tcu_select_clk_div1(PWM_CHN);                  \
        __tcu_mask_full_match_irq(PWM_CHN);              \
        __tcu_mask_half_match_irq(PWM_CHN);              \
        __tcu_set_count(PWM_CHN,0);                      \
        __tcu_set_full_data(PWM_CHN,__cpm_get_extalclk()/200);           \
        __tcu_set_half_data(PWM_CHN,__cpm_get_extalclk()/200*n/100);     \
        __tcu_enable_pwm_output(PWM_CHN);                \
        __tcu_select_extalclk(PWM_CHN);                  \
        __tcu_start_counter(PWM_CHN);                    \
} while (0)

/*
#define __lcd_set_backlight_level(n)                     \
do { \
__gpio_as_output(GPIO_PWM); \
__gpio_set_pin(GPIO_PWM); \
} while (0)
*/

#define __lcd_close_backlight() \
do { \
__gpio_as_output(GPIO_PWM); \
__gpio_clear_pin(GPIO_PWM); \
} while (0)
#define __lcd_display_pin_init() \
do { \
	__gpio_as_output(GPIO_DISP_OFF_N); \
	__cpm_start_tcu(); \
	__lcd_special_pin_init(); \
} while (0)

#define __lcd_display_on() \
do { \
	__gpio_set_pin(GPIO_DISP_OFF_N); \
	__lcd_set_backlight_level(80); \
	__lcd_special_on(); \
} while (0)

#define __lcd_display_off() \
do { \
	__lcd_special_off(); \
	__lcd_close_backlight(); \
	__gpio_clear_pin(GPIO_DISP_OFF_N); \
} while (0)

void  lcd_set_backlight(int level)
{
	__lcd_set_backlight_level(level); 
}

void lcd_board_init(void)
{
	__lcd_display_pin_init();
	__lcd_display_on();
//	__lcd_display_off();
}
void  lcd_close_backlight()
{
	__lcd_close_backlight();
}
#endif
