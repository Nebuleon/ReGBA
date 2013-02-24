#include <bsp.h>
#include <jz4740.h>
#include <mipsregs.h>
//#include <jzfs_api.h>
#include "common.h"

#include "libdsm.h"
#include "test.h"

void update_down_dis( u32 getbuf, u32 len);
void update_up_dis( u32 getbuf, u32 len);
///////////////////////////////////////////

u32 DSMID;
u32 VideoOffset;
u32 VideoSize;
u32 VideoSeekTableOffset;
u32 VideoSeekTableSize;
u32 VideoCount;
u32 VideoDataBegin;

u32 AudioOffset;
u32 AudioSize;
u32 AudioFrameLength;
u32 AudioFramesCount;
u32 AudioDataBegin;

int audio_curr_offset;
int video_curr_offset;

// -----------------
typedef struct {
  u16 wFormatTag;
  u16 nChannels;
  u32 nSamplesPerSec;
  u32 nAvgBytesPerSec;
  u16 nBlockAlign;
  u16 wBitsPerSample;
  u16 cbSize;
} WAVEFORMATEX;

FILE* FileHandle;
static WAVEFORMATEX wfex;
static int DataTopOffset;
static int SampleCount;
static int SampleOffset;
static int BytePerSample;

static int SamplePerFrame;
static u32 *ReadBuffer;

FILE* fpdsm;
FILE* fpdsm1;
u8 tempbuf[256*192 + 512 + 1024];
u8 tempbuf1[256*192 + 512 + 1024];
extern unsigned long DecompressData(unsigned char *src, unsigned char *dest);
int read_video(u16 *buff)
{
    int i;
    int size;
    int curr_sample;
u8* dot;
u16* Pal;
int read_len;

i=0;
    //printf("video_curr_offset=%x \n",video_curr_offset);
    //printf("curr_sample=%x \n",curr_sample);



   fseek(fpdsm,video_curr_offset,SEEK_SET);
   read_len = fread(&size,1,4,fpdsm);
if (read_len != 4)return -1;
   read_len=fread(&curr_sample,1,4,fpdsm);
if (read_len != 4)return -1;
   read_len=fread(tempbuf,1,size - 4,fpdsm);
if (read_len != (size - 4))return -1;
   video_curr_offset= ftell(fpdsm);


   //swiDecompressLZSSWram(&tempbuf[256*2],&tempbuf1[256*2]);
   DecompressData(&tempbuf[256*2],&tempbuf1[256*2]);
   dot=(u8*)&tempbuf1[256*2];
   Pal=(u16*)&tempbuf[0];

   for (i=0;i<(256*192) ; i++)
   {
       buff[i]=Pal[dot[i]] | (1<<15);
   }
return 0;

}
///////////////////////////////////////////////////////////////////////////////
#define FRAME_TIME	1.04489795918367346939
#define MAX_ORDER	8
#define TTBHDR_SIZE (6*4)

#define TTA1_SIGN	0x31415454
#define TTB1_SIGN	0x31425454

#define ttainfo_BPS (8)
#define ttainfo_NCH (2)
#define ttainfo_BSIZE (ttainfo_BPS/8)
#define ttainfo_FORMAT (1)
#define ttainfo_SampleRate (32768)
#define ttainfo_FrameLength (4096)

unsigned int bit_mask[] = {
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
	0xffffffff
};

unsigned int bit_shift[] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x80000000, 0x80000000, 0x80000000, 0x80000000,
	0x80000000, 0x80000000, 0x80000000, 0x80000000
};

unsigned int *shift_16 = bit_shift + 4;

//typedef u8 byte;

#define PREDICTOR1(x, k)	((int)((((int)x << k) - x) >> k))
#define DEC(x)			(((x)&1)?(((x)+1)>>1):(-(x)>>1))

typedef struct {
	unsigned int k0;
	unsigned int k1;
	unsigned int sum0;
	unsigned int sum1;
} adapt;

typedef struct {
//	int shift;
//	int round;
	int error;
//	int mutex;
	int qm[MAX_ORDER+1];
	int dx[MAX_ORDER+1];
	int dl[MAX_ORDER+1];
} fltst;

typedef struct {
	int last;
	fltst fst;
	adapt rice;
} decoder;

u8 pbitread_buffer[ttainfo_FrameLength*ttainfo_NCH];

//static TTTAInfo *ttainfo;		// currently playing file info

decoder tta_initialized_decoder_8bit2ch[ttainfo_NCH];

#define fs_shift (10) // for 8bit
#define fs_round (1 << (fs_shift - 1)) // for 8bit

void
memshl (register int *pA, register int *pB) {
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA   = *pB;
}

void
hybrid_filter (fltst *fs, int *in) {
	register int *pA = fs->dl;
	register int *pB = fs->qm;
	register int *pM = fs->dx;
	register int sum = fs_round;

	if (!fs->error) {
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++; pM += 8;
	} else if (fs->error < 0) {
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
	} else {
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
	}

	*(pM-0) = ((*(pA-1) >> 30) | 1) << 2;
	*(pM-1) = ((*(pA-2) >> 30) | 1) << 1;
	*(pM-2) = ((*(pA-3) >> 30) | 1) << 1;
	*(pM-3) = ((*(pA-4) >> 30) | 1);

	fs->error = *in;
	*in += (sum >> fs_shift);
	*pA = *in;

	*(pA-1) = *(pA-0) - *(pA-1);
	*(pA-2) = *(pA-1) - *(pA-2);
	*(pA-3) = *(pA-2) - *(pA-3);

	memshl (fs->dl, fs->dl + 1);
	memshl (fs->dx, fs->dx + 1);
}

void filter_init_8bit (fltst *fs) {
	memset (fs, 0, sizeof(fltst));
}

/************************* file access ******************************/


int FAT_fread (void* buffer)
{
  //DMA2_SRC = (u32)IPC6->TTAInfo.pCodeBuf;
  //DMA2_DEST = (u32)buffer;
  //DMA2_CR=(DMA_32_BIT | DMA_ENABLE | DMA_START_NOW | DMA_SRC_INC | DMA_DST_INC)+(IPC6->TTAInfo.CodeSize >> 2);
  
  //IPC6->TTAInfo.CodeSize=0;
   u32 size;
int read_len;
   fseek(fpdsm,audio_curr_offset,SEEK_SET);

   read_len=fread(&size,1,4,fpdsm);
if (read_len != 4)return -1;
   read_len=fread(buffer,1,size,fpdsm);
if (read_len != size)return -1;


   audio_curr_offset = ftell(fpdsm);
return 0;

}

/************************* bit operations ******************************/

#define GET_BINARY(value, bits) \
  while (bit_count < bits) { \
    bit_cache |= *bit_pos << bit_count; \
    bit_count += 8; \
    bit_pos++; \
  } \
  value = bit_cache & bit_mask[bits]; \
  bit_cache >>= bits; \
  bit_count -= bits; \
  bit_cache &= bit_mask[bit_count];

#define GET_UNARY(value) \
  value = 0; \
  while (!(bit_cache ^ bit_mask[bit_count])) { \
    value += bit_count; \
    bit_cache = *bit_pos++; \
    bit_count = 8; \
  } \
  while (bit_cache & 1) { \
    value++; \
    bit_cache >>= 1; \
    bit_count--; \
  } \
  bit_cache >>= 1; \
  bit_count--;

#define setup_buffer_read() \
unsigned int bit_count=0; \
unsigned int bit_cache=0; \
unsigned char *bit_pos=pbitread_buffer; \
FAT_fread(bit_pos);

#define done_buffer_read() /* bit_pos += 4; */ // skip CRC32

/************************* decoder functions ****************************/

void rice_init(adapt *rice, unsigned int k0, unsigned int k1) {
  rice->k0 = k0;
  rice->k1 = k1;
  rice->sum0 = shift_16[k0];
  rice->sum1 = shift_16[k1];
}

void decoder_init_8bit(decoder *ptta) {
  filter_init_8bit(&ptta->fst);
  rice_init(&ptta->rice, 10, 10);
  ptta->last = 0;
}

void player_init (void) {
  //ttainfo = _pTTAInfo;
  u32 idx;
  for(idx=0;idx<ttainfo_NCH;idx++){
    decoder_init_8bit(&tta_initialized_decoder_8bit2ch[idx]);
  }
  
  //pbitread_buffer=(u8*)safemalloc(ttainfo_FrameLength*ttainfo_NCH);
}
//int fpdsmA;
decoder tta[ttainfo_NCH];
#define read_audio(l,r) get_samples(l,r)
/*
int get_samples2 (s16 *bufl,s16 *bufr)
{
   u32 size;
int read_len;
 
   read_len=fread(bufl,1,8192,fpdsmA);
if (read_len != 8192)return -1;

   read_len=fread(bufr,1,8192,fpdsmA);
if (read_len != 8192)return -1;
return 0;



} 

*/



int get_samples (s16 *bufl,s16 *bufr) {
  //setup_buffer_read();
unsigned int bit_count;
unsigned int bit_cache;
unsigned char *bit_pos;
decoder *dec;
      int value;
      unsigned int unary;
adapt *rice;
      unsigned int k;
      int depth;
        unsigned int binary;
u32 data_cur;
int right;
int left;
int read_len;
    bit_count=0;
    bit_cache=0;
    bit_pos=pbitread_buffer;
read_len=FAT_fread(bit_pos);
  if(read_len == -1) return -1;
  //MemCopy32swi256bit(&tta_initialized_decoder_8bit2ch,tta,2*sizeof(decoder));
  memcpy(tta,&tta_initialized_decoder_8bit2ch,2*sizeof(decoder));
  
  for (data_cur=ttainfo_FrameLength;data_cur!=0;data_cur--) {
    int samplecache_left;
    {
      dec = &tta[0];
      
      
      // decode Rice unsigned
      GET_UNARY(unary);
      
      rice = &dec->rice;
      
      if(unary==0) {
        depth = 0; k = rice->k0;
        }else{
        depth = 1; k = rice->k1;
        unary--;
      }
      
      if (k!=0) {
        GET_BINARY(binary, k);
        value = (unary << k) + binary;
        }else{
        value = unary;
      }
      
      if(depth==1){
        rice->sum1 += value - (rice->sum1 >> 4);
        if (rice->k1 > 0 && rice->sum1 < shift_16[rice->k1])
          rice->k1--;
        else if (rice->sum1 > shift_16[rice->k1 + 1])
          rice->k1++;
        value += bit_shift[rice->k0];
      }
      
      {
        rice->sum0 += value - (rice->sum0 >> 4);
        if (rice->k0 > 0 && rice->sum0 < shift_16[rice->k0])
          rice->k0--;
        else if (rice->sum0 > shift_16[rice->k0 + 1])
        rice->k0++;
      }
  
      value = DEC(value);
  
      // decompress stage 1: adaptive hybrid filter
      hybrid_filter(&dec->fst, &value);
  
      // decompress stage 2: fixed order 1 prediction
      value += PREDICTOR1(dec->last, 4);	// bps 8
      dec->last = value;
  
      samplecache_left=value;
    }
    // ----------------------------------------------
    {
      dec = &tta[1];
      
      
      // decode Rice unsigned
      GET_UNARY(unary);
      
      rice = &dec->rice;
      
      if(unary==0) {
        depth = 0; k = rice->k0;
        }else{
        depth = 1; k = rice->k1;
        unary--;
      }
      
      if (k!=0) {
        GET_BINARY(binary, k);
        value = (unary << k) + binary;
        }else{
        value = unary;
      }
      
      if(depth==1){
        rice->sum1 += value - (rice->sum1 >> 4);
        if (rice->k1 > 0 && rice->sum1 < shift_16[rice->k1])
          rice->k1--;
        else if (rice->sum1 > shift_16[rice->k1 + 1])
          rice->k1++;
        value += bit_shift[rice->k0];
      }
      
      {
        rice->sum0 += value - (rice->sum0 >> 4);
        if (rice->k0 > 0 && rice->sum0 < shift_16[rice->k0])
          rice->k0--;
        else if (rice->sum0 > shift_16[rice->k0 + 1])
        rice->k0++;
      }
  
      value = DEC(value);
  
      // decompress stage 1: adaptive hybrid filter
      hybrid_filter(&dec->fst, &value);
  
      // decompress stage 2: fixed order 1 prediction
      value += PREDICTOR1(dec->last, 4);	// bps 8
      dec->last = value;
  
      right=value+(samplecache_left/2);
      left=right-samplecache_left;
      
      *bufl++ = left<<8;
      *bufr++ = right<<8;
    }
  }
  
  done_buffer_read();
return 0;
}

void opendsm(char *name )
{
    if ((fpdsm=fopen(name,"rb"))==NULL)
    {
        printf("open error \n");

        return  ;
    }
    //if ((fpdsmA=fopen("/mnt/mmc/DIDI.BIN","rb"))==NULL)
    //{
    //    printf("open error \n");
//
//        return  ;
 //   }
   fread(&DSMID,4,1,fpdsm);
   fread(&VideoOffset,4,1,fpdsm);
   fread(&VideoSize,4,1,fpdsm);
   fread(&VideoSeekTableOffset,4,1,fpdsm);
   fread(&VideoSeekTableSize,4,1,fpdsm);
   fread(&AudioOffset,4,1,fpdsm);
   fread(&AudioSize,4,1,fpdsm);

   fseek(fpdsm,VideoOffset,SEEK_SET);
   fread(&VideoCount,4,1,fpdsm);
   fseek(fpdsm,VideoOffset + (4*4),SEEK_SET);
   VideoDataBegin = ftell(fpdsm);
   video_curr_offset=VideoDataBegin;


   fseek(fpdsm,AudioOffset + (4*4),SEEK_SET);
   fread(&AudioFrameLength,4,1,fpdsm);
   fread(&AudioFramesCount,4,1,fpdsm);
   fseek(fpdsm,AudioOffset + (TTBHDR_SIZE),SEEK_SET);
   AudioDataBegin = ftell(fpdsm);
   audio_curr_offset=AudioDataBegin;
//fseek(fpdsmA,0,SEEK_SET);

   player_init();

}

typedef struct sTransferSoundData {
//---------------------------------------------------------------------------------
  const void *data;
  u32 len;
  u32 rate;
  u8 vol;
  u8 pan;
  u8 format;
  u8 PADDING;
} TransferSoundData, * pTransferSoundData;


//---------------------------------------------------------------------------------
typedef struct sTransferSound {
//---------------------------------------------------------------------------------
  TransferSoundData data[2];
  u8 count;
  u8 PADDING[3];
} TransferSound, * pTransferSound;

struct video_set video_set_buf;
struct audio_set audio_set_buf;

int video_enable;

TransferSoundData SoundDataL;
TransferSoundData SoundDataR;

TransferSound SoundDataL_T;
TransferSound SoundDataR_T;
int draw_col;


void enable_play(void)
{
    u32 temp32p[512/4];
    int offset;
    //int flag;
    //u32 getbuf;
    int offsetindex;
    //int i;
    memset(temp32p,0,512);
    temp32p[0]=0xc0;

    offset = 0x60;
    offsetindex = 4;		

    temp32p[offsetindex/4]=IS_SET_AUDIO;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;//sizeof(audio_set);
    memcpy(&temp32p[offset/4],&audio_set_buf,sizeof(struct audio_set));
    offset+=sizeof(struct audio_set);

    temp32p[offsetindex/4]=IS_SET_VIDEO;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;//sizeof(audio_set);
    memcpy(&temp32p[offset/4],&video_set_buf,sizeof(struct video_set));
    offset+=sizeof(struct video_set);
    temp32p[offsetindex/4]=IS_SET_ENABLE_VIDEO;offsetindex+=4;//
    temp32p[offsetindex/4]=offset;offsetindex+=4;//sizeof(audio_set);
    temp32p[offset/4]=1;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_AUDIO;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;//sizeof(audio_set);
    temp32p[offset/4]=1;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_KEY;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;//sizeof(audio_set);
    temp32p[offset/4]=1;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_RTC;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=1;
    offset+=4;



printf("set_nds_var\n");
    set_nds_var(temp32p,512);



}
void disenable_play(void)
{

}
extern u32 get(void);






void test_Brightness()
{
    
    INPUT * inputdata;
    int buf_change;
    int down_buf_num;
    u32 getbuf;
    int data;
    int blup;
    int bldown;
    int swap;
    u32 temp_buf12[512/4];
printf("test Brightness\n");
    blup = getupBacklight();
    bldown = getdownBacklight();
    swap =getswap();

    buf_change=0;
    down_buf_num=0;

    initInput();
    for (; ; )
    {
        inputdata = updateInput();
        if (inputdata != 0)
        {
            if ( inputdata->keysDown & KEY_B )
            {
printf("test Brightness end\n");
                return ;
            }
            if ( inputdata->keysDown & KEY_A ) 
            {
                data = getBrightness();
                setBrightness(1+data);
            }
            if ( inputdata->keysDown & KEY_UP ) 
            {
                blup = blup ^ 1;
                setBacklight(blup,bldown);
            }
     
            if ( inputdata->keysDown & KEY_DOWN ) 
            {
                bldown = bldown ^ 1;
                setBacklight(blup,bldown);
            }

            if ( inputdata->keysDown & KEY_LEFT ) 
            {
                swap = swap ^ 1; 
                setswap(swap);
            }

//            if ( inputdata->keysDown & KEY_SELECT ) 
//            {
//                setshutdown();
//            }

        } 
}
}

void draw_dot(int x,int y,u32 buf)
{
	x=min(x,256);
	y=min(y,192);
        buf +=(x*2) + (y*256*2);
        *(vu16*)buf = draw_col;
	
}

touchPosition touchPtOld;

void test_touch()
{
    u32 touch_buf[256*192*2/4];
        INPUT * inputdata;
    int buf_change;
    int down_buf_num;
    u32 getbuf;
        initInput();
printf("test touch\n");
    memset(touch_buf,0,256*192*2);
    buf_change=0;
    down_buf_num=0;
    draw_col= RGB15(0x1f,0,0);
    for (; ; )
    {
        inputdata = updateInput();
        if (inputdata != 0)
        {
            if ( inputdata->keysDown & KEY_B )
            {
			printf("test touch end\n");
                return ;
            }
            if ( inputdata->keysDown & KEY_A )
            {
			memset(touch_buf,0,256*192*2);
                buf_change=1;
            }

            if (inputdata->touchHeld)
            {
                draw_dot(inputdata->touchPt.x,inputdata->touchPt.y,(u32)touch_buf);
				buf_change=1;
            }
        }
        if ((buf_change == 1)&&(down_buf_num == 0))
        {

            down_buf_num=get_video_down_buf();
            if (down_buf_num >=0)
            {
                getbuf=get_buf_form_bufnum(down_buf_num);
                memcpy((char*)getbuf,touch_buf,256*192*2);
                down_buf_num++;
                buf_change = 0;
            }
        }
        if (down_buf_num > 0)
        {
            if(update_buf(down_buf_num - 1) >= 0)
            {
                down_buf_num=0;
            }
        }
    }
}


int Start(char *filename);
int read_audio_wav(s16 *bufl,s16 *bufr);




//
//static inline unsigned int GetTimer(){
//  struct timeval tv;
//  struct timezone tz;
//  float s;
//  gettimeofday(&tv,&tz);
//  s=tv.tv_usec;s*=0.000001;s+=tv.tv_sec;
//  return (tv.tv_sec*1000000+tv.tv_usec);
//}  
//
	 unsigned int t[2];
	 unsigned int t1[2];
	 unsigned int t2[2];
	 unsigned int t3[2];
static inline unsigned int GetTimer(){
 JZ_GetTimer(t1);
 JZ_DiffTimer(t2,t1,t);
 return ((t2[1]*1000000) +t2[0] );
}  






#if 1
void test(char *filename,char *filename1)
{ 
	video_set_buf.frequence=25;
    video_set_buf.timer_l_data=32768;
    video_set_buf.timer_l_ctr=0;
    video_set_buf.timer_h_data=32768/25;
    video_set_buf.timer_h_ctr=0;
    video_set_buf.width=256;
    video_set_buf.height=192;
    video_set_buf.data_type=0;
    video_set_buf.play_buf=0;//a=0 b=1
    video_set_buf.swap=0;//0 a up

    audio_set_buf.sample=32768;
    audio_set_buf.timer_l_data=32768;
    audio_set_buf.timer_l_ctr=0;
    audio_set_buf.timer_h_data=4096;
    audio_set_buf.timer_h_ctr=0;
    audio_set_buf.sample_size=4096;
    audio_set_buf.sample_bit=16;
    audio_set_buf.stereo=1;

    SoundDataL.len=4096*2;
    SoundDataL.rate=32768;
    SoundDataL.vol=0x7F;
    SoundDataL.pan=0;
    SoundDataL.format=2;

    SoundDataR.len=4096*2;
    SoundDataR.rate=32768;
    SoundDataR.vol=0x7F;
    SoundDataR.pan=127;
    SoundDataR.format=2;

    SoundDataL_T.count=1;
    memcpy( &SoundDataL_T.data[0], &SoundDataL, sizeof(TransferSoundData) );
    SoundDataR_T.count=1;
	memcpy( &SoundDataR_T.data[0], &SoundDataR, sizeof(TransferSoundData) );

printf("before initInput\n");
    initInput();
	printf("initInput ok\n");
    enable_play();
//	printf("enable_play ok\n");

	JZ_StartTimer();
	JZ_GetTimer(t);

#if 1
//-----------NDS屏幕测试------------------------
	INPUT * inputdata;
	int getbuf=get();
	update_down_dis( getbuf, 256*192*2 );
	update_up_dis( getbuf, 256*192*2 );

//-----------NDS按键与触摸屏测试------------------------
	while (0)
    {
		inputdata = updateInput();
		if (inputdata != 0)
		{
			if(( inputdata->keysDown & KEY_A ))
			{
				printf( "KEY A was pressed!\n" );
			}
			else if (inputdata->touchHeld)
			{
				printf( "%d,%d\n", inputdata->touchPt.x, inputdata->touchPt.y );
			}
			else if(( inputdata->keysDown & KEY_B ))
			{
				printf( "KEY B was pressed!\n" );
				printf( "KEY test end!\n" );
				break;
			}			
		}
	}
//-----------NDS触摸屏测试------------------------
	//test_touch();
#endif
}


//--------------------------------------------------------------------------------
//更新上屏显示
//--------------------------------------------------------------------------------
void update_up_dis( u32 getbuf, u32 len)
{
		int up_ready,getbuf1,i;
		up_ready=get_video_up_buf();
		if (up_ready>=0)//up_ready一定要是int型不然会系统错误
		{			
			getbuf1=get_buf_form_bufnum(up_ready);
			for (i=0;i<len;i+=4)
			{
				*(u32*)(getbuf1+i)=*(u32*)(getbuf+i);
			}
			update_buf(up_ready);				
		}       
}

//--------------------------------------------------------------------------------
//更新下屏显示
//--------------------------------------------------------------------------------
void update_down_dis( u32 getbuf, u32 len)
{
		int up_ready,getbuf1,i;
		up_ready=get_video_down_buf();//up_ready一定要是int型不然会系统错误
		if (up_ready>=0)
		{			
			getbuf1=get_buf_form_bufnum(up_ready);
			for (i=0;i<len;i+=4)
			{
				*(u32*)(getbuf1+i)=*(u32*)(getbuf+i);
			}
			update_buf(up_ready);				
		}       
}

#else
void test(char *filename,char *filename1)
{
    //int err_flag;
    int read_len;
    int i,getbuf,getbuf1,flag,flag1;
    //u32 audio_iqe_num_w;
    //u32 video_iqe_num_w;
    INPUT * inputdata;
    int pause;
    int vol;
u32 timedata,timedata1,video_time;


    int up_ready;
    int av_ready;
   //u32 getwritebuf,getwritebuf2;
    //int fpdsm1;
    video_set_buf.frequence=25;
    video_set_buf.timer_l_data=32768;
    video_set_buf.timer_l_ctr=0;
    video_set_buf.timer_h_data=32768/25;
    video_set_buf.timer_h_ctr=0;
    video_set_buf.width=256;
    video_set_buf.height=192;
    video_set_buf.data_type=0;
    video_set_buf.play_buf=0;//a=0 b=1
    video_set_buf.swap=0;//0 a up

    audio_set_buf.sample=32768;
    audio_set_buf.timer_l_data=32768;
    audio_set_buf.timer_l_ctr=0;
    audio_set_buf.timer_h_data=4096;
    audio_set_buf.timer_h_ctr=0;
    audio_set_buf.sample_size=4096;
    audio_set_buf.sample_bit=16;
    audio_set_buf.stereo=1;

    SoundDataL.len=4096*2;
    SoundDataL.rate=32768;
    SoundDataL.vol=0x7F;
    SoundDataL.pan=0;
    SoundDataL.format=2;

    SoundDataR.len=4096*2;
    SoundDataR.rate=32768;
    SoundDataR.vol=0x7F;
    SoundDataR.pan=127;
    SoundDataR.format=2;

    SoundDataL_T.count=1;
    memcpy( &SoundDataL_T.data[0], &SoundDataL, sizeof(TransferSoundData) );
    SoundDataR_T.count=1;
    memcpy( &SoundDataR_T.data[0], &SoundDataR, sizeof(TransferSoundData) );



    opendsm(filename);
printf("opendsm ok\n");
    Start(filename1);
printf("wav ok\n");


    initInput();
printf("initInput ok\n");
    enable_play();
printf("enable_play ok\n");
i=0;
//while(i==0);

    flag1=0;
    pause=0;
    up_ready=0;
    av_ready=0;
getbuf=get();
flag=get_video_down_buf();
if (flag >=0)
{
	getbuf1=get_buf_form_bufnum(flag);
	for (i=0;i<(256*192*2);i+=4)
	{
		*(u32*)(getbuf1+i)=*(u32*)(getbuf+i);
	}
	update_buf(flag);
}

//test_touch();
//test_Brightness();

JZ_StartTimer();
//timedata=GetTimer(); 
 JZ_GetTimer(t);

video_time =0;

vol=0x7f;

printf( "test pause!\n" );


    while (flag1==0)
    {

        inputdata = updateInput();
        if (inputdata != 0)
        {
            if(( inputdata->keysDown & KEY_A ) && (pause ==0))
            {
                pause_play();
                pause=1;

            }

            if(( inputdata->keysDown & KEY_B ) && (pause ==1))
            {
                begin_pause_play();
                pause=0;
            }
            if ( inputdata->keysDown & KEY_RIGHT )
            {
                vol += 8;
                vol=min(vol,0x7f);
                setVol(vol,vol,vol);

            }
            if ( inputdata->keysDown & KEY_LEFT )
            {
                vol -= 8;
                vol=max(vol,0);
                setVol(vol,vol,vol);
            }
        }





        if (pause ==0)
        {
            if (up_ready ==0)
            {

                up_ready=get_video_up_buf();
                if (up_ready>=0)
                {
                        getbuf=get_buf_form_bufnum(up_ready);
                        up_ready++;
                        read_len=read_video((u16*)getbuf);
                         if(read_len == -1)
                         {
                             disenable_play();
                             return;//;exit(1);
                         }
                }
                else{
                    up_ready=0;
                }

                
            }
            if (up_ready!=0)
            {
		timedata1=GetTimer()-timedata; 
		if (timedata1 > video_time)
		{
                if(update_buf(up_ready-1) >= 0) {
                    up_ready=0;
			video_time += 40000;
			
                	};
		}
            }


            if (av_ready ==0)
            {
                av_ready=get_audio_buf();
                if (av_ready>=0)
                {
                        getbuf=get_buf_form_bufnum(av_ready);
                        av_ready++;
                        read_len=read_audio_wav((u16*)getbuf,(u16*)(getbuf+8192));
                         if(read_len == -1)
                         {
                             disenable_play();
                             return;//exit(1);
                         }
                }
                else{
                    av_ready=0;
                }
                
            }
            if (av_ready!=0)
            {
                if(update_buf(av_ready-1) >= 0) {
                    av_ready=0;
                };
            }

        
         }
    }
}

#endif


/////////////////////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------

static int WaveFile_ReadWaveChunk(void)
{
  fseek(FileHandle,0x14,SEEK_SET);
  fread((void*)&wfex,sizeof(wfex),1,FileHandle);
  
  if(wfex.wFormatTag!=0x0001){
    printf("Illigal CompressFormat Error. wFormatTag=0x%x\n",wfex.wFormatTag);
    return(false);
  }
  
  if((wfex.nChannels!=1)&&(wfex.nChannels!=2)){
    printf("Channels Error. nChannels=%d\n",wfex.nChannels);
    return(false);
  }
  
  if((wfex.wBitsPerSample!=8)&&(wfex.wBitsPerSample!=16)){
    printf("Bits/Sample Error. wBitsPerSample=%d\n",wfex.wBitsPerSample);
    return(false);
  }
  
  printf("fmt:0x%x chns:%d\n",wfex.wFormatTag,wfex.nChannels);
  printf("Smpls/Sec:%d AvgBPS:%d\n",wfex.nSamplesPerSec,wfex.nAvgBytesPerSec);
  printf("BlockAlign:%d Bits/Smpl:%d\n",wfex.nBlockAlign,wfex.wBitsPerSample);
  
  return(true);
}

static int WaveFile_SeekDataChunk(void)
{
  u32 DataSize;
  fseek(FileHandle,0,SEEK_SET);
  
  // find "data"
  {
    char *readbuf=(char*)malloc(256); 
    int size=0;
    int ofs=0;
    
    size=fread(readbuf,1,256,FileHandle);
    if(size<4){
      free(readbuf);
      printf("can not find data chunk.\n");
      return(false);
    }
    
    while(true){
      if(readbuf[ofs]=='d'){
        if((readbuf[ofs+1]=='a')&&(readbuf[ofs+2]=='t')&&(readbuf[ofs+3]=='a')){
          free(readbuf);
          fseek(FileHandle,ofs+4,SEEK_SET);
          break;
        }
      }
      ofs++;
      if(ofs==(size-4)){
        free(readbuf);
        printf("can not find data chunk.\n");
        return(false);
      }
    }
  }
  

  fread(&DataSize,4,1,FileHandle);
  
  if(DataSize==0){
    printf("DataSize is NULL\n");
    return(false);
  }
  
  BytePerSample=0;
  
  if((wfex.nChannels==1)&&(wfex.wBitsPerSample==8)) BytePerSample=1;
  if((wfex.nChannels==2)&&(wfex.wBitsPerSample==8)) BytePerSample=2;
  if((wfex.nChannels==1)&&(wfex.wBitsPerSample==16)) BytePerSample=2;
  if((wfex.nChannels==2)&&(wfex.wBitsPerSample==16)) BytePerSample=4;
  
  if(BytePerSample==0){
    printf("Illigal Channels or Bits/Sample or no data\n");
    return(false);
  }
  
  SampleCount=DataSize/BytePerSample;
  
  DataTopOffset=ftell(FileHandle);
  
  printf("DataTop:%d DataSize:%dbyte\n",DataTopOffset,DataSize);
  
  return(true);
}

static int WaveFile_Open(void)
{
 u32 RIFFID;
  fseek(FileHandle,0,SEEK_SET);
  
 
  fread(&RIFFID,4,1,FileHandle);
  
  if(RIFFID!=0x46464952){ // check "RIFF"
    printf("no RIFFWAVEFILE error.");
    printf("topdata:0x%04x\n",RIFFID);
    return(false);
  }
  
  if(WaveFile_ReadWaveChunk()==false) return(false);
  if(WaveFile_SeekDataChunk()==false) return(false);
  
  return(true);
}
void Free(void)
{
  if(ReadBuffer!=NULL){
    free(ReadBuffer); ReadBuffer=NULL;
  }
}

int Update(s16 *lbuf,s16 *rbuf)
{
  int SampleCount;
  u32 idx;
  SampleCount=fread(ReadBuffer,1,SamplePerFrame*BytePerSample,FileHandle)/BytePerSample;
  
  if(wfex.wBitsPerSample==8){
    if(wfex.nChannels==1){ // 8bit mono
      s8 *readbuf=(s8*)ReadBuffer;
      for( idx=SampleCount;idx!=0;idx--){
        *lbuf=(((s16)*readbuf)-128)<<8;
        lbuf++; readbuf++;
      }
      }else{ // 8bit stereo
      s8 *readbuf=(s8*)ReadBuffer;
      for(idx=SampleCount;idx!=0;idx--){
        *lbuf=(((s16)*readbuf)-128)<<8;
        lbuf++; readbuf++;
        *rbuf=(((s16)*readbuf)-128)<<8;
        rbuf++; readbuf++;
      }
    }
    }else{
    if(wfex.nChannels==1){ // 16bit mono
      s16 *readbuf=(s16*)ReadBuffer;
      for(idx=SampleCount;idx!=0;idx--){
        *lbuf=*readbuf;
        lbuf++; readbuf++;
      }
      }else{ // 16bit stereo
      s16 *readbuf=(s16*)ReadBuffer;
      for(idx=SampleCount;idx!=0;idx--){
        *lbuf=*readbuf;
        lbuf++; readbuf++;
        *rbuf=*readbuf;
        rbuf++; readbuf++;
      }
    }
  }

  SampleOffset+=SampleCount;
  if (SampleCount == 0) return -1;
  return(SampleCount);

}

s32 GetPosMax(void)
{
  return(SampleCount);
}

s32 GetPosOffset(void)
{
  return(SampleOffset);
}

void SetPosOffset(s32 ofs)
{
  if(ofs<0) ofs=0;
  ofs&=~3;
  
  SampleOffset=ofs;
  
  fseek(FileHandle,DataTopOffset+(SampleOffset*BytePerSample),SEEK_SET);
}

u32 GetChannelCount(void)
{
  return(wfex.nChannels);
}

u32 GetSampleRate(void)
{
  return(wfex.nSamplesPerSec);
}

u32 GetSamplePerFrame(void)
{
  return(SamplePerFrame);
}

int GetInfoIndexCount(void)
{
  return(3);
}



int GetInfoStrUTF8(int idx,char *str,int len)
{
  return(false);
}

// -----------------------------------------------------------
int read_audio_wav(s16 *bufl,s16 *bufr)
{
    return Update((s16*)bufl,(s16*)bufr);
}

int Start(char *filename)
{
    if ((FileHandle=fopen(filename,"rb"))==NULL)
    {
        printf("open error wav\n");
        return  false;
    }
  
  if(WaveFile_Open()==false){
    return(false);
  }
  
  SampleOffset=0;
  
  if(wfex.nSamplesPerSec<=48000){
    SamplePerFrame=2560/4;
    }else{
    if(wfex.nSamplesPerSec<=96000){
      SamplePerFrame=2560/2;
      }else{
      SamplePerFrame=2560/1;
    }
  }
  SamplePerFrame=4096;
  
  ReadBuffer=(u32*)malloc(SamplePerFrame*BytePerSample);
  if(ReadBuffer==NULL){
    printf("out of memory.\n");
    return(false);
  }

    audio_set_buf.sample=wfex.nSamplesPerSec;
    audio_set_buf.sample_size=SamplePerFrame;
    audio_set_buf.sample_bit=16;    
    audio_set_buf.data_type=0;    
    audio_set_buf.stereo= wfex.nChannels == 2;   
    audio_set_buf.timer_l_data=audio_set_buf.sample;
    audio_set_buf.timer_l_ctr=0;
    audio_set_buf.timer_h_data=audio_set_buf.sample_size;
    audio_set_buf.timer_h_ctr=0;

    printf("sample=%d,%x,%d\n",audio_set_buf.sample,audio_set_buf.sample_size,audio_set_buf.stereo);

/*
    while (1)
    {
        int i,getwritebuf,getwritebuf2;
        
        i = audio_buf_write - audio_buf_read;
        if ((i == 0) || ((i &3 ) !=0))
        {
            getwritebuf = audio_buf[audio_buf_write&3];
            getwritebuf2 = getwritebuf + (audio_set_buf.sample_size * 2);
            Update((s16*)getwritebuf,(s16*)getwritebuf2);
            audio_buf_write++;

        }
        
    }
*/

  return(true);
}
