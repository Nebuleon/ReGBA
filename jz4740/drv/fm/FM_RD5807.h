//
// Copyright (c) Ingenic Semiconductor Co., Ltd. 2007.
//

#include <bsp.h>

#define	TEA5767_WRITE_ADDR		0x60
#define	TEA5767_READ_ADDR		0x60

typedef struct
{
	unsigned char	BYTE[5];	// 0x00
} FM_CONFIG, *PFM_CONFIG;

FM_CONFIG	fmConfig[] =
{
	{0x29,0xc2,0x20,0x11,0x00},
{0xFF, 0xFF, 0xFF, 0xFF, 0xFF},	// Must be 0xFF to indicate that the table is END.
};
unsigned char ucWritData[5] = {0x00, 0x00, 0x20, 0x17, 0x00};
unsigned char ucReadData[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
//------------------------------------------------------------------------------
int 	FMM_Init();
int  	FMM_Deinit(void);
unsigned int 	FMM_Open (unsigned int dwData,
				  unsigned int dwAccess,
				  unsigned int dwShareMode
				  );
int 	FMM_Read ( void );
int 	FMM_Write( void );
int  FMM_IOControl(unsigned int hOpenContext,
					unsigned int dwCode,
					unsigned char *pBufIn,
					unsigned int dwLenIn,
					unsigned char *pBufOut,
					unsigned int dwLenOut,
					unsigned int *pdwActualOut
					);

void    Get_Pll(void);
void 	Set_Frequency(int freq);
int 	Get_Frequency(void);
void	Set_Volume(int vol_level);
int		Get_Volume(void);
int		Get_Max_Vol(void);
int		Get_Min_Vol(void);
int 	HLSI_optimal(unsigned int frequency);
int		Get_Status( void );
int		Set_Status( int mute, int stereo, int level, int band);
char	I2CSim_CommonRead(unsigned char ucSlaveID, unsigned char *pucBuffer, unsigned char ucBytes);
char 	I2CSim_CommonWrite(unsigned char ucSlaveID, unsigned char *pucBuffer, unsigned char ucBytes);