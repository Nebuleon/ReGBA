/*
 *
 * JZ4740 SLCD driver.
 *
 * Author: <jgao@ingenic.cn>
 * Copyright (C) 2006  Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#if LCDTYPE == 3

#include <jz4740.h>

#define SLCD_DMA_EN 1
#define SLCD_DATA_SWAP 0

#define SLCD_NAME        "slcd"
#define PHYSADDR(x) ((x) & 0x1fffffff)

static int dma_enable = 0;
static unsigned short rgb_default = 0;

struct slcd_device {
	int mode;
	int dma_chan;
	char *ts_buf;
	char *pbat_buf;
	char *slcd_buf;
};

static struct slcd_device *slcd_dev;

static unsigned char *sssss, *aaaaa;

static volatile unsigned int *dma_ptr;
static unsigned int dma_idx = 0;


/************************************************************************************/
/************************      LCD INSTRUCTION       ********************************/
/************************************************************************************/
#define EXT_CHIP_CMD_ADDR  	(volatile UINT16*)0xac000000
#define EXT_CHIP_DATA_ADDR     	(volatile UINT16*)0xac000004


#define UINT16 unsigned short


#define GPPE2 	*( volatile unsigned int*)0xb0010064
#define GPDIR2 *( volatile unsigned int*)0xB001006c
#define GPD2   *(volatile unsigned int*)0xB0010060
#define GPFSH2   *(volatile unsigned int*)0xB0010074

static void DelayXms(long count);
static void delay(long delay_time);
static void mdelay(long mdelay_time)
{
	udelay(mdelay_time*1000);
}

void InitLCDgpio(void)
{

	unsigned int val, pclk;
	int pll_div, i;
	unsigned int pixclk, lcdclk;
	/* 
	 * LCD_DE -> SLCD_CS   -> WR#
	 * LCD_PS -> SLCD_RS   -> RS
	 * LCD_REV -> SLCD_CLK -> RD#
	 *
	 * LCD_CLS ->          -> CS#
	 * LCD_SPL ->          -> RESET#
	 */

#define PIN_CS_N 	(32*1+17)	/* LCD_CLS: GP B17 */
#define PIN_RD_N 	(32*2+23)	/* LCD_REV: GP C23 */
#define PIN_RESET_N 	(32*1+18)	/* LCD_SPL: GP B18 */


	/* LCD_D0 ~ LCD_D15, LCD_DE, LCD_PS */	
	REG_GPIO_PXFUNS(2) = 0x0060ffff;
	REG_GPIO_PXSELC(2) = 0x0060ffff;

	__gpio_as_output(PIN_CS_N); /* CS_N: LCD_SPL */
	__gpio_as_output(PIN_RD_N); /* RD#: LCD_REV */
	__gpio_as_output(PIN_RESET_N); /* RESET#: LCD_SPL */

	__gpio_clear_pin(PIN_CS_N); /* set CS low */
	__gpio_set_pin(PIN_RD_N); /*set read signal high */

	/* RESET# */
	__gpio_set_pin(PIN_RESET_N);
	mdelay(100);
	__gpio_clear_pin(PIN_RESET_N);
	mdelay(100);
	__gpio_set_pin(PIN_RESET_N);
	

	/* Configure SLCD module */

	REG_LCD_CFG &= ~LCD_CFG_LCDPIN_MASK;
	REG_LCD_CFG |= LCD_CFG_LCDPIN_SLCD;

	REG_SLCD_CFG = SLCD_CFG_BURST_4_WORD | SLCD_CFG_DWIDTH_16 | SLCD_CFG_CWIDTH_16BIT | SLCD_CFG_CS_ACTIVE_LOW | SLCD_CFG_RS_CMD_LOW | SLCD_CFG_CLK_ACTIVE_FALLING | SLCD_CFG_TYPE_PARALLEL;

	if (!dma_enable)
		REG_SLCD_CTRL = 0;

	__cpm_stop_lcd();

	pclk = 4000000; 	/* Pixclk:4MHz */

	pll_div = ( REG_CPM_CPCCR & CPM_CPCCR_PCS ); /* clock source,0:pllout/2 1: pllout */
	pll_div = pll_div ? 1 : 2 ;
	val = ( __cpm_get_pllout()/pll_div ) / pclk;
	val--;
	if ( val > 0x1ff ) {
		printf("CPM_LPCDR too large, set it to 0x1ff\n");
		val = 0x1ff;
	}
	__cpm_set_pixdiv(val);

	REG_CPM_CPCCR |= CPM_CPCCR_CE ; /* update divide */

	pixclk = __cpm_get_pixclk();
	lcdclk = __cpm_get_lcdclk();
	printf("LCDC: PixClock:%d LcdClock:%d\n",
	       pixclk, lcdclk);

	__cpm_start_lcd();
	udelay(1000);
}

static void CmdWrite(UINT16 cmd)
{  
#if SLCD_DATA_SWAP
	u16 tmp;
	tmp = cmd & 0xf;
	cmd &= ~0xf;
	cmd |= ((tmp & 0x1) << 3) | ((tmp & 0x2) << 1) | ((tmp & 0x4) >> 1) | ((tmp & 0x8) >> 3);
#endif
	if (dma_enable)
		dma_ptr[dma_idx++] = SLCD_FIFO_RS_COMMAND | cmd;
	else {
		while (REG_SLCD_STATE & SLCD_STATE_BUSY);
		udelay(30);
		REG_SLCD_DATA = SLCD_DATA_RS_COMMAND | cmd;
	}
}


static void DataWrite(UINT16 data)
{
#if SLCD_DATA_SWAP
	u16 tmp;
	tmp = data & 0xf;
	data &= ~0xf;
	data |= ((tmp & 0x1) << 3) | ((tmp & 0x2) << 1) | ((tmp & 0x4) >> 1) | ((tmp & 0x8) >> 3);
#endif

	if (dma_enable)
		dma_ptr[dma_idx++] = SLCD_FIFO_RS_DATA | data;
	else {
		while (REG_SLCD_STATE & SLCD_STATE_BUSY);
		udelay(20);
		REG_SLCD_DATA = SLCD_DATA_RS_DATA | data;
	}
}

static UINT16 ReadData(void)
{
#if 0
	UINT16 data;
	data = (*EXT_CHIP_DATA_ADDR);  
	printf("readdata:%4x\n",data);
	return data;
#else
	return 0; // Not implemented
#endif
} 

 

static void DelayXms(long count)
{
  long i,j;
  for(i=0;i<count;i++)
    for(j=0;j<5000;j++)
      { ; }
} 


static void delay(long delay_time)
{
  long cnt; 

  delay_time *= (384/8);

  for (cnt=0;cnt<delay_time;cnt++)
     {
     	asm("nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     			"nop\n"
     		);
        
		}

} 


/*---- LCD Initial ----*/
void LcdInit(void)
{ 
  delay(10000);        
  CmdWrite(0x0301); //reset
  delay(10000);
  CmdWrite(0x0101); 
  CmdWrite(0x0301); 
  CmdWrite(0x0008); 
  CmdWrite(0x2201);		//reset
  CmdWrite(0x0000);
  CmdWrite(0x0080);   //0x0020
  delay(10000);  

  CmdWrite(0x2809);
  CmdWrite(0x1900);
  CmdWrite(0x2110);
  CmdWrite(0x1805);
  CmdWrite(0x1E01);
  CmdWrite(0x1847);
  delay(1000);
  CmdWrite(0x1867);
  delay(10000);
  CmdWrite(0x18F7);
  delay(10000);
  CmdWrite(0x2100);
  CmdWrite(0x2809);
  CmdWrite(0x1a05);
  CmdWrite(0x1900);
  CmdWrite(0x1f64);
  CmdWrite(0x2070);
  CmdWrite(0x1e81);
  CmdWrite(0x1b01);
  
  CmdWrite(0x0200);
  CmdWrite(0x0504);   //y address increcement
  CmdWrite(0x0D00);		//*240
  CmdWrite(0x1D08);
  CmdWrite(0x2300);
  CmdWrite(0x2D01);
  CmdWrite(0x337F);
  CmdWrite(0x3400);
  CmdWrite(0x3501);
  CmdWrite(0x3700);
  CmdWrite(0x42ef);		//x start from 239
  CmdWrite(0x4300);
  CmdWrite(0x4400);		//y start from 0
  CmdWrite(0x4500);
  CmdWrite(0x46EF);
  CmdWrite(0x4700);
  CmdWrite(0x4800);
  CmdWrite(0x4901);
  CmdWrite(0x4A3F);
  CmdWrite(0x4B00);
  CmdWrite(0x4C00);
  CmdWrite(0x4D00);
  CmdWrite(0x4E00);
  CmdWrite(0x4F00);
  CmdWrite(0x5000);
  CmdWrite(0x7600);
  CmdWrite(0x8600);
  CmdWrite(0x8720);
  CmdWrite(0x8802);
  CmdWrite(0x8903);
  CmdWrite(0x8D40);
  CmdWrite(0x8F05);
  CmdWrite(0x9005);
  CmdWrite(0x9144);
  CmdWrite(0x9244);
  CmdWrite(0x9344);
  CmdWrite(0x9433);
  CmdWrite(0x9505);
  CmdWrite(0x9605);
  CmdWrite(0x9744);
  CmdWrite(0x9844);
  CmdWrite(0x9944);
  CmdWrite(0x9A33);
  CmdWrite(0x9B33);
  CmdWrite(0x9C33);
  //==> SETP 3
  CmdWrite(0x0000);
  CmdWrite(0x01A0);
  CmdWrite(0x3B01);
  
  CmdWrite(0x2809);
  delay(1000);      
  CmdWrite(0x1900);
  delay(1000);    
  CmdWrite(0x2110);
  delay(1000);    
  CmdWrite(0x1805);
  delay(1000);        
  CmdWrite(0x1E01);
  delay(1000);        
  CmdWrite(0x1847);
  delay(1000);        
  CmdWrite(0x1867);
  delay(1000);        
  CmdWrite(0x18F7);
  delay(1000);        
  CmdWrite(0x2100); 
  delay(1000);     
  CmdWrite(0x2809);
  delay(1000);      
  CmdWrite(0x1A05);    
  delay(1000);       
  CmdWrite(0x19E8);  
  delay(1000);    
  CmdWrite(0x1F64);
  delay(1000);      
  CmdWrite(0x2045); 
  delay(1000);       
  CmdWrite(0x1E81);    
  delay(1000);  
  CmdWrite(0x1B09); 
  delay(1000);       
  CmdWrite(0x0020);
  delay(1000);     
  CmdWrite(0x0120);
  delay(1000);     
  
  CmdWrite(0x3B01);
  delay(1000); 
    
}  

void LTPS_Window(UINT16 Min_X, UINT16 Min_Y , UINT16 Max_X , UINT16 Max_Y)
{ 
  CmdWrite(0x0510);				//windows address mode,y address increase
  CmdWrite(0x01c0);				//
  
  CmdWrite(0x4500+Min_X); /* Window X starting adress=0 */
  CmdWrite(0x4600+Max_X); /* Window X ending adress=239 */
 
  if(Min_Y<=0x00FF)
  {
  	CmdWrite(0x4800+Min_Y); /* Window Y starting adress=0 */
	CmdWrite(0x4700); /* Window Y starting adress=0 */
  }
  else
  {
    Min_Y &= 0x00FF;
    CmdWrite(0x4800+Min_Y); /* Window Y starting adress=0 */
    CmdWrite(0x4701);/* Window Y starting adress=0 */
  }
    
    
  if(Max_Y<=0x00FF)
  {
  CmdWrite(0x4A00+Max_Y); /* Window Y starting adress=0 */
   CmdWrite(0x4900); /* Window Y ending adress=0 */
  }
  else
    {
    Max_Y &= 0x00FF;
    CmdWrite(0x4A00+Max_Y); /* Window Y starting adress=0 */
    CmdWrite(0x4901);/* Window Y edning adress=0 */
    }     
}

void LTPS_CurSet(UINT16 X,UINT16 Y)
{
  CmdWrite(0x4200+X); /* X starting adress=0 */
  if(Y<=0x00FF)
   {
    CmdWrite(0x4400+Y); /* Y starting adress=0 */
    CmdWrite(0x4300); /* Y starting adress=0 */
   }
   else
   {
    Y &= 0X00FF;
    CmdWrite(0x4400+Y); /* Y starting adress=0 */
    CmdWrite(0x4301); /* X starting adress=0 */
   }
}

void LTPSFull(int Pix_16bit)
{ 
  unsigned char i; int j ;

 LTPS_Window(0,0,239,319);
 LTPS_CurSet(0,0);
 j=0;
 while(j<320)
 {
  i=0;
  while(i<240) 
   {   
    DataWrite(Pix_16bit);   
    i++;
    }
   j++;
 }
}


void LCD_DrawPixel(UINT16 x,UINT16 y,UINT16 Pix_16bit)
{
	LTPS_Window(0,0,239,319);
	LTPS_CurSet(239-x,319-y);
	DataWrite(Pix_16bit);
}

void LCD_DrawHorLine(UINT16 x,UINT16 y,UINT16 len,UINT16 Pix_16bit)
{
	int i = 0;
	LTPS_Window(0,0,239,319);
	for(i=0;i<len;i++)
	{
		LTPS_CurSet(239-x,319-y);
		DataWrite(Pix_16bit);
		x++;
	}
}



void LCD_DrawVerLine(UINT16 x,UINT16 y,UINT16 len,UINT16 Pix_16bit)
{
	int i = 0;
	LTPS_Window(0,0,239,319);
	for(i=0;i<len;i++)
	{
		LTPS_CurSet(239-x,319-y);
		DataWrite(Pix_16bit);
		y++;
	}
}

//for test
void LTPSFull_part(UINT16 Pix_16bit)
{
	unsigned int i, j ;
	 LTPS_Window(239-40,319-40,239-20,319-20);
   LTPS_CurSet(239-20,319-20);
   j = 0;
   while(j<20)
   {
    i=0;
    while(i<20) 
     {
      DataWrite(Pix_16bit);
      i++;
     }
    j++;
   } 
	
}

long LCD_Write(UINT16 topLeftX, UINT16 topLeftY, UINT16 width, UINT16 height, UINT16 *picBuf)
{
   unsigned int i, j ;
   int lcdInterface;
   long len;
   len = width * height;

   LTPS_Window(239-(topLeftX+width-1),319-(topLeftY+height-1),239-topLeftX,319-topLeftY);
   LTPS_CurSet(239-topLeftX,319-topLeftY);
   j = 0;
   while(j<width)
   {
    i=0;
    while(i<height) 
     {
//     	printf("*picBuf:%x\n",*picBuf);
      DataWrite(*picBuf);
      i++;
      picBuf++;
     }
    j++;
   } 
   
   
/*
   j=0;
		   	  
   lcdInterface = (int)0xac000000;
   len = width * height;
   i = len - len%16;
	 
		  __asm
			  {
			loop_lwrite:
			  	LH t0, [picBuf], #2
			  	LH t1, [picBuf], #2
			  	SH t0, [lcdInterface, #4]
			  	LH t2, [picBuf], #2
			  	SH t1, [lcdInterface, #4]
			  	LH t3, [picBuf], #2	
			  	SH t2, [lcdInterface, #4]
			  	
			  	LH t0, [picBuf], #2
			  	SH t3, [lcdInterface, #4]
			  	LH t1, [picBuf], #2
			  	SH t0, [lcdInterface, #4]
			  	LH t2, [picBuf], #2
			  	SH t1, [lcdInterface, #4]
			  	LH t3, [picBuf], #2
			  	SH t2, [lcdInterface, #4]
			  	
			  	LH t0, [picBuf], #2
			  	SH t3, [lcdInterface, #4]
			  	LH t1, [picBuf], #2
			  	SH t0, [lcdInterface, #4]
			  	LH t2, [picBuf], #2
			  	SH t1, [lcdInterface, #4]
			  	LH t3, [picBuf], #2
			  	SH t2, [lcdInterface, #4]
			  	
			  	LH t0, [picBuf], #2
			  	SH tp3, [lcdInterface, #4]
			  	LH t1, [picBuf], #2
			  	SH t0, [lcdInterface, #4]
			  	LH t2, [picBuf], #2
			  	SH t1, [lcdInterface, #4]
			  	LH t3, [picBuf], #2
			  	SH t2, [lcdInterface, #4]
			  	
				subu i, i, 16
				SH t3, [lcdInterface, #4]
				bne	loop_lwrite
			  }
			 */
		return len;
} 



void lcdlight(char flag)
{
	
	GPFSH2 &= 0xcfffffff;	
	GPDIR2 |= 0x40000000;
	GPD2   |= 0x40000000;
}

/**************************************************************************************/
/************************      LCD APP function      **********************************/
/**************************************************************************************/ 

static int dma_chan;

static void slcd_dma_irq(int irq, void *dev_id, struct pt_regs *regs)
{
	printf("slcd_dma_irq %d\n", irq);
	dump_jz_dma_channel(dma_chan);

	if (__dmac_channel_transmit_halt_detected(dma_chan)) {
		printf("DMA HALT\n");
		__dmac_channel_clear_transmit_halt(dma_chan);
	}

	if (__dmac_channel_address_error_detected(dma_chan)) {
		printf("DMA ADDR ERROR\n");
		__dmac_channel_clear_address_error(dma_chan);
	}

	if (__dmac_channel_descriptor_invalid_detected(dma_chan)) {
		printf("DMA DESC INVALID\n");
		__dmac_channel_clear_descriptor_invalid(dma_chan);
	}

	if (__dmac_channel_count_terminated_detected(dma_chan)) {
		printf("DMA CT\n");
		__dmac_channel_clear_count_terminated(dma_chan);
	}

	if (__dmac_channel_transmit_end_detected(dma_chan)) {
		printf("DMA TT\n");
		REG_DMAC_DCCSR(dma_chan) &= ~DMAC_DCCSR_EN;  /* disable DMA */
		__dmac_channel_clear_transmit_end(dma_chan);
	}
}

//open the LCD      
long LCD_Open(int * pCtl) //need modify
{
//	 lcdlight(0);
//	 InitLCDgpio();
	 LcdInit();
	 return 1;
}



char* bufferchangebyte(char* buffer)
{
	int n = 0;
	char temchar = 0;
	for(n=0;n<240*320;n++)
	{
		temchar = buffer[2*n];
		buffer[2*n]=buffer[2*n+1];
		buffer[2*n+1]=temchar;
	}
	return buffer;
}

extern void dump_jz_dma_channel(unsigned int dmanr);

void lcdtest(int dma)
{
	int i;

	printf("Lcd test\n");

#if 0
	//dma_enable = 1;
	dma_enable = 0;
	REG_SLCD_CTRL = SLCD_CTRL_DMA_EN;
#endif

	REG_LCD_REV |= 64;

	/* reset buffer index */
	dma_ptr = (volatile unsigned int *)aaaaa;
	dma_idx = 0;

	 LCD_Open(0);		/* Cmd_Write need delay, use cpu mode */

#if 1
	memset(sssss, 0x7e0, 240 * 80 * 2);
	memset(sssss + 240 * 80 * 2, 0x206, 240 * 80 * 2);
	memset(sssss + 240 * 80 * 2 * 2, 0x6010, 240 * 80 * 2);
	memset(sssss + 240 * 80 * 2 * 3, 0x1d1f, 240 * 80 * 2);
#endif

	LTPSFull(0xffff);

//	while(1)
	{
//		printf("sizeofbuffer:%d\n",sizeof(img_buffer));
//		LCD_Write(0,00,240,300,sssss);
#if 1
		dma_enable = 1;
		REG_SLCD_CTRL = SLCD_CTRL_DMA_EN;
#endif
		//LCD_Write(0,0,20,240,sssss);
		LCD_Write(0,0,240,320,sssss);

//		LTPSFull_part(0x7e0);
//		LTPSFull(0x001f);
	
//		LCD_DrawPixel(239,0,0x7e0);

//		LCD_DrawHorLine(0,100,120,0x7e0);
//		mdelay(1000);

//		LCD_DrawHorLine(0,120,240,0x7f0);
//		LCD_DrawVerLine(120,0,320,0x31f);
	}

	if (dma_enable) {
		int i;

		printf("Buffer ready, start DMA\n");
		printf("dma_idx = %d\n", dma_idx);

		dma_ptr = (volatile unsigned int *)aaaaa;
		for (i = 0; i < 64; i++) {
			//printk("%08x\n", *dma_ptr++);
		}

		dma_cache_wback_inv((unsigned long)aaaaa, 4096 * (2 ^ 9));

		REG_DMAC_DCCSR(dma_chan) = 0;
		REG_DMAC_DRSR(dma_chan) = DMAC_DRSR_RS_SLCD;
		REG_DMAC_DSAR(dma_chan) = PHYSADDR((unsigned long)aaaaa);
		REG_DMAC_DTAR(dma_chan) = PHYSADDR(SLCD_FIFO);
		REG_DMAC_DTCR(dma_chan) = dma_idx;
//	REG_DMAC_DCMD(dma_chan) = DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BIT | DMAC_DCMD_TIE;
		REG_DMAC_DCMD(dma_chan) = DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BIT;

		REG_DMAC_DCCSR(dma_chan) = DMAC_DCCSR_NDES | DMAC_DCCSR_EN;
		REG_DMAC_DMACR = DMAC_DMACR_DMAE;

		printf("SLCD_CTRL=0x%x\n", REG_SLCD_CTRL);
		
		printf("DMA started. IMR=%08x\n", REG_INTC_IMR);
		printf("SLCD_CTRL=0x%x\n", REG_SLCD_CTRL);
		dump_jz_dma_channel(dma_chan);
		printf("SLCD_CTRL=0x%x\n", REG_SLCD_CTRL);

		while (!(REG_DMAC_DCCSR(dma_chan) & DMAC_DCCSR_TT))
			;
		printf("DMA TX ended\n");
		dump_jz_dma_channel(dma_chan);
		REG_DMAC_DCCSR(dma_chan) = 0;
	}
}


/*
 * Module init and exit
 */

static int slcd_init(void)
{
	struct slcd_device *dev;
	int ret;

	/* Request DMA channel and setup irq handler */
//	dma_chan = jz_request_dma(DMA_ID_AUTO, "auto", slcd_dma_irq,
//				  SA_INTERRUPT, NULL);
	dma_chan=0;
	dma_request(dma_chan, slcd_dma_irq, 0,
		    DMAC_DCMD_SAI | DMAC_DCMD_RDIL_IGN |  DMAC_DCMD_DWDH_16,
		    DMAC_DRSR_RS_SLCD);

	printf("Requested DMA channel = %d\n", dma_chan);

	InitLCDgpio();

	lcdtest(0);

	return 0;
}

static void slcd_exit(void)
{
	dma_stop(dma_chan);

}

#endif
