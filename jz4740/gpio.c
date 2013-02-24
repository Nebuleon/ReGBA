/*
 * gpio.c
 *
 * Init GPIO pins for PMP board.
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
                           
                            
void gpio_init(void)        
{       
#if USE_UART0             
	__gpio_as_uart0();
#endif                    
#if USE_UART1
//	__gpio_as_uart1();
#endif
#if USE_NAND
	__gpio_as_nand();
#endif
  	
#if USE_AIC
//	__gpio_as_aic();
#endif
#if USE_CIM
//	__gpio_as_cim();
#endif
	
	
	///* First PW_I output high */
	//__gpio_as_output(GPIO_PW_I);
	//__gpio_set_pin(GPIO_PW_I);
  //
	///* Then PW_O output high */
	//__gpio_as_output(GPIO_PW_O);
	//__gpio_set_pin(GPIO_PW_O);
  //
	///* Last PW_I output low and as input */
	//__gpio_clear_pin(GPIO_PW_I);
	//__gpio_as_input(GPIO_PW_I);
  //
	///* USB clock enable */
	//__gpio_as_output(GPIO_USB_CLK_EN);
	//__gpio_set_pin(GPIO_USB_CLK_EN);
  //
	///* LED enable */
	//__gpio_as_output(GPIO_LED_EN);
	//__gpio_set_pin(GPIO_LED_EN);
  //
	///* LCD display off */
	//__gpio_as_output(GPIO_DISP_OFF_N);
	//__gpio_clear_pin(GPIO_DISP_OFF_N);
  //
	///* No backlight */
	//__gpio_as_output(94); /* PWM0 */
	//__gpio_clear_pin(94);
  //
	///* RTC IRQ input */
	//__gpio_as_input(GPIO_RTC_IRQ);
  //
	///* make PW_I work properly */
	//__gpio_disable_pullupdown(GPIO_PW_I);
}

