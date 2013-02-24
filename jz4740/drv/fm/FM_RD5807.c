//
// Copyright (c) Ingenic Semiconductor Co., Ltd. 2007.
//

#include "FM_RD5807.h"
#include "jz4740.h"

//------------------------------------------------------------------------------
//// Private globals
#define UP		1
#define DOWN 	0
#define  TRUE  1
#define  FALSE  0
#define DEVCLK	11200000
#define I2C_READ 1
#define I2C_WRITE 0
#define TIMEOUT         1000

/* error code */
#define ETIMEDOUT	1
#define ENODEV		2
unsigned int  frequency;//kHZ
unsigned int  pll;


//------------------------------------------------------------------------------

/*
 * I2C bus protocol basic routines
 */
static int i2c_put_data(unsigned char data)
{
	unsigned int timeout = TIMEOUT*10;
	__i2c_write(data);
	__i2c_set_drf();
	while (__i2c_check_drf() != 0);
	while (!__i2c_transmit_ended());
	while (!__i2c_received_ack() && timeout)
		timeout--;
	if (timeout)
		return 0;
	else
		return -ETIMEDOUT;
}

static int i2c_get_data(unsigned char *data, int ack)
{
	int timeout = TIMEOUT*10;

	if (!ack)
		__i2c_send_nack();
	else
		__i2c_send_ack();

	while (__i2c_check_drf() == 0 && timeout)
		timeout--;

	if (timeout) {
		if (!ack)
			__i2c_send_stop();
		*data = __i2c_read();
		__i2c_clear_drf();
		return 0;
	} else
		return -ETIMEDOUT;
}

int i2cucs_read(unsigned char device, unsigned char *buf,
	      int count)
{
	unsigned char *ptr = buf;
	int cnt = count;
	int timeout = 5;

	__i2c_set_clk(DEVCLK, 10000); /* default 10 KHz */
	__i2c_enable();

L_try_again:

	if (timeout < 0)
		goto L_timeout;
	__i2c_send_start();
	if (i2c_put_data( (device << 1) | I2C_READ ) < 0)
		goto device_rerr;
	__i2c_send_ack();	/* Master sends ACK for continue reading */
	while (cnt) {
		if (cnt == 1) {
			if (i2c_get_data(buf, 0) < 0)
				break;
		} else {
			if (i2c_get_data(buf, 1) < 0)
				break;
		}
		cnt--;
		buf++;
	}

	__i2c_send_stop();

	udelay(300); /* wait for STOP goes over. */
	__i2c_disable();

	return count - cnt;

 device_rerr:
 device_werr:
	printf("ERROR: No I2C device 0x%2x.\n", device);
 offset_err:
	timeout --;
	__i2c_send_stop();
	goto L_try_again;

L_timeout:
	printf("Read I2C device 0x%2x failed.\n", device);
	__i2c_send_stop();
	udelay(300); /* wait for STOP goes over. */
	__i2c_disable();
	return -ENODEV;
}
int i2cucs_write(unsigned char device, unsigned char *buf,
	       int count)
{
	int cnt = count;
	int cnt_in_pg;
	int timeout = 5;
	unsigned char *tmpbuf;
	__i2c_set_clk(DEVCLK, 10000); /* default 10 KHz */
	__i2c_enable();

	__i2c_send_nack();	/* Master does not send ACK, slave sends it */
 W_try_again:
	if (timeout < 0)
		goto W_timeout;

	cnt = count;
	tmpbuf = (unsigned char *)buf;

 start_write_page:
	cnt_in_pg = 0;
 	__i2c_send_start();
	if (i2c_put_data( (device << 1) | I2C_WRITE ) < 0)
		goto device_err;
	while (cnt) { 
		if (++cnt_in_pg > 8) {
			__i2c_send_stop();
			mdelay(1);
			goto start_write_page;
		}
		if (i2c_put_data(*tmpbuf) < 0)
			break;
		cnt--;
		tmpbuf++;
	}
	__i2c_send_stop();
	udelay(300); /* wait for STOP goes over. */
	__i2c_disable();
	return count - cnt;
 device_err:
	printf("ERROR: No I2C device 0x%2x.\n", device);
	timeout--;
	__i2c_send_stop();
	goto W_try_again;

 W_timeout:
	printf("Write I2C device 0x%2x failed.\n", device);
	__i2c_send_stop();
	udelay(300); /* wait for STOP goes over. */
	__i2c_disable();
	return -ENODEV;
}

char I2CSim_CommonRead(unsigned char ucSlaveID, unsigned char *pucBuffer, unsigned char ucBytes)
{
	return (char)(i2cucs_read(ucSlaveID, pucBuffer,ucBytes));
}
char I2CSim_CommonWrite(unsigned char ucSlaveID, unsigned char *pucBuffer, unsigned char ucBytes)
{
	return (char)(i2cucs_write(ucSlaveID, pucBuffer,ucBytes));
}
//------------------------------------------------------------------------------
void Sleep(unsigned int time)
{
    OSTimeDly(1);
}

int AICInit(void )
{
	REG_AIC_FR |= 0x00000030;
	REG_AIC_FR &= 0xFFFFFFF9;
	REG_AIC_I2SCR &= 0xFFFFFFFE;
	REG_ICDC_CDCCR1 = 0x28002000;
	REG_ICDC_CDCCR2 &=0xffe0fffc;
	REG_ICDC_CDCCR2 |=0x00130000;
	return (TRUE);
}
void AICDeinit(void )
{
	REG_ICDC_CDCCR1 = 0x0033300;
}
int  FMM_Deinit(void)
{
	AICDeinit();
	ucWritData[3]|=0x40;
	FMM_Write();
	return (TRUE);
}
//------------------------------------------------------------------------------

int FMM_Init( void )
{
	int	bRet = FALSE;

	if ( !AICInit())
		return (FALSE);
		
	ucWritData[3] &=0xBF;
	Get_Pll();
	printf("Write1:%x,Write2:%x,Write3:%x,Write4:%x,Write5:%x\n",ucWritData[0],ucWritData[1],ucWritData[2],ucWritData[3],ucWritData[4]);
	FMM_Write(); 
	Sleep(500);
	FMM_Read();
	printf("Read1:%x,Read2:%x,Read3:%x,Read4:%x,Read5:%x\n",ucReadData[0],ucReadData[1],ucReadData[2],ucReadData[3],ucReadData[4]);

	return ( bRet );
}
//-----------------------------------------------------------------------------
int HLSI_optimal(unsigned int freq)
{
	unsigned char status;
	int temp;
	unsigned char levelhigh, levellow;	
	freq = freq+450;
	Get_Pll(); // Set PLL value High
	status = FMM_Write();  // Send Command
	Sleep(100);
	     
	status = FMM_Read();  // Read status
	
	levelhigh = ((ucReadData[3] >> 4) & 0x0f); // Get ADC value
	freq = freq-450;  
	Get_Pll(); // Set PLL value Low
	status = FMM_Write();
	Sleep(100);  
	     
	status = FMM_Read();
	
	levellow = ((ucReadData[3] >> 4) & 0x0f); // Get ADC value
	  
	if(levelhigh < levellow) 
		temp = 1;
	else 
		temp = 0;
	
	return temp;
}
//------------------------------------------------------------------------------
//Calculate PLL from frequency
void Get_Pll(void)
{
    unsigned char hlsi,temp;
    unsigned int twpll=0;
    hlsi=ucWritData[2]&0x10;
    if(frequency > 108000)
    	frequency = 108000;
    else if (frequency < 87500)
    	frequency = 87500;
    if (hlsi)
        pll=(unsigned int)((float)((frequency + 225 ) * 4000) / (float)32768);    //k
    else
        pll=(unsigned int)((float)((frequency - 225) * 4000) / (float)32768);    //k
	temp = (ucWritData[0] & 0xC0);
	ucWritData[0] = (unsigned char)(pll/256);//Search mode
	ucWritData[0] |= temp;
    ucWritData[1] = (unsigned char)(pll%256);     
}
//------------------------------------------------------------------------------
void Set_Volume(int vol_level)
{
	if(vol_level==0)
		REG_ICDC_CDCCR1 |= 0x00004000;
	else
	{
	    REG_ICDC_CDCCR1 &=0xFFFFBFFF;
		REG_ICDC_CDCCR2 &=0xffe0fffc;
		REG_ICDC_CDCCR2 |=(vol_level<<16);	
	}	
}
int Get_Volume(void)
{
	int vol;
	if (REG_ICDC_CDCCR1 & 0x00004000)
	{
		vol = 0;
		return vol;
	}
	else
	{
		vol = (REG_ICDC_CDCCR2 & 0x001f0000);
		return (vol>>16);	
	}
}
int Get_Max_Vol(void)
{
	return (31);	
}
int Get_Min_Vol(void)
{
	return (0);
}
//Calculate frequency from PLL
void Set_Frequency(int freq)
{
	frequency = freq;
	Get_Pll();
	FMM_Write();
    Get_Status();
	printf("Read1:%x,Read2:%x,Read3:%x,Read4:%x,Read5:%x\n",ucReadData[0],ucReadData[1],ucReadData[2],ucReadData[3],ucReadData[4]);
}
int Get_Frequency(void)
{
    unsigned char hlsi;
    unsigned int npll=0;
    npll=pll;
    hlsi=ucWritData[2]&0x10;
    if (hlsi)
        frequency=(unsigned int)(((float)(npll * 32768) / 4 - 225000)/1000);    //KHz
    else
        frequency=(unsigned int)(((float)(npll * 32768) / 4 + 225000)/1000);    //KHz
	return frequency;
}

//------------------------------------------------------------------------------

int FMM_Read ( void )
{
	int bRet;
	unsigned char temp_l,temp_h; 
	bRet = I2CSim_CommonRead(TEA5767_READ_ADDR, ucReadData, 5);
	
	temp_l = ucReadData[1];
    temp_h = ucReadData[0];
    temp_h &= 0x3f;
    pll = ((temp_h << 8 )| temp_l);
    Get_Frequency();
    
	return (bRet);
}

//------------------------------------------------------------------------------

int FMM_Write(void) 
{
	int bRet = FALSE;
	bRet = I2CSim_CommonWrite(TEA5767_WRITE_ADDR, ucWritData, 5);
	return (bRet);
}

int	Set_Status( int mute, int stereo, int level, int band)
{
  	if (!mute)
		ucWritData[0]|=0x80;//mute
	else 
		ucWritData[0]&=0x7F;//play
  	
	if (stereo)
		ucWritData[2]|=0x8;
	else
		ucWritData[2]&=0xF7;

	if (band)//0--us/e
		ucWritData[3]|=0x20;
	else
		ucWritData[3]&=0xDF;
	
	if(FMM_Write())
		return TRUE;
	
	return FALSE;
		
}
	  
int Get_Status( void )
{
	//bit[0]~bit[1]--Reserved,bit[2]--stereo,
	//bit[3]--Reserved,bit[4]~bit[5]--level ADC Output(low=0,high=3).
	int status=0;
	int temp_IF=0,temp_LEV=0;
	FMM_Read();
	
	if (ucReadData[2]&0x80)
		status |=(1<<2);//stereo flag
		
	temp_IF=(ucReadData[2]& 0x7F);
	temp_LEV=(ucReadData[3]& 0xF0)>>4;
    printf("LEV=%d,IF =%d\n",temp_LEV,temp_IF);
    
	if ((temp_LEV == 8)&& (temp_IF==0x38 ))
	    status |= 0x30;//level high,true channel

	return status;
}
