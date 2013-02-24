#include <jz4740.h>
#include <ucos_ii.h>
#include <jztouch.h>

#define TS_AD_COUNT             5
#define M_SADC_CFG_SNUM_5	    ((TS_AD_COUNT - 1) << SADC_CFG_SNUM_BIT)

#define SADC_CFG_INIT (   \
                             (2 << SADC_CFG_CLKOUT_NUM_BIT) |  \
						                SADC_CFG_XYZ1Z2		   		   |  \
									    SADC_CFG_SNUM_5                |  \
									    (1 << SADC_CFG_CLKDIV_BIT)    |  \
						                SADC_CFG_PBAT_HIGH             |  \
									    SADC_CFG_CMD_INT_PEN )
									    
static unsigned char run_flag = 0;
static u16 ts_TimeOut = 0;


#define TASK_STK_SIZE	1024
static OS_STK SADCTaskStack[TASK_STK_SIZE];
static OS_EVENT *sadcEvent;
static OS_EVENT *batteryEvent;
#define TOUCH_TASK_PRIO	5

static PFN_SADC touchFunction = 0;
static PFN_SADC sadcFunction = 0;
static PFN_SADC batFunction = 0;
static unsigned int batCountperSec = 0;
static unsigned int sadcCountperSec = 0;
static unsigned char dCount = 0;
unsigned short tsxData = -1,tsyData = -1;
static unsigned pendown_flag = 0;

static void ReadTsData()
{
	unsigned int dat;
	unsigned short xData,yData;
	short tsz1Data,tsz2Data;
    
	
	dat = REG_SADC_TSDAT;
	
	xData = (dat >>  0) & 0xfff;
	yData = (dat >> 16) & 0xfff;
	
    //printf("%d -> %d %d\r\n",dCount,xData,yData);		
	dat = REG_SADC_TSDAT;
    tsz1Data = (dat >>  0) & 0xfff;
	tsz2Data = (dat >> 16) & 0xfff;
	//printf("%d -> %d %d\r\n",dCount,tsz1Data,tsz2Data);		
	tsz1Data = tsz2Data - tsz1Data;
	if(pendown_flag != 1)
		return ;
	
	
	if((tsz1Data > 15) || (tsz1Data < -15))
    {
		if(tsxData == (unsigned short)-1)
			tsxData = xData;
		else
			tsxData = (tsxData + xData) / 2;
		
		if(tsyData == (unsigned short)-1)
			tsyData = yData;
		else
			tsyData = (tsyData + yData) / 2;
		
    }

//	printf("0x%08x \r\n",dat);	
    dCount ++;
	
	if(dCount > TS_AD_COUNT - 1)
	{
		if(tsxData != (unsigned short) -1)
		{
			
		    if(touchFunction)
			{
				dat = tsxData + (tsyData << 16);
				touchFunction((unsigned short *)&dat);
			}
			//printf("x: 0x%d y: 0x%d\r\n",tsxData,tsyData);	
			tsxData = -1;
			tsyData = -1;
		}
		
		dCount = 0;
	}
	
}
static void touchTaskEntry(void *arg)
{
		unsigned char state;
		unsigned char sadcstat;
		unsigned char err;
		
		while(run_flag)
		{
			OSSemPend(sadcEvent,ts_TimeOut,&err);
			ts_TimeOut = 0;
			sadcstat = REG_SADC_STATE;
			state = REG_SADC_STATE & (~REG_SADC_CTRL);
			REG_SADC_STATE &= sadcstat;
					
			if(state & SADC_CTRL_PENDM)
			{
				  //printf("pen down irq \r\n");
				  REG_SADC_CTRL &= (~(SADC_CTRL_PENUM |  SADC_CTRL_TSRDYM));
				  REG_SADC_CTRL |= (SADC_CTRL_PENDM);// | SADC_CTRL_TSRDYM);
				  pendown_flag = 1;
			}
			if(state & SADC_CTRL_PENUM)
			{
				//printf("pen up irq \r\n");
				REG_SADC_CTRL &= (~SADC_CTRL_PENDM );
				REG_SADC_CTRL |= SADC_CTRL_PENUM;
				pendown_flag = 0;
				tsxData = -1;
				tsyData = -1;
				if(touchFunction)
				{
					unsigned int dat;
					
					dat = tsxData + (tsyData << 16);
					touchFunction((unsigned short *)&dat);
				}
			    //printf("x: 0x%d y: 0x%d\r\n",tsxData,tsyData);	
			}
			if(state & SADC_CTRL_TSRDYM)
			{
				//printf("touch ad irq 0x%x \r\n",sadcstat);
					ReadTsData();
					
			}
			if(state & SADC_CTRL_PBATRDYM)
			{
				printf("battery ad irq\r\n");
			}
			if(state & SADC_CTRL_SRDYM)
			{
				printf("sad ad irq\r\n");
				
			}
			
			__intc_unmask_irq(IRQ_SADC);
		}
		
}


static void handler(unsigned int arg)
{
	__intc_mask_irq(IRQ_SADC);
//	printf("IRQ_SADC");
	
	OSSemPost(sadcEvent);
}

static int SADC_Init()
{
	REG_SADC_ENA = 0;
	REG_SADC_STATE &= (~REG_SADC_STATE);
	REG_SADC_CTRL = 0x1f;
	
	sadcEvent = OSSemCreate(0);
	//__cpm_start_sadc();
	REG_SADC_CFG =  SADC_CFG_INIT;

	run_flag = 1;
	request_irq(IRQ_SADC, handler, 0);
	
	OSTaskCreate(touchTaskEntry, (void *)0,
		     (void *)&SADCTaskStack[TASK_STK_SIZE - 1],
		     TOUCH_TASK_PRIO);
	REG_SADC_SAMETIME = 1;
	REG_SADC_WAITTIME = 1000; //per 100 HZ
	ts_TimeOut = 0;
	REG_SADC_STATE &= (~REG_SADC_STATE);
	REG_SADC_CTRL &= (~(SADC_CTRL_PENDM | SADC_CTRL_TSRDYM)); // 
	REG_SADC_ENA = SADC_ENA_TSEN; // | REG_SADC_ENA;//SADC_ENA_TSEN | SADC_ENA_PBATEN | SADC_ENA_SADCINEN;
	
}

static int SADC_DeInit()
{
	REG_SADC_ENA = 0;
	__intc_mask_irq(IRQ_SADC);
	OSTimeDly(20);
	run_flag = 0;
	__cpm_stop_sadc();
}


void initTouch(PFN_SADC touchfn)
{
	touchFunction = touchfn;
	//printf("++++++++++++++++++++%s\n", __FUNCTION__);
	
}
void initBattery(int countpersec,PFN_SADC batfn)
{
	batCountperSec = countpersec;
	batFunction = batfn;
}
void initSadc(int countpersec,PFN_SADC sadcfn)
{
	sadcCountperSec = countpersec;
	sadcfn = sadcfn;
}
int TS_init(void)
{
	return SADC_Init();
}

/********* battery start ************************************************/

u16 pbat; //battery voltage

/*
 *  battery voltage int handle
 */
static void handler_battery(unsigned int arg)
{
	unsigned char state;

	__intc_mask_irq(IRQ_SADC);
	state = REG_SADC_STATE & (~REG_SADC_CTRL);
	if(state & SADC_CTRL_PBATRDYM)
	{
		pbat = REG_SADC_BATDAT;
		OSSemPost(batteryEvent);
		REG_SADC_STATE = SADC_STATE_PBATRDY;
	}
	__intc_unmask_irq(IRQ_SADC);
}

/*
 * Read the battery voltage
 */

static unsigned int jz4740_read_battery(void)
{
	if(!(REG_SADC_STATE & SADC_STATE_PBATRDY) ==1)
		start_pbat_adc();
}

/* 
 * set adc clock to 12MHz/div. A/D works at freq between 500KHz to 6MHz.
 */
static void sadc_init_clock(int div)
{
	if (div < 2) div = 2;
	if (div > 23) div = 23;
	
	REG_SADC_CFG &= ~SADC_CFG_CLKDIV_MASK;
	REG_SADC_CFG |= (div - 1) << SADC_CFG_CLKDIV_BIT;
}

static void start_pbat_adc(void)
{
	REG_SADC_CFG |= SADC_CFG_PBAT_HIGH ;   /* full baterry voltage >= 2.5V */
//  	REG_SADC_CFG |= SADC_CFG_PBAT_LOW;    /* full baterry voltage < 2.5V */
  	REG_SADC_ENA |= SADC_ENA_PBATEN;      /* Enable pbat adc */
}


int Battery_Init()
{
	batteryEvent = OSSemCreate(0);
	sadc_init_clock(3);
	request_irq(IRQ_SADC,handler_battery, 0);
}

int Read_Battery(void)
{
	unsigned char err;

	jz4740_read_battery();
	OSSemPend(batteryEvent,100,&err);

	return pbat;
}

/************ battery end ************************************************/
