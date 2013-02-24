/****************************************************************************

  FBM fonts print string source
                                                                      by mok
****************************************************************************/

// gpSP用に不必要な部分を削除(32bit modeなど)

#include "common.h"

#define FBM_PSP_WIDTH			(480)
#define FBM_PSP_HEIGHT			(272)
#define FBM_PSP_PIXEL_FORMAT	(3)

#define FBM_SIZE_CONTROL		(9)
#define FBM_SIZE_MAP			(6)

static fbm_control_t fbmControl[2];
static fbm_map_t     *fbmFontMap[2];
static fbm_font_t    *fbmFont[2];
static u8     *font_buf[2];

static int           fbmMaxCol;
static int           fbmMaxRow;
static int    nextx;
static int    nexty;

static char   use_subfont;	// 0=single only, other=single+double

//static char   *fbm_path[2];
static int    fbm_whence[2];
static int    fbm_fd[2] = {-1, -1};

static void *fbm_malloc(size_t size);
static void fbm_free(void **ptr);
static int fbm_fopen(char *path);
static void fbm_fclose(int *fd);
static int fbm_readfct(int fd, fbm_control_t *control, fbm_map_t **map, int *fbm_whence);
static int fbm_readfbm(int fd, fbm_control_t *control, fbm_font_t **font, u8 **buf, int fbm_whence, int index, int fontcnt);
static int fbm_issubfont(u16 c);
static int fbm_ismainfont(u16 c);
static fbm_font_t * fbm_getfont(u16 index, u8 isdouble);
static const char* utf8decode(const char *utf8, u16 *ucs);
static u8 utf8_ucs2(const char *utf8, u16 *ucs);
static u32 ucslen(const u16 *ucs);

#ifndef ENABLE_MALLOC
#define FBM_MEM_MAX_QUEUE 8
#define FBM_MEM_BLOCK_SIZE 0x24000
static char fbm_mem[FBM_MEM_MAX_QUEUE][FBM_MEM_BLOCK_SIZE];			//144KB*8
static int fbm_mem_queue_num;
struct _fbm_mem_queue_attr
{
	void *mem_ptr;
	int status;
} fbm_mem_queue_attr[FBM_MEM_MAX_QUEUE];

static void fbm_mem_init(void);
#endif

/*------------------------------------------------------
  默认字体
  s_path: Single Byte Font (ex.ASCII)
  d_path: Double Byte Font (ex.SJIS)
------------------------------------------------------*/
int fbm_init(char *m_path, char *s_path)
{
  int  result;

  use_subfont = (s_path) ? (s_path[0]) ? 1: 0: 0;
  fbmMaxCol = fbmMaxRow = 0;
  nextx = nexty = 0;

#ifndef ENABLE_MALLOC
  fbm_mem_init();
#endif

  fbm_fd[0] = fbm_fopen(m_path);

//  if (fbm_fd[0] < 0)
  if (!fbm_fd[0])
  {
    result = -1;
    goto err_label;
  }

  result = fbm_readfct(fbm_fd[0], &fbmControl[0], &fbmFontMap[0], &fbm_whence[0]);

  if (result)
  {
    result -= 1;
    goto err_label;
  }

  if (use_subfont)
  {
    fbm_fd[1] = fbm_fopen(s_path);

//    if (fbm_fd[1] < 0)
    if (!fbm_fd[1])
    {
      result = -5;
      goto err_label;
    }

    result = fbm_readfct(fbm_fd[1], &fbmControl[1], &fbmFontMap[1], &fbm_whence[1]);

    if (result)
    {
      result -= 5;
      goto err_label;
    }
  }

  result = fbm_readfbm(fbm_fd[0], &fbmControl[0], &fbmFont[0], &font_buf[0], fbm_whence[0], 0, fbmControl[0].fontcnt);

  if (result)
  {
    result -= 8;
    goto err_label;
  }

  if (use_subfont)
  {
    result = fbm_readfbm(fbm_fd[1], &fbmControl[1], &fbmFont[1], &font_buf[1], fbm_whence[1], 0, fbmControl[1].fontcnt);

    if (result)
    {
      result -= 12;
      goto err_label;
    }
  }

  fbmMaxCol = FBM_PSP_WIDTH / fbmControl[0].width;
  fbmMaxRow = FBM_PSP_HEIGHT / fbmControl[0].height;

  fbm_fclose(&fbm_fd[0]);
  fbm_fclose(&fbm_fd[1]);

  return 0;

  err_label:
  fbm_fclose(&fbm_fd[0]);
  fbm_fclose(&fbm_fd[1]);
  fbm_freeall();

  return result;
}

/*------------------------------------------------------
  释放字体库
------------------------------------------------------*/
void fbm_freeall()
{
//	fbm_free((void *)&fbm_path[0]);
//	fbm_free((void *)&fbm_path[1]);
	fbm_free((void *)&fbmFontMap[0]);
	fbm_free((void *)&fbmFontMap[1]);
	fbm_free((void *)&fbmFont[0]);
	fbm_free((void *)&fbmFont[1]);
	fbm_free((void *)&font_buf[0]);
	fbm_free((void *)&font_buf[1]);

	fbm_whence[0] = 0;
	fbm_whence[1] = 0;

	fbmMaxCol = fbmMaxRow = 0;
}
/*------------------------------------------------------
  计算一个字符串的宽度
  str: Draw String
  RET: Draw Width
------------------------------------------------------*/
int fbm_getwidth(char *str)
{
  int           i;
  int           len;
  int           index;
  int           width;
  fbm_font_t    *font;
  u16           ucs[2048];
  u32 font_num;
  width = 0;
  utf8_ucs2(str, ucs);
  len = ucslen(ucs);

  for (i = 0; i < len; i++)
  {
    index = fbm_issubfont(ucs[i]);

    if (index >= 0)
      font_num = 1;
    else
    {
      index = fbm_ismainfont(ucs[i]);
      font_num = 0;
      if (index < 0)
      {
        index = fbmControl[0].defaultchar;
      }
    }
    font = fbm_getfont(index, font_num);
    width += *(font->width);
  }

  return width;
}

/*------------------------------------------------------
  写一个字符串到指定的帧缓冲
  Print String: Base VRAM Addr + XY Pixel
  vram: Base VRAM Addr
  bufferwidth: buffer-width per line,
  x: X (0-479), y: Y (0-271), str: Print String,
  color: Font Color, back: Back Grand Color,
  fill: Fill Mode Flag (ex.FBM_FONT_FILL | FBM_BACK_FILL),
------------------------------------------------------*/
int fbm_printVRAM(void *vram, int bufferwidth, int x, int y, char *str, u32 color, u32 back, u8 fill)
{
  int i;
  int len;
  int index;
  int issub;
  u16 ucs[2048];

  if (bufferwidth == 0) return -1;

  if (x >= 0) nextx = x % FBM_PSP_WIDTH;
  if (y >= 0) nexty = y % FBM_PSP_HEIGHT;

  // utf-8nをUCS2に変換
  utf8_ucs2(str, ucs);
  len = ucslen(ucs);

  for (i = 0; i < len; i++)
  {
    if (ucs[i] == '\n')
    {
      nextx = x;
      nexty += fbmControl[0].height;
    }
    else
    {
      index = fbm_issubfont(ucs[i]);
      issub = 1;
      if (index < 0)
      {
        index = fbm_ismainfont(ucs[i]);
        issub = 0;
        if (index < 0)
        {
          index = fbmControl[0].defaultchar;
        }
      }
      fbm_printSUB(vram, bufferwidth, index, issub, fbmControl[issub].height, fbmControl[issub].byteperchar / fbmControl[issub].height, color, back, fill);
    }
  }

  return 0;
}

// TODO GUでの描画
/////////////////////////////////////////////////////////////////////////////
// Print String Subroutine (Draw VRAM)
// vram: Base VRAM Addr, bufferwidth: buffer-width per line,
// index: Font Index, isdouble: Is Double Byte Font? (0=Single, 1=Double),
// height: Font Height, byteperline: Used 1 Line Bytes,
// color: Font Color, back: Back Grand Color, when 0x1xxx xxxx transparent
// fill: Fill Mode Flag (ex.FBM_FONT_FILL | FBM_BACK_FILL),
/////////////////////////////////////////////////////////////////////////////
void fbm_printSUB(void *vram, int bufferwidth, int index, int isdouble, int height, int byteperline, u32 color, u32 back, u8 fill)
{
  int           i;
  int           j;
  int           shift;
  u8            pt;
  u16           *vptr;
  fbm_font_t    *font;

  if (index < 0) return;

  font = fbm_getfont(index, isdouble);

  if (nextx + *(font->width) > FBM_PSP_WIDTH)
  {
    nextx = 0;
    nexty += fbmControl[0].height;
  }

  if (nexty + height > FBM_PSP_HEIGHT)
  {
    nexty = 0;
  }

  vram = (u16 *)vram + nextx + nexty * bufferwidth;

  for (i = 0; i < height; i++)
  {
    vptr = (u16 *)vram;
    shift = 0;

    index = i * byteperline;
    pt = font->font[index++];

    // ビットマップの表示
    for (j = 0; j < *(font->width); j++)
    {
      if (shift >= 8)
      {
        shift = 0;
        pt = font->font[index++];
      }

      if (pt & 0x80)
      {
        // 文字描画時
        if (fill & 0x01)
          *vptr =  (u16)color;
      }
      // 背景描画時
      else
      {
        if (fill & 0x10)
        if (!(back & 0x8000))
          *vptr = (u16)back;
      }

      vptr++;

      shift++;
      pt <<= 1;
    }

    vram = (u16 *)vram + bufferwidth;
  }

  nextx = nextx + *(font->width);
}

#ifdef ENABLE_MALLOC
void *fbm_malloc(size_t size)
{
  int h_block;

//  if (size == 0) return NULL;

  h_block = (int)malloc(size);

//  if (h_block < 0) return NULL;

  return (void *)(h_block);
}


void fbm_free(void **ptr)
{
  if (*ptr != NULL)
    free(*ptr);
}

#else

void fbm_mem_init(void)
{
	int i;

	fbm_mem_queue_num = FBM_MEM_MAX_QUEUE;
	for(i= 0; i< FBM_MEM_MAX_QUEUE; i++)
	{
		fbm_mem_queue_attr[i].mem_ptr = (void*)&fbm_mem[i][0];
		fbm_mem_queue_attr[i].status = 0;
	}
}

void *fbm_malloc(size_t size)
{
static m= 0;

  if (size == 0) return NULL;
  
  if(fbm_mem_queue_num == 0)
  {
	  dbg_printf("FBM Error! no enough memory for malloc \n");
	  return NULL;
  }

printf("No. %d need %08x\n", m++, size);

  if(FBM_MEM_BLOCK_SIZE < size)
  {
	  dbg_printf("FBM Error! memory block too small for malloc\n");
	  printf("size %08x\n", size);
	  return NULL;
  }

  int i;
  for(i= 0; i < FBM_MEM_MAX_QUEUE; i++)
  {
	if(!fbm_mem_queue_attr[i].status)
		break;
  }

  fbm_mem_queue_attr[i].status = 1;
  fbm_mem_queue_num-- ;
  return (fbm_mem_queue_attr[i].mem_ptr);
}


void fbm_free(void **ptr)
{
  int i;
  
  for(i= 0; i < FBM_MEM_MAX_QUEUE; i++)
  {
	if(fbm_mem_queue_attr[i].mem_ptr == ptr)
	  break;
  }
  if(i== FBM_MEM_MAX_QUEUE)
  {
	  dbg_printf("FBM msg! free not allocated\n");
	  return;
  }

  if(fbm_mem_queue_attr[i].status)
  {
    fbm_mem_queue_attr[i].status = 0;
	fbm_mem_queue_num++;
	return;
  }  
}
#endif

int fbm_fopen(char *path)
{
  int result;
  
//result = sceIoOpen(path, PSP_O_RDONLY, 0777);
  result= (int)fopen(path, "r");

//  if(!result)
//	result = -1;

  return result;
}


void fbm_fclose(int *fd)
{
  if (*fd < 0) return;
//  sceIoClose(*fd);
  fclose((FILE*)(*fd));
  *fd = -1;
}


int fbm_readfct(int fd, fbm_control_t *control, fbm_map_t **map, int *fbm_whence)
{
  int result;


//  result = sceIoRead(fd, control, FBM_SIZE_CONTROL);
  result = fread(control, 1, FBM_SIZE_CONTROL, (FILE*)fd);

  if (result != FBM_SIZE_CONTROL)
  {
//    sceIoClose(fd);
	fclose((FILE*)fd);
    return -1;
  }

  *map = (fbm_map_t *)fbm_malloc(6 * control->mapcnt);

  if (*map == NULL)
  {
//    sceIoClose(fd);
	fclose((FILE*)fd);
    return -2;
  }

//  result = sceIoRead(fd, *map, FBM_SIZE_MAP * control->mapcnt);
  result = fread(*map, 1, FBM_SIZE_MAP * control->mapcnt, (FILE*)fd);

  if (result != FBM_SIZE_MAP * control->mapcnt)
  {
//    sceIoClose(fd);
    fclose((FILE*)fd);
    return -3;
  }

  *fbm_whence = FBM_SIZE_CONTROL + FBM_SIZE_MAP * control->mapcnt;

  return 0;
}


int fbm_readfbm(int fd, fbm_control_t *control, fbm_font_t **font, u8 **buf, int fbm_whence, int index, int fontcnt)
{
  int result;
  int offset;
  int rebuild;
  u16 i;


  rebuild = (*font == NULL || *buf == NULL) ? 1: 0;

  if (rebuild)
  {
    fbm_free((void **)font);
    fbm_free((void **)buf);

    *font = (fbm_font_t *)fbm_malloc(sizeof(fbm_font_t) * fontcnt);

    if (*font == NULL)
    {
      return -1;
    }

    *buf = (u8 *)fbm_malloc((1 + control->byteperchar) * fontcnt);

    if (*buf == NULL)
    {
      return -2;
    }
  }

  offset = (1 + control->byteperchar) * index;
//  result = sceIoLseek32(fd, offset + fbm_whence, 0);
  result = fseek((FILE*)fd, offset + fbm_whence, SEEK_SET);

  if (result != offset + fbm_whence)
  {
//    sceIoClose(fd);
	fclose((FILE*)fd);
    return -3;
  }

//  result = sceIoRead(fd, *buf, (1 + control->byteperchar) * fontcnt);
  result = fread(*buf, 1, (1 + control->byteperchar) * fontcnt, (FILE*)fd);

  if (result != (1 + control->byteperchar) * fontcnt)
  {
//    sceIoClose(fd);
	fclose((FILE*)fd);
    return -4;
  }

  if (rebuild)
  {
    for (i = 0; i < fontcnt; i++)
    {
      (*font)[i].width = *buf + (1 + control->byteperchar) * i;
      (*font)[i].font = *buf + (1 + control->byteperchar) * i + 1;
    }
  }

  return 0;
}

int fbm_ismainfont(u16 c)
{
  int i;

  for (i = 0; i < fbmControl[0].mapcnt && c >= fbmFontMap[0][i].start; i++)
  {
    if (c >= fbmFontMap[0][i].start && c <= fbmFontMap[0][i].end)
      return c - fbmFontMap[0][i].distance;
  }

  return -1;
}

int fbm_issubfont(u16 c)
{
  int i;

  if (!use_subfont) return -1;

  for (i = 0; i < fbmControl[1].mapcnt && c >= fbmFontMap[1][i].start; i++)
  {
    if (c >= fbmFontMap[1][i].start && c <= fbmFontMap[1][i].end)
      return c - fbmFontMap[1][i].distance;
  }

  return -1;
}


fbm_font_t * fbm_getfont(u16 index, u8 isdouble)
{
  return &fbmFont[isdouble][index];
}

/*------------------------------------------------------
  UTF-8をucs2に変換
  return 次の文字へのポインタ
------------------------------------------------------*/
static const char* utf8decode(const char *utf8, u16 *ucs)
{
    unsigned char c = *utf8++;
    unsigned long code;
    int tail = 0;

    if ((c <= 0x7f) || (c >= 0xc2)) {
        /* Start of new character. */
        if (c < 0x80) {        /* U-00000000 - U-0000007F, 1 byte */
            code = c;
        } else if (c < 0xe0) { /* U-00000080 - U-000007FF, 2 bytes */
            tail = 1;
            code = c & 0x1f;
        } else if (c < 0xf0) { /* U-00000800 - U-0000FFFF, 3 bytes */
            tail = 2;
            code = c & 0x0f;
        } else if (c < 0xf5) { /* U-00010000 - U-001FFFFF, 4 bytes */
            tail = 3;
            code = c & 0x07;
        } else {
            /* Invalid size. */
            code = 0xfffd;
        }

        while (tail-- && ((c = *utf8++) != 0)) {
            if ((c & 0xc0) == 0x80) {
                /* Valid continuation character. */
                code = (code << 6) | (c & 0x3f);

            } else {
                /* Invalid continuation char */
                code = 0xfffd;
                utf8--;
                break;
            }
        }
    } else {
        /* Invalid UTF-8 char */
        code = 0xfffd;
    }
    /* currently we don't support chars above U-FFFF */
    *ucs = (code < 0x10000) ? code : 0xfffd;
    return utf8;
}

static u8 utf8_ucs2(const char *utf8, u16 *ucs)
{
  while(*utf8 !='\0')
  {
    utf8 = utf8decode(utf8, ucs++);
  }
  *ucs = '\0';
  return 0;
}

static u32 ucslen(const u16 *ucs)
{
  u32 len = 0;
  while(ucs[len] != '\0')
    len++;
  return len;
}
