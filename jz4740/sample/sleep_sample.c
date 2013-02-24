#include <bsp.h>
#include <jz4740.h>
#include <key.h>
#include <mipsregs.h>

extern PFN_KEYHANDLE UpKey;
extern PFN_KEYHANDLE DownKey;



static PFN_KEYHANDLE oldKeyHandle = 0;
static PFN_KEYHANDLE oldKeyHandle1 = 0;

static void jz_sleep(u32 key)
{
    u32 intc_mr,i;
    unsigned int curtime;
	
	if(key & 1)
	{
		intc_mr = REG_INTC_IMR;
		REG_INTC_IMSR = -1;
        REG_INTC_IMCR = (1 << (IRQ_GPIO0 - KEY_PIN / 32));
		for(i = 0; i < KEY_NUM; i++)
		{

			__gpio_ack_irq(KEY_PIN + i);
			__gpio_unmask_irq(KEY_PIN + i);
			
		}
		
		printf("sleep 0x%08x\r\n",intc_mr);
		serial_waitfinish();
		
		jz_pm_sleep();

		for(i = 0; i < KEY_NUM; i++)
			__gpio_mask_irq(KEY_PIN + i);
			
		
		REG_INTC_IMCR = (~intc_mr);
		
		printf("wake up 0x%08x\r\n",REG_INTC_IMR);
	}
	if(key & 0x20)
	{
		cli();
		
		intc_mr = REG_INTC_IMR;
		REG_INTC_IMSR = -1;
        REG_INTC_IMCR = (1 << (IRQ_GPIO0 - KEY_PIN / 32));
		REG_INTC_IMCR = (1 << IRQ_RTC);
		for(i = 0; i < KEY_NUM; i++)
		{

			__gpio_ack_irq(KEY_PIN + i);
			__gpio_unmask_irq(KEY_PIN + i);
			
		}
		
		
		while(!__rtc_write_ready());
		
		curtime = REG_RTC_RSR;
		curtime += 30;
		
		printf("wake up at 30s after sleep %x",curtime);

		__rtc_clear_alarm_flag();
		
		__rtc_set_alarm_second( curtime);
		__rtc_enable_alarm_irq();
		__rtc_enable_alarm();
		printf("sleep up time 0x%08x\r\n",REG_RTC_RSR);
		serial_waitfinish();

		jz_pm_sleep();
				
		for(i = 0; i < KEY_NUM; i++)
			__gpio_mask_irq(KEY_PIN + i);
		
		while(!__rtc_write_ready());
		printf("sleep up time 0x%08x\r\n",REG_RTC_RSR);	
		__rtc_clear_alarm_flag();
		__intc_ack_irq(IRQ_RTC);
		__rtc_disable_alarm_irq();
		REG_INTC_IMCR = (~intc_mr);
		printf("wake up 0x%08x\r\n",REG_INTC_IMR);
		sti();
		
	}
	
	if(oldKeyHandle) oldKeyHandle(key);
	
}


static void jz_idle()
{
    u32 prog_entry = ((u32)jz_idle / 32 - 1) * 32 ;
	u32 intc_mr;
	u32 prog_size = 1 * 1024;
    register u32 i;
    unsigned int t;
	t = read_c0_status();
	write_c0_status(t & (~1));
    for( i = (prog_entry);i < prog_entry + prog_size; i += 32)
		__asm__ __volatile__(
			"cache 0x1c, 0x00(%0)\n\t"
            :
			: "r" (i));

	intc_mr = REG_INTC_IMR;
   
	REG_EMC_DMCR |= (1 << 25);
    REG_INTC_IMSR = -1;
	REG_INTC_IMCR = (1 << (IRQ_GPIO0 - KEY_PIN / 32));
    
	for(i = 0; i < KEY_NUM; i++)
	{
		
		__gpio_ack_irq(KEY_PIN + i);
		__gpio_unmask_irq(KEY_PIN + i);
		
	}
	
   
	__asm__(
        ".set\tmips3\n\t"
        "wait\n\t"
		".set\tmips0"
        );
     
     REG_EMC_DMCR &= (~(1 << 25));
	// 90 ns				 
	for(i = 0; i < 30;i++) 
		__asm__ volatile("nop\n\t");
	write_c0_status(t);
	 /* Restore to IDLE mode */    
}

static void Idle(u32 key)
{
    u32 intc_mr,i;
    
	if(key & 2)
	{
		intc_mr = REG_INTC_IMR;
		REG_INTC_IMSR = -1;
        REG_INTC_IMCR = (1 << (IRQ_GPIO0 - KEY_PIN / 32));
		for(i = 0; i < KEY_NUM; i++)
		{

			__gpio_ack_irq(KEY_PIN + i);
			__gpio_unmask_irq(KEY_PIN + i);
			
		}
		
		printf("idle in 0x%08x\r\n",intc_mr);
	    printf("REG_EMC_RTCOR = 0x%08x\r\n",REG_EMC_RTCOR);

		serial_waitfinish();


		jz_idle();
		
		//jz_pm_sleep();
        for(i = 0; i < KEY_NUM; i++)
			__gpio_mask_irq(KEY_PIN + i);
			
		
		REG_INTC_IMCR = (~intc_mr);
		
		printf("idle out 0x%08x\r\n",REG_INTC_IMR);
	}
	if(oldKeyHandle1) oldKeyHandle1(key);
	
}



void SleepTest()
{
	
	//REG_GPIO_PXDATS(p)
	oldKeyHandle = DownKey;
	DownKey = jz_sleep;
	oldKeyHandle1 = UpKey;
	UpKey = Idle;
	__rtc_set_second(0);
	while(!__rtc_write_ready());
	REG_RTC_RGR = 0x7fff;
	
	__rtc_enabled();
	__rtc_clear_alarm_flag();
	__rtc_disable_alarm_irq();
	
}
