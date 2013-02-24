#define NR_PALETTE	256
#if 0
struct jzfb_info {

        unsigned int cfg;	/* panel mode and pin usage etc. */
	unsigned int w;
	unsigned int h;
	unsigned int bpp;	/* bit per pixel */
	unsigned int fclk;	/* frame clk */
	unsigned int hsw;	/* hsync width, in pclk */
	unsigned int vsw;	/* vsync width, in line count */
	unsigned int elw;	/* end of line, in pclk */
	unsigned int blw;	/* begin of line, in pclk */
	unsigned int efw;	/* end of frame, in line count */
	unsigned int bfw;	/* begin of frame, in line count */

	unsigned char *cpal;	/* Cacheable Palette Buffer */
	unsigned char *pal;	/* Non-cacheable Palette Buffer */
	unsigned char *cframe;	/* Cacheable Frame Buffer */
	unsigned char *frame;	/* Non-cacheable Frame Buffer */

	struct {
		unsigned char red, green, blue;
	} palette[NR_PALETTE];
};
#endif
extern struct jzfb_info jzfb;

void lcd_test()
{
	unsigned int *frame;
	unsigned int i;
	
	
	jzlcd_init();
	frame = jzfb.frame;
	printf("lcd test = 0x%x 0x%x 0x%x\r\n",frame,jzfb.frame,&jzfb);
	for(i = 0;i < 10;i++)
	{
		printf("lcd test = 0x%x \r\n",frame[i * 480]);
	}


	for(i = 0;i < 480 * 90;i++)
	{
		frame[i] = 0x000000ff;
	}
	for(;i < 480 * 180;i++)
	{
		frame[i] = 0x0000ff00;
	}
	for(;i < 480 * 272;i++)
	{
		frame[i] = 0x00ff0000;
	}
	for(i = 0;i < 10;i++)
	{
		printf("lcd test = 0x%x \r\n",frame[i * 480]);
	}
 	
	
}
