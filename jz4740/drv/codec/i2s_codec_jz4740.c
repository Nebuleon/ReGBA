#include <jz4740.h>

#define u8	unsigned char
#define u16	unsigned short
#define u32	unsigned int

u16 codec_volume;
u16 codec_base_gain;
u16 codec_mic_gain;
int HP_on_off_flag; 
static int HP_register_value; 
//int IS_WRITE_PCM;

#define SLOW_ANTI_POP 1
//#define FAST_ANTI_POP 1
//#define NEW_ANTI_POP 1

void i2s_codec_clear(void)
{
        REG_ICDC_CDCCR1 = 0x021b2302;
}

void i2s_codec_reset(void)
{
    REG_ICDC_CDCCR1 |= 1;
    mdelay(1);
    REG_ICDC_CDCCR1 &= 0xfffffffe;
}
#if SLOW_ANTI_POP
void i2s_codec_init(void)
{


	REG_CPM_SCR &= 0x0fffffff;

	__i2s_internal_codec();
	__aic_select_i2s();
//	__aic_enable_unsignadj(); 
	
	REG_ICDC_CDCCR1 = 0x001b2303; //reset
	actual_delay_msec(10);
	REG_ICDC_CDCCR1 = 0x001b2302;
	actual_delay_msec(10);
	REG_ICDC_CDCCR2 = 0x001f0833;

	REG_ICDC_CDCCR1 = 0x001f2102;
        actual_delay_msec(550);                 
	REG_ICDC_CDCCR1 = 0x00033302;
	REG_ICDC_CDCCR1 = 0x14033302;
	actual_delay_msec(550);                 
	actual_delay_msec(550);      
	actual_delay_msec(550);
	HP_on_off_flag = 0;//HP is off

}
#endif
#if FAST_ANTI_POP
void i2s_codec_init(void)
{
	long val=0;
	signed long sample = 0;
	int errno,hpvol_step,sample_shift,tx_con=0;
	
	REG_CPM_SCR &= 0x0fffffff;

	__i2s_internal_codec();
	__i2s_as_slave();
	__i2s_select_i2s();
	__aic_select_i2s();
//	__aic_enable_unsignadj(); 
	
	HP_on_off_flag = 0;//HP is off
	REG_ICDC_CDCCR1 = 0x001b2303;
	mdelay(1);
	REG_ICDC_CDCCR1 = 0x001b2302;

	REG_ICDC_CDCCR1 = 0x03000100;
	
	mdelay(500);
	mdelay(500);
	mdelay(500);
	mdelay(500);
	mdelay(500);
	mdelay(500);

	__aic_play_lastsample();
	val = REG_ICDC_CDCCR2;
	val = val & 0xfffff0f0;
	val = val | 0x00000803;
	REG_ICDC_CDCCR2 = val;

	__i2s_set_oss_sample_size(18);
	__aic_enable_mono2stereo();

	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;

	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;

	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;

	__aic_enable_replay();
	__i2s_enable();
	mdelay(20);

	sample = 0x1ffff;
	REG_ICDC_CDCCR1 = 0x03000000;
	//sta1 = jiffies;
	while(sample >= 0) {		
		tx_con = (REG_AIC_SR & 0x00003f00) >> 8;
		if(tx_con < 32) {
			REG_AIC_DR = (signed long)sample;
			sample -= 6;//460ms
		}
	}
//	tx_con = (REG_AIC_SR & 0x00003f00) >> 8;
//	while(tx_con > 0) {
//		tx_con = (REG_AIC_SR & 0x00003f00) >> 8;
//	}
	REG_AIC_DR = 0x1;
	//REG_ICDC_CDCCR1 = 0x03002000;
	//sta2 = jiffies;
	//printk("replay %d\n", sta2-sta1);
	//__aic_disable_replay();
	//__i2s_disable();
	__aic_disable_mono2stereo();
	/*__aic_disable_replay();
	__i2s_disable();
	__aic_disable_mono2stereo();*/
	__i2s_set_oss_sample_size(16);
//	REG_ICDC_CDCCR1 = 0x03002000;
	REG_ICDC_CDCCR1 = 0x00033302;
	REG_AIC_DR = 0x1;

}
#endif
#if NEW_ANTI_POP
void i2s_codec_init(void)
{
	long val=0;
	signed long sample = 0;
	int errno,hpvol_step,sample_shift,tx_con=0;
	
	REG_CPM_SCR &= 0x0fffffff;

	__i2s_internal_codec();
	__i2s_as_slave();
	__i2s_select_i2s();
	__aic_select_i2s();
//	__aic_enable_unsignadj(); 
	
	HP_on_off_flag = 0;//HP is off
	REG_ICDC_CDCCR1 = 0x001b2303;
	mdelay(1);
	REG_ICDC_CDCCR1 = 0x001b2302;

	REG_ICDC_CDCCR1 = 0x03000100;
	
	mdelay(500);
	mdelay(500);
	mdelay(500);
	mdelay(500);
	mdelay(500);
	mdelay(500);

	__aic_play_lastsample();
	val = REG_ICDC_CDCCR2;
	val = val & 0xfffff0f0;
	val = val | 0x00000803;
	REG_ICDC_CDCCR2 = val;

	__i2s_set_oss_sample_size(18);
	__aic_enable_mono2stereo();

	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;

	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;

	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;
	REG_AIC_DR = 0x1ffff;

	__aic_enable_replay();
	__i2s_enable();
	mdelay(20);

	sample = 0x1ffff;
	REG_ICDC_CDCCR1 = 0x03000000;

	while(sample >= 0) {		
		tx_con = (REG_AIC_SR & 0x00003f00) >> 8;
		if(tx_con < 32) {
			REG_AIC_DR = (signed long)sample;
			sample -= 6;//460ms
		}
	}
	tx_con = (REG_AIC_SR & 0x00003f00) >> 8;
	while(tx_con > 0) {
		tx_con = (REG_AIC_SR & 0x00003f00) >> 8;
	}
	REG_AIC_DR = 0x1;

	__aic_disable_mono2stereo();

	__i2s_set_oss_sample_size(16);
	REG_ICDC_CDCCR1 = 0x03002000;
	REG_AIC_DR = 0x1;

}
#endif
void i2s_codec_set_mic(u16 v) /* 0 <= v <= 100 */
{
	v = v & 0xff;
	if(v < 0)
		v = 0;
	if(v > 100)
		v = 100;	
	codec_mic_gain = 31 * v/100;
	REG_ICDC_CDCCR2 = ((REG_ICDC_CDCCR2 & ~(0x1f << 16)) | (codec_mic_gain << 16));
}
void i2s_codec_set_bass(u16 v) /* 0 <= v <= 100 */
{
	v = v & 0xff;
	if(v < 0)
		v = 0;
	if(v > 100)
		v = 100;

	if(v < 25)
		codec_base_gain = 0;
	if(v >= 25 && v < 50)
		codec_base_gain = 1;
	if(v >= 50 && v < 75)
		codec_base_gain = 2;
	if(v >= 75 && v <= 100 )
		codec_base_gain = 3;
	REG_ICDC_CDCCR2 = ((REG_ICDC_CDCCR2 & ~(0x3 << 4)) | (codec_base_gain << 4));


}
void i2s_codec_set_volume(u16 v) /* 0 <= v <= 100 */
{
	v = v & 0xff;
	if(v < 0)
		v = 0;
	if(v > 100)
		v = 100;

	if(v < 25)
		codec_volume = 0;
	if(v >= 25 && v < 50)
		codec_volume = 1;
	if(v >= 50 && v < 75)
		codec_volume = 2;
	if(v >= 75 && v <= 100 )
		codec_volume = 3;
	
	REG_ICDC_CDCCR2 = ((REG_ICDC_CDCCR2 & ~(0x3)) | codec_volume);


}
u16 i2s_codec_get_bass(void)
{
	u16 val;
	int ret;
	if(codec_base_gain == 0)
		val = 0;
	if(codec_base_gain == 1)
		val = 25;
	if(codec_base_gain == 2)
		val = 50;
	if(codec_base_gain == 3)
		val = 75;
	
	ret = val << 8;
	val = val | ret;
}
u16 i2s_codec_get_mic(void)
{
	u16 val;
	int ret;
	val = 100 * codec_mic_gain / 31;
	ret = val << 8;
	val = val | ret;
}
u16 i2s_codec_get_volume(void)
{
	u16 val;
	int ret;

	if(codec_volume == 0)
		val = 0;
	if(codec_volume == 1)
		val = 25;
	if(codec_volume == 2)
		val = 50;
	if(codec_volume == 3)
		val = 75;
	
	ret = val << 8;
	val = val | ret;
	return val;
}

void i2s_codec_set_channel(u16 ch)
{
//	i2s_codec_write(I2S_KEYCLICK_CONTROL, 0x4415);
}

void i2s_codec_set_samplerate(u16 rate)
{
	short speed = 0;
	unsigned short val = 0;
	long codec_speed;
	
		switch (rate) {
		case 8000:
			speed = 0;
			break;
		case 11025:
			speed = 1;
			break;
		case 12000:
			speed = 2;
			break;
		case 16000:
			speed = 3;
			break;
		case 22050:
			speed = 4;
			break;
		case 24000:
			speed = 5;
			break;
		case 32000:
			speed = 6;
			break;
		case 44100:
			speed = 7;
			break;
		case 48000:
			speed = 8;
			break;
		default:
			break;
		}

	codec_speed = REG_ICDC_CDCCR2;
        codec_speed |= 0x00000f00;

	speed = speed << 8;
	speed |= 0xfffff0ff;

	codec_speed &= speed;

	REG_ICDC_CDCCR2 = codec_speed;
}

void actual_delay_msec(long delay_val)
{
	udelay(delay_val*1000);	
}

#if SLOW_ANTI_POP
void HP_turn_on(void)
{ 
        //see 1.3.4.1
	REG_ICDC_CDCCR1 = 0x00037302;  // anti-pop
	actual_delay_msec(2);
	REG_ICDC_CDCCR1 = 0x03006000;  // anti-pop
	actual_delay_msec(2);
	REG_ICDC_CDCCR1 = 0x03002000;/*good*/ //bad 0x03000000 can replay // anti-pop
}
#endif
#if FAST_ANTI_POP
void HP_turn_on(void)
{ 
        //see 1.3.4.1
		REG_AIC_DR = 0x1;
	REG_AIC_DR = 0x1;
	REG_AIC_DR = 0x1;
	REG_AIC_DR = 0x1;
	REG_ICDC_CDCCR1 = 0x00037302;  // anti-pop
	actual_delay_msec(2);
	REG_ICDC_CDCCR1 = 0x03006000;  // anti-pop
	actual_delay_msec(2);
	REG_ICDC_CDCCR1 = 0x03002000;/*good*/ //bad 0x03000000 can replay // anti-pop
}
#endif
#if NEW_ANTI_POP
static void HP_turn_on(void)
{
	REG_AIC_DR = 0x1;
	REG_AIC_DR = 0x1;
	REG_AIC_DR = 0x1;
	REG_AIC_DR = 0x1;
	//__i2s_enable();
}
#endif

#if NEW_ANTI_POP
static void HP_turn_off(void)
{
	REG_AIC_DR = 0x1;
	REG_AIC_DR = 0x1;
	REG_AIC_DR = 0x1;
	REG_AIC_DR = 0x1;
	//__i2s_enable();
}
#endif

#if SLOW_ANTI_POP
void HP_turn_off(void)
{
	//see 1.3.4.1
	REG_ICDC_CDCCR1 = 0x00033302;  // anti-pop
	actual_delay_msec(550);                  
	actual_delay_msec(550);
	actual_delay_msec(550);
}
#endif

#if FAST_ANTI_POP
void HP_turn_off(void)
{
	//see 1.3.4.1
	REG_ICDC_CDCCR1 = 0x00033302;  // anti-pop
	actual_delay_msec(550);                  
	actual_delay_msec(550);
	actual_delay_msec(550);
}
#endif

void in_codec_app1(void)
{
	/* test is OK */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}
	REG_ICDC_CDCCR1 &= 0xc3ffbfff;
	REG_ICDC_CDCCR1 |= 0x03000000;
//	REG_ICDC_CDCCR2 = 0x00170803;
}

void in_codec_app2(void)
{
	/* test is OK */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}

	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 | (1 << 29) | (1 << 27 ) | (1 << 25) | (1 << 24);
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 & ~(1 << 28) & ~(1 << 26) & ~(1 << 14);

	//REG_ICDC_CDCCR2 = 0x00170803;
}


static void in_codec_app3(void)
{
	/* test is OK */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}

	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 | (1 << 28) | (1 << 27 ) | (1 << 25) | (1 << 24);
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 & ~(1 << 29) & ~(1 << 26) & ~(1 << 14);

	//REG_ICDC_CDCCR2 = 0x00170803;
}

static void in_codec_app4(void)
{
	/* test is OK */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 | (1 << 29) | (1 << 28) | (1 << 27 ) | (1 << 25) | (1 << 24);
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 & ~(1 << 26) & ~(1 << 14);

	//REG_ICDC_CDCCR2 = 0x00170803;
}

static void in_codec_app5(void)
{
	/* wait to be test */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}

	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 | (1 << 28) | (1 << 26) | (1 << 25) | (1 << 24);
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 & ~(1 << 29) & ~(1 << 27) & ~(1 << 14);

	//REG_ICDC_CDCCR2 = 0x00170803;
}

static void in_codec_app6(void)
{
	/* test is OK */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 | (1 << 29) | (1 << 27);
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 & ~(1 << 28) & ~(1 << 26) & ~(1 << 25) & ~(1 << 24) & ~(1 << 14);

	//REG_ICDC_CDCCR2 = 0x00170803;
}

static void in_codec_app7(void)
{
	/* test is OK */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}	

	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 | (1 << 29) | (1 << 26) | (1 << 14);
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 & ~(1 << 28) & ~(1 << 27) & ~(1 << 25) & ~(1 << 24);

	//REG_ICDC_CDCCR2 = 0x00170803;
}

static void in_codec_app8(void)
{
	/* test is wrong */
	if(HP_on_off_flag == 1) {
		HP_on_off_flag = 0;
		HP_turn_off();
	}	

	REG_ICDC_CDCCR1 &= ~(1 << 1);
	udelay(7000);


	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 | (1 << 29) | (1 << 26);
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 & ~(1 << 28) & ~(1 << 24);

	//REG_ICDC_CDCCR2 = 0x00170803;
}

static void in_codec_app9(void)
{
	/* test is OK */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}	

	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 | (1 << 29) | (1 << 27) | (1 << 26);
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 & ~(1 << 28) & ~(1 << 25) & ~(1 << 24) & ~(1 << 14);
	//REG_ICDC_CDCCR2 = 0x00170803;
}

static void in_codec_app10(void)
{
	/* test is OK */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}	

	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 | (1 << 28) | (1 << 27);
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 & ~(1 << 29) & ~(1 << 26) & ~(1 << 25) & ~(1 << 24) & ~(1 << 14);

	//REG_ICDC_CDCCR2 = 0x00170803;
}

void in_codec_app11(void)
{
	/* test is OK */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}

	REG_ICDC_CDCCR1 |= 1 << 28;
	REG_ICDC_CDCCR1 |= 1 << 26;
	REG_ICDC_CDCCR1 |= 1 << 14;

	REG_ICDC_CDCCR1 &= ~(1 << 29);
	REG_ICDC_CDCCR1 &= ~(1 << 27);
	REG_ICDC_CDCCR1 &= ~(1 << 25);
	REG_ICDC_CDCCR1 &= ~(1 << 24);

	//REG_ICDC_CDCCR2 = 0x00170823;
}


void in_codec_app12(void)
{
	/* test is OK */
	//REG_ICDC_CDCCR1 = 0x00033300;
	//REG_ICDC_CDCCR1 = 0x14004000;
	//REG_ICDC_CDCCR1 = 0x1c004000;
	//REG_ICDC_CDCCR1 = 0x1c004300;
//	REG_ICDC_CDCCR1 = 0x1c000000;
	//       REG_ICDC_CDCCR1 = 0x14000000;
	//REG_ICDC_CDCCR2 = 0x00170820;
	REG_ICDC_CDCCR1 = 0x14024300;
	mdelay(50);
}

void in_codec_app13(void)
{
	/* test is OK */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 | (1 << 28) | (1 << 27) | (1 << 26);
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 & ~(1 << 29) & ~(1 << 25) & ~(1 << 24) & ~(1 << 14);
}

static void in_codec_app14(void)
{
	/* test is OK */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 | (1 << 29) | (1 << 28) | (1 << 27);
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 & ~(1 << 26) & ~(1 << 25) & ~(1 << 24) & ~(1 << 14);
}

static void in_codec_app15(void)
{
	/* test is OK */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 | (1 << 29) | (1 << 28) | (1 << 26) | (1 << 14);
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 & ~(1 << 27) & ~(1 << 25) & ~(1 << 24);
}

static void in_codec_app16(void)
{
	/* test is wrong */
	if(HP_on_off_flag == 1) {
		HP_on_off_flag = 0;
		HP_turn_off();
	}	

	REG_ICDC_CDCCR1 &= ~(1 << 1);
	udelay(7000);

	REG_ICDC_CDCCR1 |= 1 << 29;
	REG_ICDC_CDCCR1 |= 1 << 28;
	REG_ICDC_CDCCR1 |= 1 << 26;

	REG_ICDC_CDCCR1 &= ~(1 << 24);

	//REG_ICDC_CDCCR2 = 0x00170823;
}

static void in_codec_app17(void)
{
	/* test is OK */
	if(HP_on_off_flag == 0) {
		HP_on_off_flag = 1;
		HP_turn_on();
	}
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 | (1 << 29) | (1 << 28) | (1 << 27) | (1 << 26);
	REG_ICDC_CDCCR1 = REG_ICDC_CDCCR1 & ~(1 << 25) & ~(1 << 24) & ~(1 << 14);
}

