#include <bsp.h>
#include <jz4740.h>
#include <key.h>
#include <mipsregs.h>

extern void mdelay(unsigned int msec);

extern PFN_KEYHANDLE UpKey;
extern PFN_KEYHANDLE DownKey;


#define LED_PIN (32 * 3 + 28)

static PFN_KEYHANDLE oldPowerKeyHandle = 0;

void jz_hibernate(u32 key)
{
    
	if(key & 4)
	{
		printf("Power Down\r\n");
		serial_waitfinish();
		jz_pm_hibernate();
		
	}
	oldPowerKeyHandle(key);
	
}

void HibernateTest()
{
	int m= 5;
	
	__gpio_as_output(LED_PIN);
	while(m--)
	{
	    __gpio_set_pin(LED_PIN);
	    mdelay(100);
	    __gpio_clear_pin(LED_PIN);
	    mdelay(100);
	}
    
	oldPowerKeyHandle = DownKey;
	DownKey = jz_hibernate;
	
}
