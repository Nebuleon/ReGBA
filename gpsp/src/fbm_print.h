/****************************************************************************

  FBM fonts print string headder
                                                                      by mok
****************************************************************************/
#ifndef __FBM_PRINT__
#define __FBM_PRINT__


#define FBM_FONT_FILL	(0x01)		// Fill Font(normal)
#define FBM_BACK_FILL	(0x10)		// Fill BackGrand


typedef struct
{
	u16 fontcnt;
	u16 mapcnt;
	u16 defaultchar;
	u8  width;
	u8  height;
	u8  byteperchar;
} fbm_control_t;

typedef struct
{
	u16 start;
	u16 end;
	u16 distance;
} fbm_map_t;

typedef struct
{
	u8  *width;
	u8  *font;
} fbm_font_t;


/////////////////////////////////////////////////////////////////////////////
// Fbm Font Initialize
// s_path: Single Byte Font (ex.ASCII), d_path: Double Byte Font (ex.SJIS),
/////////////////////////////////////////////////////////////////////////////
int fbm_init(char *s_path, char *d_path);

/////////////////////////////////////////////////////////////////////////////
// Fbm Font Termination
/////////////////////////////////////////////////////////////////////////////
void fbm_freeall();

/////////////////////////////////////////////////////////////////////////////
// Get Draw Width-Pixcels
// str: Draw String
/////////////////////////////////////////////////////////////////////////////
int fbm_getwidth(char *str);

/////////////////////////////////////////////////////////////////////////////
// Print String: Base VRAM Addr + XY Pixel
// vram: Base VRAM Addr, bufferwidth: buffer-width per line,
// pixelformat: pixel color format (0=16bit, 1=15bit, 2=12bit, 3=32bit)
// x: X (0-479), y: Y (0-271), str: Print String,
// color: Font Color, back: Back Grand Color,
// fill: Fill Mode Flag (ex.FBM_FONT_FILL | FBM_BACK_FILL),
// rate: Mix Rate (0-100 or -1--101)
/////////////////////////////////////////////////////////////////////////////
int fbm_printVRAM(void *vram, int bufferwidth, int x, int y, char *str, u32 color, u32 back, u8 fill);

/////////////////////////////////////////////////////////////////////////////
// Print String Subroutine (Draw VRAM)
// vram: Base VRAM Addr, bufferwidth: buffer-width per line,
// index: Font Index, isdouble: Is Double Byte Font? (0=Single, 1=Double),
// height: Font Height, byteperline: Used 1 Line Bytes,
// color: Font Color, back: Back Grand Color,
// fill: Fill Mode Flag (ex.FBM_FONT_FILL | FBM_BACK_FILL),
// rate: Mix Rate (0-100 or -1--101)
/////////////////////////////////////////////////////////////////////////////
void fbm_printSUB(void *vram, int bufferwidth, int index, int isdouble, int height, int byteperline, u32 color, u32 back, u8 fill);

#endif
