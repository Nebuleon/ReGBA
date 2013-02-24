#include <bsp.h>
#include <jz4740.h>
#include <lcdc.h>
#include <dm.h>

#ifdef UNCACHED
#undef UNCACHED
#endif
#define UNCACHED(addr)	((unsigned int)(addr) | 0xa0000000)
#define STATIC_MALLOC

#define dprintf(x...)

static struct lcd_desc lcd_palette_desc __attribute__ ((aligned (32)));
//static struct lcd_desc lcd_frame_desc0 __attribute__ ((aligned (32)));
//static struct lcd_desc lcd_frame_desc1 __attribute__ ((aligned (32)));
static struct lcd_desc lcd_frame_desc2[FRAMEBUF_NUM] __attribute__ ((aligned (32)));

static struct lcd_desc *cur_lcd_frame_desc,*old_lcd_frame_desc;
u32 lcd_get_width(void) {return jzfb.w;}
u32 lcd_get_height(void) {return jzfb.h;}
u32 lcd_get_bpp(void) {
	if(jzfb.bpp > 16)
		return 32;
	return jzfb.bpp;
}
u8* lcd_get_frame(void) {return jzfb.frame;}
u8* lcd_get_cframe(void) {return jzfb.cframe;}


u8* lcd_get_change_frame(void) {return (u8*)UNCACHED(old_lcd_frame_desc->buf);}
u8* lcd_get_change_cframe(void) {return (u8*)(old_lcd_frame_desc->buf + 0x80000000);}
u8* lcd_get_change_phyframe(void){return old_lcd_frame_desc->buf;}

u8* lcd_get_current_frame(void) {return (u8*)UNCACHED(cur_lcd_frame_desc->buf);}
u8* lcd_get_current_cframe(void) {return (u8*)(cur_lcd_frame_desc->buf + 0x80000000);}
u8* lcd_get_current_phyframe(void){return cur_lcd_frame_desc->buf;}

void lcd_clean_frame_all() 
{ 
	int i; 
	for(i = 0; i < FRAMEBUF_NUM;i++)
		memset(jzfb.cframe2[i],0,
			lcd_get_width() * lcd_get_height() * lcd_get_bpp() / 8
		);
}
void lcd_flush_frame_all() {__dcache_writeback_all();}
void lcd_change_frame(void){
	
	old_lcd_frame_desc->next = PHYS(old_lcd_frame_desc);
	cur_lcd_frame_desc->next = PHYS(old_lcd_frame_desc);
	cur_lcd_frame_desc = old_lcd_frame_desc;
	old_lcd_frame_desc++;
	if(old_lcd_frame_desc >= &lcd_frame_desc2[FRAMEBUF_NUM])
		old_lcd_frame_desc = &lcd_frame_desc2[0];
}
void lcd_reset_frame()
{
	if((int)cur_lcd_frame_desc != (int)&lcd_frame_desc2[0])
	{
		old_lcd_frame_desc = &lcd_frame_desc2[0];
		old_lcd_frame_desc->next = PHYS(old_lcd_frame_desc);
		cur_lcd_frame_desc->next = PHYS(old_lcd_frame_desc);
	}	
}
static __inline__ unsigned int __cpm_divisor_encode(unsigned int n)
{
	unsigned int encode[10] = {1,2,3,4,6,8,12,16,24,32};
	int i;
	for (i=0;i<10;i++)
		if (n < encode[i])
			break;
	return i;
}
static int jzfb_setcolreg(u32 regno, u8 red, u8 green, u8 blue)
{
	u16 *ptr, ctmp;

	if (regno >= NR_PALETTE)
		return 1;

	red	&= 0xff;
	green	&= 0xff;
	blue	&= 0xff;
	
	jzfb.palette[regno].red		= red ;
	jzfb.palette[regno].green	= green;
	jzfb.palette[regno].blue	= blue;
	
	if (jzfb.bpp <= 8) {
		if (((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_SINGLE) ||
		    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL)) {
			ctmp = (77L * red + 150L * green + 29L * blue) >> 8;
			ctmp = ((ctmp >> 3) << 11) | ((ctmp >> 2) << 5) |
				(ctmp >> 3);
		} else {
			/* RGB 565 */
			if (((red >> 3) == 0) && ((red >> 2) != 0))
				red = 1 << 3;
			if (((blue >> 3) == 0) && ((blue >> 2) != 0))
				blue = 1 << 3;
			ctmp = ((red >> 3) << 11) 
				| ((green >> 2) << 5) | (blue >> 3);
		}

		ptr = (u16 *)jzfb.pal;
		ptr[regno] = ctmp;
		REG_LCD_DA0 = PHYS(&lcd_palette_desc);
	} else
		printf("No palette used.\n");

	return 0;
}


#ifdef STATIC_MALLOC
static u8 lcd_heap[4096 + (480 * 272 * 4 + 4095) / 4096 * 4096 * FRAMEBUF_NUM] __attribute__((aligned(4096))); // 4Kb align
#endif

/*
 * Map screen memory
 */
static int fb_malloc(void)
{
	struct page * map = NULL;
	u8 *tmp;
	u32 page_shift, needroom, t;
	u32 i;
	if (jzfb.bpp == 15)
		t = 16;
	if(jzfb.bpp == 18)
		t = 32;
	else
		t = jzfb.bpp;

	needroom = ((jzfb.w * t + 7) >> 3) * jzfb.h;

	jzfb.cpal = (u8 *)(((u32)lcd_heap) & ~0xfff);
	jzfb.cframe = (u8 *)((u32)jzfb.cpal + 0x1000);
	for(i = 0; i < FRAMEBUF_NUM;i++)  
		jzfb.cframe2[i] = (u8 *)((u32)jzfb.cframe + ((needroom + 0x1000 - 1)/ 0x1000 * 0x1000 * i));

	jzfb.pal = (u8 *)UNCACHED(jzfb.cpal);
	jzfb.frame = (u8 *)UNCACHED(jzfb.cframe);
	for(i = 0; i < FRAMEBUF_NUM;i++) 
	{ 
		//memset(jzfb.cframe2[i], 0, needroom);
		jzfb.frame2[i] = (u8 *)UNCACHED(jzfb.cframe2[i]);
	}
	memset(jzfb.cpal, 0, 512);
	return 0;
}

static void lcd_descriptor_init(void)
{
	int i;
	unsigned int pal_size;
	unsigned int frm_size, ln_size;
	unsigned char dual_panel = 0;
	struct lcd_desc *pal_desc, *frame_desc0, *frame_desc1;

	pal_desc	= &lcd_palette_desc;
	frame_desc0	= &lcd_frame_desc2[0];
	frame_desc1	= &lcd_frame_desc2[0];
	
	
	i = jzfb.bpp;
	if (i == 15)
		i = 16;
	if( i == 18)
		i = 32;
	frm_size = (jzfb.w*jzfb.h*i)>>3;
	ln_size = (jzfb.w*i)>>3;

	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL)) {
		dual_panel = 1;
		frm_size >>= 1;
	}

	frm_size = frm_size / 4;
	ln_size = ln_size / 4;

	switch (jzfb.bpp) {
	case 1:
		pal_size = 4;
		break;
	case 2:
		pal_size = 8;
		break;
	case 4:
		pal_size = 32;
		break;
	case 8:
	default:
		pal_size = 512;
		break;
	}

	pal_size /= 4;

	/* Palette Descriptor */
	pal_desc->next	= PHYS(frame_desc0);
	pal_desc->buf	= PHYS(jzfb.pal);
	pal_desc->id	= 0xdeadbeaf;
	pal_desc->cmd	= pal_size|LCD_CMD_PAL; /* Palette Descriptor */

	/* Frame Descriptor 0 */
	frame_desc0->next	= PHYS(frame_desc0);
	frame_desc0->buf	= PHYS(jzfb.frame);
	frame_desc0->id		= 0xbeafbeaf;
	//frame_desc0->cmd	= LCD_CMD_SOFINT | LCD_CMD_EOFINT | frm_size;
	frame_desc0->cmd	= frm_size;

	//if (!(dual_panel))
	//	return;

	/* Frame Descriptor 1 */
	for(i = 0;i < FRAMEBUF_NUM;i++)
	{
		
		frame_desc1->next	= PHYS(frame_desc1);
		frame_desc1->buf	= PHYS(jzfb.frame2[i]);
		frame_desc1->id		= 0xdeaddead - i;
		//frame_desc1->cmd	 = LCD_CMD_SOFINT | LCD_CMD_EOFINT | frm_size;
	  frame_desc1->cmd	 = frm_size;
	  frame_desc1++;
  }
  if(FRAMEBUF_NUM > 0)
  	old_lcd_frame_desc =  &lcd_frame_desc2[1];
  cur_lcd_frame_desc = &lcd_frame_desc2[0];	
}

static int controller_init(void)
{
	unsigned int val = 0;
	unsigned int pclk;
	unsigned int stnH;
	int ret = 0;

	/* Setting Control register */
	switch (jzfb.bpp) {
	case 1:
		val |= LCD_CTRL_BPP_1;
		break;
	case 2:
		val |= LCD_CTRL_BPP_2;
		break;
	case 4:
		val |= LCD_CTRL_BPP_4;
		break;
	case 8:
		val |= LCD_CTRL_BPP_8;
		break;
	case 15:
		val |= LCD_CTRL_RGB555;
	case 16:
		val |= LCD_CTRL_BPP_16;
		break;
//	case 18:
//		val |= LCD_CTRL_BPP_18_24;
//		break;
	case 17 ... 32:
		val |= LCD_CTRL_BPP_18_24;	/* target is 4bytes/pixel */
		break;	
	default:
		printf("The BPP %d is not supported\n", jzfb.bpp);
		val |= LCD_CTRL_BPP_16;
		break;
	}

	switch (jzfb.cfg & MODE_MASK) {
	case MODE_STN_MONO_DUAL:
	case MODE_STN_COLOR_DUAL:
	case MODE_STN_MONO_SINGLE:
	case MODE_STN_COLOR_SINGLE:
		switch (jzfb.bpp) {
		case 1:
//			val |= LCD_CTRL_PEDN;
		case 2:
			val |= LCD_CTRL_FRC_2;
			break;
		case 4:
			val |= LCD_CTRL_FRC_4;
			break;
		case 8:
		default:
			val |= LCD_CTRL_FRC_16;
			break;
		}
		break;
	}

	//val |= LCD_CTRL_BST_4;		/* Burst Length is 16WORD=64Byte */
	val |= LCD_CTRL_BST_16;		/* Burst Length is 16WORD=64Byte */

	switch (jzfb.cfg & MODE_MASK) {
	case MODE_STN_MONO_DUAL:
	case MODE_STN_COLOR_DUAL:
	case MODE_STN_MONO_SINGLE:
	case MODE_STN_COLOR_SINGLE:
		switch (jzfb.cfg & STN_DAT_PINMASK) {
#define align2(n) (n)=((((n)+1)>>1)<<1)
#define align4(n) (n)=((((n)+3)>>2)<<2)
#define align8(n) (n)=((((n)+7)>>3)<<3)
		case STN_DAT_PIN1:
			/* Do not adjust the hori-param value. */
			break;
		case STN_DAT_PIN2:
			align2(jzfb.hsw);
			align2(jzfb.elw);
			align2(jzfb.blw);
			break;
		case STN_DAT_PIN4:
			align4(jzfb.hsw);
			align4(jzfb.elw);
			align4(jzfb.blw);
			break;
		case STN_DAT_PIN8:
			align8(jzfb.hsw);
			align8(jzfb.elw);
			align8(jzfb.blw);
			break;
		}
		break;
	}

	REG_LCD_CTRL = val;

	switch (jzfb.cfg & MODE_MASK) {
	case MODE_STN_MONO_DUAL:
	case MODE_STN_COLOR_DUAL:
	case MODE_STN_MONO_SINGLE:
	case MODE_STN_COLOR_SINGLE:
		if (((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL) ||
		    ((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL))
			stnH = jzfb.h >> 1;
		else
			stnH = jzfb.h;

		REG_LCD_VSYNC = (0 << 16) | jzfb.vsw;
		REG_LCD_HSYNC = ((jzfb.blw+jzfb.w) << 16) | (jzfb.blw+jzfb.w+jzfb.hsw);

		/* Screen setting */
		REG_LCD_VAT = ((jzfb.blw + jzfb.w + jzfb.hsw + jzfb.elw) << 16) | (stnH + jzfb.vsw + jzfb.bfw + jzfb.efw);
		REG_LCD_DAH = (jzfb.blw << 16) | (jzfb.blw + jzfb.w);
		REG_LCD_DAV = (0 << 16) | (stnH);

		/* AC BIAs signal */
		REG_LCD_PS = (0 << 16) | (stnH+jzfb.vsw+jzfb.efw+jzfb.bfw);

		break;

	case MODE_TFT_GEN:
	case MODE_TFT_SHARP:
	case MODE_TFT_CASIO:
	case MODE_8BIT_SERIAL_TFT:
	case MODE_TFT_SAMSUNG:
		REG_LCD_VSYNC = (0 << 16) | jzfb.vsw;
		REG_LCD_DAV = ((jzfb.vsw + jzfb.bfw) << 16) | (jzfb.vsw + jzfb.bfw + jzfb.h);
		REG_LCD_VAT = (((jzfb.blw + jzfb.w + jzfb.elw + jzfb.hsw)) << 16) | (jzfb.vsw + jzfb.bfw + jzfb.h + jzfb.efw);
		REG_LCD_HSYNC = (0 << 16) | jzfb.hsw;
		REG_LCD_DAH = ((jzfb.hsw + jzfb.blw) << 16) | (jzfb.hsw + jzfb.blw + jzfb.w);
		break;
	}

	switch (jzfb.cfg & MODE_MASK) {
	case MODE_TFT_SAMSUNG:
	case MODE_TFT_SHARP:
	case MODE_TFT_CASIO:
		printf("LCD DOES NOT supported.\n");
		break;
	}

	/* Configure the LCD panel */
	REG_LCD_CFG = jzfb.cfg;

	/* Timing setting */
	__cpm_stop_lcd();

	val = jzfb.fclk; /* frame clk */
	pclk = val * (jzfb.w + jzfb.hsw + jzfb.elw + jzfb.blw) *
	       (jzfb.h + jzfb.vsw + jzfb.efw + jzfb.bfw); /* Pixclk */

	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_SINGLE) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL))
		pclk = (pclk * 3);

	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_SINGLE) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_SINGLE) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL))
		pclk = pclk >> ((jzfb.cfg & STN_DAT_PINMASK) >> 4);

	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL))
		pclk >>= 1;
 

	
	val = __cpm_get_pllout2() / pclk;
	__cpm_set_pixdiv(val - 1);
    

	val = __cpm_get_pllout2() / (pclk * 4);
	__cpm_set_ldiv(val - 1);

    
	REG_CPM_CPCCR |= CPM_CPCCR_CE;
	
    
	printf("LCDC: %d LcdClock:%d\n",
		__cpm_get_pixclk(), __cpm_get_lcdclk());
   
	__cpm_start_lcd();
	udelay(100);
	//printf("REG_LCD_CTRL: 0x%x\r\n",REG_LCD_CTRL);
	
	return ret;
}

static void lcd_interrupt_handler(u32 irq)
{
	u32 state;

	state = REG_LCD_STATE;

	if (state & LCD_STATE_EOF) /* End of frame */
		REG_LCD_STATE = state & ~LCD_STATE_EOF;

	if (state & LCD_STATE_IFU0) {
		dprintf("InFiFo0 underrun\n");
		REG_LCD_STATE = state & ~LCD_STATE_IFU0;
	}

	if (state & LCD_STATE_OFU) { /* Out fifo underrun */
		dprintf("Out FiFo underrun.\n");
		REG_LCD_STATE = state & ~LCD_STATE_OFU;
	}
}

static int hw_init(void)
{
	lcd_board_init();

	if (controller_init() < 0)
		return -1;

	if (jzfb.bpp <= 8)
		REG_LCD_DA0 = PHYS(&lcd_palette_desc);
	else
		REG_LCD_DA0 = PHYS(&lcd_frame_desc2[0]);

	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL))
		REG_LCD_DA1 = PHYS(&lcd_frame_desc2[1]);

	__lcd_set_ena();

	return 0;
}

int jzlcd_init(void)
{
	int err = 0;
    __gpio_as_lcd_18bit();

	__gpio_as_input(32 + 17);
	__gpio_as_input(32 + 18);
	__gpio_disable_pull(32 + 17);
	__gpio_disable_pull(32 + 18);
	
	__gpio_as_input(32*2 + 22);
	__gpio_as_input(32*2 + 23);
	__gpio_disable_pull(32*2 + 22);
	__gpio_disable_pull(32*2 + 23);
	
/*
	REG_GPIO_PXFUNS(2) = 0x003fffff;	\
	REG_GPIO_PXSELC(2) = 0x003fffff;	\
	REG_GPIO_PXPES(2) = 0x003fffff;		\
*/
	err = fb_malloc();
	if (err) 
		goto failed;
	lcd_descriptor_init();

	__dcache_writeback_all();

	if (hw_init() < 0)
		goto failed;

	request_irq(IRQ_LCD, lcd_interrupt_handler, 0);

	__lcd_enable_ofu_intr();
	__lcd_enable_ifu0_intr();
	return 0;

failed:
	return err;

}

#if (DM==1)
void  lcd_poweron(void)
{
}
void  lcd_poweroff(void)
{
}
int lcd_preconvert(void)
{
//	__cpm_stop_lcd();

	return 1;
}
void  lcd_convert(void)
{
	unsigned int val = 0;
	unsigned int pclk;
//	__cpm_stop_lcd();
	val = jzfb.fclk; /* frame clk */
	pclk = val * (jzfb.w + jzfb.hsw + jzfb.elw + jzfb.blw) *
		(jzfb.h + jzfb.vsw + jzfb.efw + jzfb.bfw); /* Pixclk */
	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_SINGLE) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL))
		pclk = (pclk * 3);
	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_SINGLE) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_SINGLE) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL))
		pclk = pclk >> ((jzfb.cfg & STN_DAT_PINMASK) >> 4);

	if (((jzfb.cfg & MODE_MASK) == MODE_STN_COLOR_DUAL) ||
	    ((jzfb.cfg & MODE_MASK) == MODE_STN_MONO_DUAL))
		pclk >>= 1;
	val = __cpm_get_pllout2() / pclk;
	__cpm_set_pixdiv(val - 1);
//	printf(" pdiv: %d --%d\n",val,pclk);
	val = __cpm_get_pllout2() / (pclk * 4);
	__cpm_set_ldiv(val - 1);
//	printf(" ldiv: %d-- %d\n",val,pclk);
    
//	REG_CPM_CPCCR |= CPM_CPCCR_CE;
//	__cpm_start_lcd();
	
}
void  mng_init_lcd(void)
{
	struct dm_jz4740_t lcd_dm;
	lcd_dm.name = "LCD driver";	
	lcd_dm.init = jzlcd_init;  
	lcd_dm.poweron = lcd_poweron;
	lcd_dm.poweroff = lcd_poweroff;
	lcd_dm.convert = lcd_convert;
	lcd_dm.preconvert = lcd_preconvert;
	dm_register(0,&lcd_dm);
}
void lcdstop()
{
    __lcd_set_dis();
}
#endif
