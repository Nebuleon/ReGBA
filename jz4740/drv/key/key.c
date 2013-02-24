#include <bsp.h>
#include <jz4740.h>
#include <ucos_ii.h>
#include <key.h>

#ifdef USE_MIDWARE
#include "midware.h"
#endif

#define KEY_TASK_STK_SIZE	1024
#define KEY_TASK_PRIO	1

static OS_EVENT *key_sem;
static OS_STK KeyTaskStack[KEY_TASK_STK_SIZE];
extern void KeydownInfo();
extern u8 LcdState;
PFN_KEYHANDLE UpKey = NULL;
PFN_KEYHANDLE DownKey = NULL;
//PFN_KEYHANDLE UpKeyHandle = NULL;
//PFN_KEYHANDLE DownKeyHandle = NULL;

#define POWER_OFF_PIN    (29 + 3 * 32)	//GPD29
#define POFF_TASK_STK_SIZE       128
#define POFF_EACH_TIME    100
#define POFF_TRY_TIME     20


#define KEY_DETECT_REPEAT_TIME 500
#define KEY_REPEAT_TIME 100
#define KEY_DELAY_TIME 10

#ifdef USE_MIDWARE
volatile static MIDSRC keysrc;
volatile static MIDSRCDTA res;
volatile static u32 keyid;

volatile static MIDSRC poffsrc;
volatile static MIDSRCDTA res;
//volatile static u8 cardstate ;
static u32 poffid;

static void info_power_off()
{
	keysrc.Src = SRC_POWER_OFF;
	keysrc.Event = EVENT_POWER_OFF;
	OSQPost(keysrc.CurEvent1 , (void *)&keyid);
	OSSemPost(keysrc.CurEvent);
}

static void GetRequest(MIDSRCDTA *dat)
{
//	printf("Up layer get :%d \n",res.Val);
}

static void Response(MIDSRCDTA *dat)
{
	if ( dat->Val == 1 )
	{
		printf("Check can power off or not ! \n");
		ShutLcd();
		while( !(__gpio_get_pin(POWER_OFF_PIN)) );
                //wait until key up
		OSTimeDlyHMSM(0,0,0,200);
		if ( __gpio_get_pin(POWER_OFF_PIN) == 1 )  //really power off
		{
			jz_pm_hibernate();
		}
	}
	else
		printf("Can not power off ! \n");
}

void check_poweroff()
{
	int i;
	if ( __gpio_get_pin(POWER_OFF_PIN) == 0 )
	{
		printf("Power key push ! \n");
		OSQPost(keysrc.CurEvent1 , (void *)&keyid);
		keysrc.Src = SRC_KEY;
		OSSemPost(keysrc.CurEvent);
	}
	for ( i = 0; i < POFF_TRY_TIME ; i ++ )
	{
		OSTimeDlyHMSM(0,0,0,POFF_EACH_TIME);
		if ( __gpio_get_pin(POWER_OFF_PIN) ==1 )
			break;
		printf("Power key keep low! \n");
	}
	if ( i >= POFF_TRY_TIME )  //really power off!
	{
		info_power_off();
	}
}
#endif

static UpKeyHandle(key) 
{
	if(UpKey) UpKey(key);
}

static DownKeyHandle(key) 
{
	u8 err;

#ifdef USE_MIDWARE
	printf("Key Down ! \n");
	OSQPost(keysrc.CurEvent1 , (void *)&keyid);
	keysrc.Src = SRC_KEY;
	OSSemPost(keysrc.CurEvent);
	if (LcdState == 0)
#endif
		if(DownKey) DownKey(key);
}

#if 0
static void key_interrupt_handler(u32 arg)
{
	for(arg = 0;arg < KEY_NUM;arg++)
		__gpio_mask_irq(KEY_PIN + arg);
		__gpio_mask_irq(POWER_OFF_PIN);
	OSSemPost(key_sem);
}
#endif 

static void KeyTaskEntry(void *arg)
{
	u8 err;
	u16 i,run,count = 0;
	u32 key,oldkey;
	u32 upkey = 0;
	u32 keyrepeat = KEY_DETECT_REPEAT_TIME / KEY_DELAY_TIME;
	
	printf("Key Install \r\n");
	
	while(1)
	{
		OSSemPend(key_sem, 0, &err);
#ifdef USE_MIDWARE
		if ( __gpio_get_pin(POWER_OFF_PIN) == 0 )
		{
			check_poweroff();
			goto out;
		}
#endif
		oldkey = (~REG_GPIO_PXPIN(KEY_PIN / 32)) & KEY_MASK;
		run = 1;
		count = 0;
		keyrepeat = KEY_DETECT_REPEAT_TIME / KEY_DELAY_TIME;
		while(run)
		{
			
			OSTimeDly(KEY_DELAY_TIME);
			key = (~REG_GPIO_PXPIN(KEY_PIN / 32)) & KEY_MASK;
			//printf("reg = 0x%8x key = 0x%x oldkey = 0x%x\r\n",REG_GPIO_PXPIN(KEY_PIN / 32),key,oldkey);
			
			if(key ^ oldkey)
			{
				
				oldkey = key;
				continue;
			}
			
			else
			{
				if(key)
				{
					
					if(key & (~upkey))
					{
						
						DownKeyHandle(key & (~upkey));
					}
					
					else
					{
						if((key ^ upkey) & upkey)
							UpKeyHandle((key ^ upkey) & upkey);
					}
					if(key == upkey)
					{
						count++;
					
						if(count > keyrepeat)
						{
							count = 0;
						    upkey = 0;
							keyrepeat = KEY_REPEAT_TIME / KEY_DELAY_TIME;
						}
					}else
					{
						count = 0;
						//UpKeyHandle(key);
					    upkey = key;
					}
					
				}else
				{
					if(upkey)
						UpKeyHandle(upkey);
					run = 0;
					upkey =0;
					
				}
				
				
			}
			
		}

	out:		
		__gpio_ack_irq(POWER_OFF_PIN);
		__intc_ack_irq(48 + POWER_OFF_PIN);
		__gpio_unmask_irq(POWER_OFF_PIN);
		for(i = 0; i < KEY_NUM; i++)
		{

			__gpio_ack_irq(KEY_PIN + i);
			__gpio_unmask_irq(KEY_PIN + i);
			
		}
		
	}
	
}

#if 0
void KeyInit()
{
	int i;
	key_sem = OSSemCreate(0);
//	UpKeyHandle = UKHandle;
//	DownKeyHandle = DKHandle;

#ifdef USE_MIDWARE
	keysrc.GetRequest = GetRequest;
	keysrc.Response = Response;
	keysrc.Name = "KEY";
	RegisterMidSrc(&keysrc);
	printf("Register Midware SRC Key! %d \n",keysrc.ID);
	keyid = keysrc.ID;

	__gpio_mask_irq(POWER_OFF_PIN);
	__gpio_as_func0(POWER_OFF_PIN);
	__gpio_as_input(POWER_OFF_PIN);
	__gpio_disable_pull(POWER_OFF_PIN);

#endif
	for(i = 0;i < KEY_NUM;i++)
		request_irq(IRQ_GPIO_0 + KEY_PIN + i, key_interrupt_handler, 0);
	request_irq(48 + POWER_OFF_PIN, key_interrupt_handler, 0);

   	OSTaskCreate(KeyTaskEntry, (void *)0,
		     (void *)&KeyTaskStack[KEY_TASK_STK_SIZE - 1],
		     KEY_TASK_PRIO);
	for(i = 0;i < KEY_NUM;i++)
		__gpio_as_irq_fall_edge(KEY_PIN + i);
	__gpio_as_irq_fall_edge(POWER_OFF_PIN);
//	__intc_unmask_irq(48 + POWER_OFF_PIN);
	__gpio_unmask_irq(POWER_OFF_PIN);
}
#endif

static void key_interrupt_handler(u32 arg)
{
/*
	for(arg = 0;arg < KEY_NUM;arg++)
		__gpio_mask_irq(KEY_PIN + arg);
*/
}

void KeyInit()
{
	int i;

	for(i = 0;i < KEY_NUM;i++)
		request_irq(IRQ_GPIO_0 + KEY_PIN + i, key_interrupt_handler, 0);

	for(i = 0;i < KEY_NUM;i++)
		__gpio_as_irq_fall_edge(KEY_PIN + i);
}


