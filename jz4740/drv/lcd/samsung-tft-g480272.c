#if LCDTYPE == 1
#include <jz4740.h>

#define DISPOFF	(3 * 32 + 22)
#define LCD_RESET (2*32 + 23)
/*
#ifdef PMPVERSION == 1
#define LCD_RESET 63
#endif

#ifdef PMPVERSION == 22
#define LCD_RESET 60
#endif
*/

void lcd_board_init(void)
{
	u32 val;
	__gpio_as_output(DISPOFF);
	__gpio_set_pin(LCD_RESET);
	__gpio_as_output(LCD_RESET);
	__gpio_clear_pin(LCD_RESET);
	udelay(1000);
	__gpio_set_pin(LCD_RESET);

	
	__gpio_as_pwm4();
	__tcu_disable_pwm_output(4);
	__tcu_init_pwm_output_high(4);
	__tcu_select_clk_div1(4);

	__tcu_mask_half_match_irq(4);
	__tcu_mask_full_match_irq(4);

	val = __cpm_get_extalclk();
	val /= 900;

	__tcu_set_full_data(4,val/2);
	__tcu_set_half_data(4,val);
	__tcu_start_counter(4);
	__tcu_enable_pwm_output(4);
	__gpio_set_pin(DISPOFF);
}
#define GPIO_PWM    123		/* GP_D27 */
#define PWM_CHN 4    /* pwm channel */
#define PWM_FULL 101
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
        __tcu_set_full_data(PWM_CHN,__cpm_get_extalclk()/1000);           \
        __tcu_set_half_data(PWM_CHN,__cpm_get_extalclk()/1000*n/100);     \
        __tcu_enable_pwm_output(PWM_CHN);                \
        __tcu_select_extalclk(PWM_CHN);                  \
        __tcu_start_counter(PWM_CHN);                    \
} while (0)

#define __lcd_close_backlight() \
do { \
__gpio_as_output(GPIO_PWM); \
__gpio_clear_pin(GPIO_PWM); \
} while (0)

void  lcd_set_backlight(int level)
{
	__lcd_set_backlight_level(level); 
}
void  lcd_close_backlight()
{
	__lcd_close_backlight();
}


#endif
