/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
 * Copyright (C) 2007 ????? <?????>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common.h"
#include "gui.h"

/******************************************************************************
 *
 ******************************************************************************/
#ifndef NDS_LAYER
u32 screen_pitch = 480;
u32 screen_width = 240;//240;
u32 screen_height = 272;//160;
u32 screen_width2 = 240 / 2;
u32 screen_height2 = 160 / 2;
#else
u32 screen_pitch = 256;
u32 screen_width = 256;
u32 screen_height = 192;
u32 screen_width2 = 256 / 2;
u32 screen_height2 = 192 / 2;
#endif



#define COLORCNV(color)   (((color & 0x1F) * 8) << 0) |\
                            ((((color >> 5) & 0x1F) * 8) << 8) |\
                            ((((color >> 10) & 0x1F) * 8) << 16) | (0xFF << 24)

#define NR_PALETTE	256

//#define RGB24_15(pixel) (((((*pixel)*31/255) & 0x1F) << 10) |\
//                        ((((*(pixel+1))*31/255) & 0x1F) << 5) |\
//                        (((*(pixel+2))*31/255) & 0x1F))

#define RGB24_15(pixel) ((((*pixel) & 0xF8) << 7) |\
                        (((*(pixel+1)) & 0xF8) << 2) |\
                        (((*(pixel+2)) & 0xF8)>>3))

struct jzfb_info {
    unsigned int cfg;	/* panel mode and pin usage etc. */
	unsigned int w;
	unsigned int h;
	unsigned int bpp;	/* bit per pixel */
	unsigned int fclk;	/* frame clk */
	unsigned int hsw;	/* hsync width, in pclk */
	unsigned int vsw;	/* vsync width, in line count */
	unsigned int elw;	/* end of line, in pclk */
	unsigned int blw;	/* begin of line, in pclk */
	unsigned int efw;	/* end of frame, in line count */
	unsigned int bfw;	/* begin of frame, in line count */

	unsigned char *cpal;	/* Cacheable Palette Buffer */
	unsigned char *pal;	/* Non-cacheable Palette Buffer */
	unsigned char *cframe;	/* Cacheable Frame Buffer */
	unsigned char *frame;	/* Non-cacheable Frame Buffer */

	struct {
		unsigned char red, green, blue;
	} palette[NR_PALETTE];
};

extern int jzlcd_init(void);
extern struct jzfb_info jzfb;


#ifdef NDS_LAYER
u8 screen_buffer[256*192*2];
u16 *screen_address= (u16*)screen_buffer;
#else
unsigned char screen_buffer[480*272*2];
u16 *screen_address= (u16*)screen_buffer;
static u32 *display_addr; 
#endif

#ifdef NDS_LAYER
//RGB555
//u16  gba_screen_buffer[256*192];
//u16* gba_screen_address= gba_screen_buffer + 16*256 + 8;
u16* gba_screen_address;
u32  gba_screen_pitch= 256;
int  buf_handle;
#else
u16* gba_screen_address= (u16*)screen_buffer;
u32  gba_screen_pitch= 480;                     //= Screen width
#endif

char arg[3][100]= {"arg1\0",
               "arg2\0",
               "arg3\0"};

#if 0
void init_video()
{
    test_main(3, arg);
}
#endif 


void init_video()
{  
	// Need to show the loading screen here.
}

#ifdef NDS_LAYER
void flip_screen(void)
{
	ds2_flipScreen(UP_SCREEN, UP_SCREEN_UPDATE_METHOD);
}
#else
void flip_screen(void)
{
  u32 *dst_ptr;
  u16 *src_ptr;
  u32 x, y, z;
  //flush cache
//  dma_cache_wback_inv(screen_address, PSP_SCREEN_HEIGHT * PSP_SCREEN_WIDTH *2);

  src_ptr= screen_address;
  dst_ptr = display_addr; 
  for(y= 0; y < PSP_SCREEN_HEIGHT; y++)
  {
    for(x= 0; x < PSP_SCREEN_WIDTH; x++)
    {
      z = *src_ptr++;
      *dst_ptr++ = COLORCNV(z);
    }
  }
} 
#endif

#ifdef NDS_LAYER
void flip_gba_screen(void)
{
/*
  u32 *dst_ptr;
  u32 *src_ptr;
  u32 i;
  int buf_handle;
//  flush cache
//  dma_cache_wback_inv(screen_address, PSP_SCREEN_HEIGHT * PSP_SCREEN_WIDTH *2);

  buf_handle= get_video_up_buf();
  if(buf_handle >= 0)
  {
    dst_ptr = get_buf_form_bufnum(buf_handle);
    src_ptr= (u32*)gba_screen_buffer;
    for(i= 0; i< 128*192; i++)
        *dst_ptr++ = *src_ptr++;

    update_buf(buf_handle);
  }
*/
  int buf_h;
  u16 *ptr;

  update_buf(buf_handle);
  buf_h= get_video_up_buf();
  if(buf_h >= 0)
  {
    buf_handle = buf_h;    
    ptr = (u16*)get_buf_form_bufnum(buf_handle);
    gba_screen_address= ptr + 16*256 + 8;
  }
}
#endif


void video_resolution(u32 frame)
{

}

void clear_screen(u16 color)
{
  u32 i;
  u16 *dst; 

  dst= screen_address;
  for(i= 0; i < screen_height * screen_width; i++)
      *dst++ = color;

//  memset((u8*)screen_address, 0, 480*272*2);

}

void clear_gba_screen(u16 color)
{
#ifdef NDS_LAYER
  u32 i;
  u16 *dst; 

  dst= gba_screen_address -16*256 -8;
  for(i= 0; i < screen_height * screen_width; i++)
      *dst++ = color;
#endif
}

#ifndef NDS_LAYER
void copy_screen(u16 *buffer)
{
  u32 m;
  char *src, *dst;
  src= (char*)screen_address;
  dst= (char*)buffer;
  for(m= 0; m < GBA_SCREEN_HEIGHT; m++)
  {
    memcpy(dst, src, GBA_SCREEN_WIDTH * 2);
    dst += GBA_SCREEN_WIDTH * 2;
    src += PSP_SCREEN_WIDTH * 2;
  }
  
//  memcpy((char*)buffer, (char*)screen_address, GBA_SCREEN_BUFF_SIZE*2);
}

void blit_to_screen(u16 *src, u32 w, u32 h, u32 dest_x, u32 dest_y)
{
  u32 pitch = screen_pitch;
  u16 *dest_ptr = screen_address + dest_x + (dest_y * pitch);

  u16 *src_ptr = src;
  u32 line_skip = pitch - w;
  u32 x, y;

  for(y = 0; y < h; y++)
  {
    for(x = 0; x < w; x++, src_ptr++, dest_ptr++)
    {
      *dest_ptr = *src_ptr;
    }
    dest_ptr += line_skip;
  }
}
#else

void copy_screen(u16 *buffer)
{
    u32 x, y;
    u16 *ptr;

    for(y= 0; y < GBA_SCREEN_HEIGHT; y++)
    {
        ptr= gba_screen_address + y*256;
        for(x= 0; x < GBA_SCREEN_WIDTH; x++)
            *buffer++ = *ptr++;
    }
}

void blit_to_screen(u16 *src, u32 w, u32 h, u32 dest_x, u32 dest_y)
{
    u32 x, y;
    u16 *dst;
    u16 *screenp;

    if(w > NDS_SCREEN_WIDTH) w= NDS_SCREEN_WIDTH;
    if(h > NDS_SCREEN_HEIGHT) h= NDS_SCREEN_HEIGHT;
    if(dest_x == -1)    //align center
        dest_x= (NDS_SCREEN_WIDTH - w)/2;
    if(dest_y == -1)
        dest_y= (NDS_SCREEN_HEIGHT - h)/2;

    screenp= gba_screen_address -16*256 -8;
    for(y= 0; y < h; y++)
    {
        dst= screenp + (y+dest_y)*256 + dest_x;
        for(x= 0; x < w; x++)
            *dst++ = *src++;
    }
}

#endif

#define video_savestate_body(type)                                            \
{                                                                             \
  FILE_##type##_ARRAY(g_state_buffer_ptr, affine_reference_x);            \
  FILE_##type##_ARRAY(g_state_buffer_ptr, affine_reference_y);            \
}                                                                             \

void video_read_mem_savestate()
video_savestate_body(READ_MEM);

void video_write_mem_savestate()
video_savestate_body(WRITE_MEM);




#ifdef USE_IPU

#define IPU_BASE            0x13080000 | 0x80000000
#define IPU_CONTROL         (*((volatile u32*)(IPU_BASE + 0x0)))
#define IPU_STATUS          (*((volatile u32*)(IPU_BASE + 0x4)))
#define IPU_D_MFT           (*((volatile u32*)(IPU_BASE + 0x8)))
#define IPU_Y_ADDR          (*((volatile u32*)(IPU_BASE + 0xC)))
#define IPU_U_ADDR          (*((volatile u32*)(IPU_BASE + 0x10)))
#define IPU_V_ADDR          (*((volatile u32*)(IPU_BASE + 0x14)))
#define IPU_IN_FM_GS        (*((volatile u32*)(IPU_BASE + 0x18)))
#define IPU_Y_STRIDE        (*((volatile u32*)(IPU_BASE + 0x1C)))
#define IPU_UV_STRIDE       (*((volatile u32*)(IPU_BASE + 0x20)))
#define IPU_OUT_ADDR        (*((volatile u32*)(IPU_BASE + 0x24)))
#define IPU_OUT_GS          (*((volatile u32*)(IPU_BASE + 0x28)))
#define IPU_OUT_STRIDE      (*((volatile u32*)(IPU_BASE + 0x2C)))
#define IPU_RSZ_COEF_IDX    (*((volatile u32*)(IPU_BASE + 0x30)))
#define IPU_CSC_C_COEF(i)   (*((volatile u32*)(IPU_BASE + 0x34 + i*4)))
#define IPU_HRSZ_COEF_LUT(i)   (*((volatile u32*)(IPU_BASE + 0x48 + i*4)))
#define IPU_VRSZ_COEF_LUT(i)   (*((volatile u32*)(IPU_BASE + 0x98 + i*4)))


#define IPU_OUT_END         0x1
#define IPU_EN              0x1
#define IPU_RSZ_EN          0x2
#define IPU_FM_IRQ_EN       0x4
#define IPU_RST             0x8

#define IPU_H_SCALE         (ipu_info -> hv_ratio[0][2] << 8)
#define IPU_V_SCALE         (ipu_info -> hv_ratio[1][2] << 9)
#define IPU_SCALE           (IPU_H_SCALE | IPU_V_SCALE)

struct _screen_size
{
  u32 x;                    // screen width
  u32 y;                    // screen height
}

struct _frame_addr
{
  u32 *R;
  u32 *G;
  u32 *B;
  u32 Y_stride;
  u32 U_stride;
  u32 V_stride;
}

struct _out_frame_addr
{
    u32* addr;
    u32  stride;            //frame line to strid in data buffer, in byte unit(2/4 bytes align)
}

typedef enum
{
    YUV420  = 0;
    YUV422  = 1;
    YUV444  = 2;
    YUV411  = 3;
    YCbCr420   = 4;
    YCbCr422   = 5;
    YCbCr444   = 6;
    TCbCr411   = 7;
} SOURCE_PIX_TYPE;

typedef enum
{
    RGB555  = 0;
    RGB565  = 1;
    RGB888  = 2;
    RGB_ERR = 3;
} RESULT_PIX_TYPE;

struct _ipu_data{
  struct _screen_size prime_screen;
  struct _screen_size new_screen;  

  //hv_ratio[0][0]= prime size, [0]Numerator, [1]Denominator, [2] scale type
  //hv_ratio[1][0]= new size, [0]Numerator, [1]Denominator, [2] scale type
  u32   hv_ratio[2][3];
  
  struct _frame_addr  in_frame_addr;
  struct _out_frame_addr  out_frame_addr;

  //hv_coffe[1][0] H_COFFE; hv_coffe[1][20] HE_IDX_END
  //hv_coffe[2][0] H_COFFE; hv_coffe[2][20] HE_IDX_END
  u32   hv_coffe[2][21];

  u32   c_coffe[5];
  SOURCE_PIX_TYPE in_pixel_type;
  RESULT_PIX_TYPE out_pixel_type;
  u32   status;
}

typedef _ipd_data IPU_DATA;

IPU_DATA scale_info_A;

#define C0_COFFE 1024
#define C1_COFFE 1024
#define C2_COFFE 0
#define C3_COFFE 0
#define C4_COFFE 1024

void ipu_init(IPU_DATA *ipu_info)
{
    ipu_info -> prime_screen.x = SCREEN_WIDTH;
    ipu_info -> prime_screen.y = SCREEN_HEIGHT;
    ipu_info -> new_screen.x = SCREEN_WIDTH;
    ipu_info -> new_screen.y = SCREEN_HEIGHT;

    ipu_info -> in_frame_addr.R = NULL;
    ipu_info -> in_frame_addr.G = NULL;
    ipu_info -> in_frame_addr.B = NULL;
    ipu_info -> in_frame_addr.Y_stride = SCREEN_WIDTH >> 2;  //Word aligned
    ipu_info -> in_frame_addr.U_stride = SCREEN_WIDTH >> 2;
    ipu_info -> in_frame_addr.V_stride = SCREEN_WIDTH >> 2;

    ipu_info -> out_frame_addr.addr = NULL;
    ipu_info -> out_frame_addr.stride = SCREEN_WIDTH >> 1;   //Half word aligned

    //We only apply scale no color space convert
    ipu_info -> c_coffe[0] = C0_COFFE;
    ipu_info -> c_coffe[1] = C1_COFFE;
    ipu_info -> c_coffe[2] = C2_COFFE;
    ipu_info -> c_coffe[3] = C3_COFFE;
    ipu_info -> c_coffe[4] = C4_COFFE;

    ipu_info -> in_pix_type = YUV444;
    ipu_info -> out_pix_type = RGB565;
    
    ipu_info -> status = 0;

    IPU_CONTROL = IPU_RST;
}

static int calculate_n_m(u32 data[])
{
    u32 a, b, c;
    
    a= data[0];
    b= data[1];
    c= b;

    if(a < b)
    {
        b= a;
        a= c;
    }

    while(!b)
    {
        c= a%b;
        a= b;
        b= c;
    }
    
    data[0] /= a;
    data[1] /= a;

    return (data[0] < data[1]);         //0= Down-scale; 1= up-sclae
}

static void fix_scale_resize(IPU_DATA *ipu_info)
{
    u32 n, m, t;

    //H Down-scale
    n= ipu_info -> hv_ratio[0][0];
    m= ipu_info -> hv_ratio[0][1];
    t= ipu_info -> new_screen.x;
    if(ipu_info -> hv_ratio[0][2] == 0)
    {
        if(((t-1)*m % n) == 0)
            ipu_info -> new_screen.x = (t-1)*m/n;
        else
            ipu_info -> new_screen.x = (t-1)*m/n + 1;
    }
    else    //Up-scale
    {
        if(((t-1)*m % n) == 0)
            ipu_info -> new_screen.x = (t-1)*m/n + 1;
        else
            ipu_info -> new_screen.x = (t-1)*m/n + 2;
    }

    //V Down-scale
    n= ipu_info -> hv_ratio[1][0];
    m= ipu_info -> hv_ratio[1][1];
    t= ipu_info -> new_screen.y;
    if(ipu_info -> hv_ratio[1][2] == 0)
    {
        if(((t-1)*m % n) == 0)
            ipu_info -> new_screen.y = (t-1)*m/n;
        else
            ipu_info -> new_screen.y = (t-1)*m/n + 1;
    }
    else    //Up-scale
    {
        if(((t-1)*m % n) == 0)
            ipu_info -> new_screen.y = (t-1)*m/n + 1;
        else
            ipu_info -> new_screen.y = (t-1)*m/n + 2;
    }
}


static void calculate_hv_coffe(IPU_DATA *ipu_info)
{
    u32 n, m, i, index;
    u32 w_coef, out_en, a, b;
    
    for(index= 0; index < 2; index++)
    {
        n = ipu_info -> hv_ratio[index][0];
        m = ipu_info -> hv_ratio[index][1];
        //Down-scale
        if(ipu_info -> hv_ratio[index][2] == 0)
        {
            a = 0;
            b = 0;
            if(m > 1)
            {
                for(i= 0; i < n; i++)
                {
                    if((a*n+1) >= (i+1)*m)
                        w_coef= 0;
                    else if((a*n+1) == i*m )
                    {
                        w_coef = 1<< 2;
                        a++;
                    }
                    else 
                    {
                        w_coef = ((m -1 -((a*n)%m))*128/m) << 2;
                        a++;
                    }
                    
                    if((b*n+1) >= (i+1)*m)
                        out_en= 0;
                    else
                    {
                        out_en= 1;
                        b ++;
                    }
        
                    ipu_info -> hv_coffe[index][i] = w_coef | 2 | out_en;
                }
            }
            else
            {
                for(i= 1; i < n; i++)
                    ipu_info -> hv_coffe[index][i] = 2;

                hv_coffe[index][0] = 64 << 2 | 3;
            }
        }
        else
        {
            a= 0;
            for(i= 0; i< m; i++)
            {
                w_coef =((m - ((i*n)%m))/m) << 2;
                if(i*n >= a*m)
                {
                    b= 1<<1;
                    a++;
                }
                else
                    b= 0;

                ipu_info -> hv_coffe[index][i] = w_coef | b | 1;
            }
        }
        
        ipu_info -> hv_coffe[index][20] = i;
    }    
}



int set_frame_resolution(u32 prime_width, u32 prime_height, 
        u32 new_width, u32 new_height, IPU_DATA* ipu_info)
{
    ipu_info -> prime_screen.x= prime_width;
    ipu_info -> prime_screen.y= prime_height;
    ipu_info -> new_screen.x= new_width;
    ipu_info -> new_screen.y= new_height;
    nm[0] = prime_width;
    nm[1] = new_width;
    calculate_n_m(nm);
    ipu_info -> hv_ratio[0][0]= prime_width;
    ipu_info -> hv_ratio[0][1]= new_width;
    ipu_info -> hv_ratio[1][0]= prime_height;
    ipu_info -> hv_ratio[1][1]= new_height;
    
    //assume prime_width && new_width are not zero
    hv_ratio[0][2]= calculate_n_m(ipu_info -> hv_ratio[0][]);
    //assume prime_height && new_height are not zero 
    hv_ratio[1][2]= calculate_n_m(ipu_info -> hv_ratio[1][]); 
    
    calculate_hv_coffe(ipu_info);

    return 0;
}

int set_resolution_A()
{
}

int exe_frame_scale(IPU_DTA *ipu_info)
{
    u32 i;

    if(!(IPU_STATUS & IPU_OUT_END) && (IPU_CONTROL & IPU_EN))   //The IPU is busy, return
        return -1;

    IPU_STATUS = 0;    

    IPU_D_FMT = ipu_info -> out_pix_type | (ipu_info -> in_pix_type << 16);
    IPU_IN_FM_GS = ipu_info -> prime_screen.y | (ipu -> prime_screen.x << 16);
    IPU_Y_STRIDE = ipu_info -> in_frame_addr.Y_stride;
    IPU_U_STRIDE = ipu_info -> in_frame_addr.V_stride | (ipu -> in_frame_addr.U_stride << 16);
    IPU_Y_ADDR = ipu_info -> in_frame_addr.G;   //Note order
    IPU_U_ADDR = ipu_info -> in_frame_addr.R;
    IPU_V_ADDR = ipu_info -> in_frame_addr.B;
    IPU_OUT_GS = ipu_info -> new_screen.y | (ipu_info -> new-screen.x << 16);
    IPU_OUT_STRIDE = ipu_info -> out_frame_addr.stride;
    IPU_OUT_ADDR = ipu_info -> out_frame_addr.addr;
    
    for(i= 0; i< 5; i++)
        IPU_CSC_C0_COEF(i) = ipu_info -> c_coffe[i];

    for(i= 0; i< 20; i++)
        IPU_HRSZ_COEF_LUT(i) = ipu_info -> hv_coffe[0][i];

    for(i= 0; i< 20; i++)
        IPU_HRSZ_COEF_LUT(i) = ipu_info -> hv_coffe[1][i];

    IPU_RSZ_COEF_IDX = ipu_info -> hv_coffe[1][20] | (ipu_info -> hv_coffe[0][20] << 16);

    IPU_CONTROL = IPU_RSZ_EN | IPU_FM_IRQ_EN | IPU_SCALE;

    IPU_CONTROL |= IPU_EN;      //Start IPU

    return 0;
}


#endif




