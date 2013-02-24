#include "ucos_ii.h"
#include "jz4740.h"
#include "midware.h"
#include "fs_api.h"

#define JZ4740_PAV_1_0 0
#define JZ4740_PAV_1_1 0
#define JZ4740_PAV_1_2 1
#define CARD_IN        0
#define CARD_OUT       1 

#if JZ4740_PAV_1_0
#define MMC_CD_PIN    (16 + 3 * 32)	/* Pin to check card insertion */
#define MMC_POWER_PIN (15 + 3 * 32)	/* Pin to enable/disable card power */
#endif


#if JZ4740_PAV_1_1
#define MMC_CD_PIN    (15 + 3 * 32)	/* Pin to check card insertion */
#define MMC_POWER_PIN (16 + 3 * 32)	/* Pin to enable/disable card power */
#endif

#if JZ4740_PAV_1_2
#define MMC_CD_PIN    (14 + 3 * 32)	/* Pin to check card insertion */
#define MMC_POWER_PIN (17 + 3 * 32)	/* Pin to enable/disable card power */
#define MMC_PW_PIN    (14 + 3 * 32)	/* Pin to check protect card */
#endif

#define MMC_GPIO_TASK_PRIO	21
#define MMC_GPIO_TASK_STK_SIZE       1024
static OS_STK MGTaskStack[MMC_GPIO_TASK_STK_SIZE];

volatile static MIDSRC mmcsrc;
volatile static u8 cardstate = CARD_OUT;
static u32 mmcid;
static OS_EVENT *MMCGPIOEvent;
static u8 cardexsit = 0;

static void info_card_in()
{
	mmcsrc.Src = SRC_MMC;
	mmcsrc.Event = EVENT_MMC_IN;
	OSQPost(mmcsrc.CurEvent1 , (void *)&mmcid);
	OSSemPost(mmcsrc.CurEvent);
}

static void info_card_out()
{
	mmcsrc.Src = SRC_MMC;
	mmcsrc.Event = EVENT_MMC_OUT;
	OSQPost(mmcsrc.CurEvent1 , (void *)&mmcid);
	OSSemPost(mmcsrc.CurEvent);
}

//SD/MMC  hotplug!!
void mmc_gpio_irq_handler(unsigned int arg)
{
	__gpio_mask_irq(MMC_CD_PIN);
	OSSemPost(MMCGPIOEvent);
}

static void GetRequest(MIDSRCDTA *dat)
{
//	printf("Up layer get :%d \n",res.Val);
}

static void Response(MIDSRCDTA *dat)
{
	if (dat->Val == 1 && cardstate == CARD_IN )
		cardexsit = 0 ;
	else
		cardexsit = 1 ;

	if ( cardstate == CARD_IN )
		FS_IoCtl("mmc:",FS_CMD_MOUNT,0,0);
	else
		FS_IoCtl("mmc:",FS_CMD_UNMOUNT,0,0);
//	MMC_Initialize();
}

static void MMCGpioTask(void *arg)
{
	u8 err;
	cardstate = CARD_OUT;
	cardexsit = 1 ;
	while(1)
	{
//		__intc_mask_irq(48 + MMC_CD_PIN);
		printf("Looks like MMC gpio change! \n");
		if ( cardstate == CARD_OUT )     //card have inserted!
		{
			OSTimeDlyHMSM(0,0,0,500);
			if ( __gpio_get_pin(MMC_CD_PIN) == 0 ) //card readlly insert!
			{
				printf("Card readlly insert! \n");
				cardstate = CARD_IN;
				info_card_in();
				MMC_Initialize();
				__gpio_as_irq_rise_edge(MMC_CD_PIN);
			}
			else
				__gpio_as_irq_fall_edge(MMC_CD_PIN);
		}
		else                            //card have not inserted!
		{
			OSTimeDlyHMSM(0,0,0,500);
			if ( __gpio_get_pin(MMC_CD_PIN) == 1 ) //card readlly out!
			{
				printf("Card readlly out! \n");
				cardstate = CARD_OUT;
				info_card_out();
				__gpio_as_irq_fall_edge(MMC_CD_PIN);
			}
			else
				__gpio_as_irq_rise_edge(MMC_CD_PIN);

		}
		__gpio_ack_irq(MMC_CD_PIN);
		__intc_ack_irq(48 + MMC_CD_PIN);
		__gpio_unmask_irq(MMC_CD_PIN);
		OSSemPend(MMCGPIOEvent, 0, &err);
	}
}
 
int MMC_DetectStatus(void)
{
printf("mmc_hotplug.c\n");
#if 0
	if (jz_mmc_chkcard())
		return 0;	/* card exits */

	return 1;		/* card does not exit */
#else
//	printf("cardstate %d \n",cardstate);
	return cardexsit;                //1:in 0:out

#endif
}

void mmc_detect_init()
{
	MMCGPIOEvent = OSSemCreate(0);

	__gpio_mask_irq(MMC_CD_PIN);
	__gpio_as_input(MMC_CD_PIN);
	__gpio_disable_pull(MMC_CD_PIN);
	request_irq(48 + MMC_CD_PIN, mmc_gpio_irq_handler, 0);
	__gpio_unmask_irq(MMC_CD_PIN);
	cardexsit = 0;
	mmcsrc.GetRequest = GetRequest;
	mmcsrc.Response = Response;
	mmcsrc.Name = "MMC";
	printf("Register Midware SRC MMC! \n");
	RegisterMidSrc(&mmcsrc);
	mmcid = mmcsrc.ID;
	printf("mmc ID %d \n",mmcsrc.ID);

	MMC_Initialize();
	OSTaskCreate(MMCGpioTask, (void *)0,
		     (void *)&MGTaskStack[MMC_GPIO_TASK_STK_SIZE - 1],
		     MMC_GPIO_TASK_PRIO);

}
