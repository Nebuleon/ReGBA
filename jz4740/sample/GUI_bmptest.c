#define NR_PALETTE	256

FS_FILE *myfile;
unsigned char mybuffer[0x100];
unsigned char bmpbuffer[0x100000];

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
extern struct jzfb_info jzfb;

static void _error(const char *msg) {
#if (FS_OS_EMBOS)
  if (strlen(msg)) { 
    OS_SendString(msg);
  }
#else
  printf("%s",msg);
#endif
}

void bmptest(void) {
	
	unsigned int *frame;
//	unsigned char pix[480*272*4];
    unsigned char pix[0x2000000];
	unsigned int X= 480;
	unsigned int Y= 272;
	unsigned int m, n;
	unsigned int *pt;
	int flag;
	
	jzlcd_init();
	frame = jzfb.frame;
	printf("Read bitmap...\n");
	
	flag= BMP_read("bg.bmp", pix, &X, &Y);
	if(flag)
	    printf("Read bitmap error...\n");
	
	printf("To display...\n");
	
	pt= (unsigned int*)pix;
	for(m= 0; m< Y*X; m++)
	    *frame++= *pt++;
	    
}
