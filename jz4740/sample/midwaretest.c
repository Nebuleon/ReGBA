#include <bsp.h>
#include <jz4740.h>
#include "midware.h"

static MIDOBJ midobj;

u32 MidNotify(u32 dat)
{
	printf("Applcation layer get information : %d \n",dat);
	return 1;
}

void MidwareTest()
{

	RunMidWare();       //run midware first!

	midobj.Notify = MidNotify;          //then register uplayer OBJ
	RegisterMidObj(& midobj);
	printf("Register Midware Obj! %x \n",& midobj);

	InitUdcNand();
	InitUdcMMC();
	KeyInit();
	mmc_detect_init();
	power_off_init();           //add event source device!


	midobj.SetLcdBLTime(500);   //set LCD off time !
	printf("Set LCD time %d second !\n",5);

}
