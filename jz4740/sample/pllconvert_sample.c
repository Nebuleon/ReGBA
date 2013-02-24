#include <bsp.h>
#include <jz4740.h>
#include <key.h>
#include <mipsregs.h>

extern PFN_KEYHANDLE UpKey;
extern PFN_KEYHANDLE DownKey;


#define LED_PIN (32 * 3 + 28)
static PFN_KEYHANDLE oldPowerKeyHandle1 = 0;
void Task (void *data)
{
        U8 err;
        U32 count = 0;
        while(1) {

//		printf("Task%c\n", *(char *)data);
		count++;
		if(count > 300)
		{
			count = 0;
                        printf("Task%c\n", *(char *)data); 
		}
                udelay(5000); 		
		udelay(5000); 		
               OSTimeDly(1);                   /* Delay 3000 clock tick */
        }
}


void jz_pllconvert(u32 key)
{
	int i;
#if 1    
	if(key & 2)
	{
		serial_waitfinish();
		udelay(100);
//		jz_pm_control(0);
		jz_pm_pllconvert(30000000,3);
	
	}
	if(key & 4)
	{
		serial_waitfinish();
		udelay(100);
//		jz_pm_control(1);
		jz_pm_pllconvert(25000000,3);
	
	}
	if(key & 8)
	{
		serial_waitfinish();
		udelay(100);
//		jz_pm_control(2);
		jz_pm_pllconvert(360000000,3);
	
	}
	if(key & 32)
	{
		serial_waitfinish();
		udelay(100);
//		jz_pm_control(3);
		jz_pm_pllconvert(420000000,3);
	
	}
#endif
#if 0
	if(key & 8)
	{
		serial_waitfinish();
		udelay(100);
		for (i = 0; i < NO_TASKS; i++)
		{                               
			/* Create NO_TASKS identical asks */
			TaskData[i] = '0' + i;          /* Each task will display its own letter */
			printf("#%d\n",i);
			OSTaskCreate(Task, (void *)&TaskData[i], (void *)&TaskStk[i][TASK_STK_SIZE - 1], i + 1);
		}
	}
	if(key & 16)
	{
		serial_waitfinish();
		udelay(100);
		for (i = 0; i < NO_TASKS; i++)
		{                               
			/* Create NO_TASKS identical asks */
			TaskData[i] = '0' + i;          /* Each task will display its own letter */
			printf("#%d\n",i);
			OSTaskDel(i + 1);
		}
	
	}
#endif
	oldPowerKeyHandle1(key);
}


void PllConvertTest()
{

	*(volatile u32*)0xb3000000 |= 6; 	
	__gpio_as_output(LED_PIN);
	__gpio_clear_pin(LED_PIN);
	oldPowerKeyHandle1 = DownKey;
	DownKey = jz_pllconvert;
	
}
