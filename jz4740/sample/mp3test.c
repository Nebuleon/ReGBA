#include <bsp.h>
#include <jz4740.h>
#include <key.h>
#include <mipsregs.h>
#if 0 //key
extern PFN_KEYHANDLE UpKey;
extern PFN_KEYHANDLE DownKey;

#define LED_PIN (32 * 3 + 28)
static PFN_KEYHANDLE oldPowerKeyHandle1 = 0;

void jz_control(u32 key)
{
	int i;
		if(key & 8)
	{
		serial_waitfinish();
		udelay(100);
		MP3Init();
	}
   
	if(key & 16)
	{
		serial_waitfinish();
		udelay(100);
		MP3Quit();
		
	}
	if(key & 32)
	{
		serial_waitfinish();
		udelay(100);
		pcm_pause();
	}

}

void ControlMp3(void)
{
	__gpio_as_output(LED_PIN);
	__gpio_clear_pin(LED_PIN);
	oldPowerKeyHandle1 = DownKey;
	DownKey = jz_control;
}
#endif
#if 1


void TestMp3_old(void)
{
	printf("Test mp3!\n");
	FS_Init();

	pcm_init(1);

	madplay_main("shenhua.mp3");

}	 


#endif
