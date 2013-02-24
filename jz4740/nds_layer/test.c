#include <bsp.h>
#include <jz4740.h>
#include <mipsregs.h>
//#include <jzfs_api.h>
#include "common.h"

#include "test.h"  
//#define BIT(a) (1<<a)
extern u32 get(void);
extern void test(char *filename,char *filename1);
u32 temp32[0x200];
u32 temp32_1[(192*256*2 +512)/4];
u32 mptr;

extern u32 kmalloc_ptr_uncache;


struct main_buf *pmain_buf;
int MP4_fd;
int MP4_buf;

u32 get_buf_form_bufnum(int num)
{
    return (pmain_buf->buf_st_list[num].offset + MP4_buf);

}
int err_up=0;
int nds_buf_video_up_notfull(void)
{
    int i;
    i=pmain_buf->nds_video_up_w-pmain_buf->nds_video_up_r;
    if (i>4 )
    {
    dgprintf("video_up err=%x,%x,%x\n",i,pmain_buf->nds_video_up_w,pmain_buf->nds_video_up_r);

    }
	if (i==0 || ((i&3) != 0)) return 1;
	else  return 0;
}
int get_video_up_buf(void)
{
    int ret = -1;
    if (pmain_buf->buf_st_list[buf_video_up_0].isused == 0)
    {
        ret= buf_video_up_0;
    }
    if (pmain_buf->buf_st_list[buf_video_up_1].isused == 0)
    {
        ret= buf_video_up_1;
    }

//if(ret < 0)
//    printf("up %d, %d\n", 
//      pmain_buf->buf_st_list[buf_video_up_0].isused,
//      pmain_buf->buf_st_list[buf_video_up_1].isused);
    return ret;
}

int nds_buf_video_down_notfull(void)
{
    int i;
    i=pmain_buf->nds_video_down_w-pmain_buf->nds_video_down_r;
    if (i>4 )
    {
    dgprintf("video_down err=%x,%x,%x\n",i,pmain_buf->nds_video_down_w,pmain_buf->nds_video_down_r);

    }
	if (i==0 || ((i&3) != 0)) return 1;
	else  return 0;
}
int get_video_down_buf(void)
{
    if (pmain_buf->buf_st_list[buf_video_down_0].isused == 0)
    {
        return buf_video_down_0;
    }
    if (pmain_buf->buf_st_list[buf_video_down_1].isused == 0)
    {
        return buf_video_down_1;
    }
    return -1;
}

int nds_buf_audio_notfull(void)
{
    int i;
    i=pmain_buf->nds_audio_w - pmain_buf->nds_audio_r;
    if (i>4 )
    {
    dgprintf("audio err=%x\n",i);

    }
//if(i==0)
//    printf("down overflow\n");
//	if (i==0 || ((i&3) != 0)) return 1;
//    if (i <= 1) return 1;
//	else  return 0;
    return i;
}
int get_audio_buf(void)
{
//    if(nds_buf_audio_notfull()==0)
//        return -1;

    if (pmain_buf->buf_st_list[buf_audio_0].isused == 0)
    {
        return buf_audio_0;
    }
    if (pmain_buf->buf_st_list[buf_audio_1].isused == 0)
    {
        return buf_audio_1;
    }
    
    return -1;
}

int get_nds_set_buf(void)
{
    if (pmain_buf->buf_st_list[buf_c0_set].isused == 0)
    {
        return buf_c0_set;
    }
    return -1;
}

int update_buf(int bufnum)
{
#if 0
    switch (bufnum)
    {
/*
	case buf_video_up_0:
    case buf_video_up_1:
            if(nds_buf_video_up_notfull()==0)
                return -1;
            break;
    case buf_video_down_0:
    case buf_video_down_1:
            if(nds_buf_video_down_notfull()==0)
                return -1;
            break;
*/
/*
    case buf_audio_0:
    case buf_audio_1:
            if(nds_buf_audio_notfull()==0)
                return -1;
            break;
*/
    }
#endif

  if (pmain_buf->buf_st_list[bufnum].isused == 0)
    ioctl(MP4_fd, IQE_UPDATE, bufnum);
    return 0;
}
struct key_buf  last_input;
struct key_buf  curr_input;

void clear_input_buf(void)
{
    pmain_buf->key_read_num=pmain_buf->key_write_num= 0;
    memset((char*)&last_input,0,sizeof(last_input));
    memset((char*)&curr_input,0,sizeof(curr_input));
}

void get_nds_time(struct rtc *time)
{
    time->year= pmain_buf->nds_rtc.year;
    time->month= pmain_buf->nds_rtc.month;
    time->day= pmain_buf->nds_rtc.day;
    time->weekday= pmain_buf->nds_rtc.weekday;
    time->hours= pmain_buf->nds_rtc.hours;
    time->minutes= pmain_buf->nds_rtc.minutes;
    time->seconds= pmain_buf->nds_rtc.seconds;
}


#if 0
int update_input_form_buf(void)
{
    struct key_buf* keyp;
    if (pmain_buf->key_read_num != pmain_buf->key_write_num)
    {
		int i = pmain_buf->key_write_num-pmain_buf->key_read_num;
		if( i>5)
				pmain_buf->key_read_num = pmain_buf->key_write_num-2;
        keyp =(struct key_buf*)(MP4_buf + pmain_buf->key_buf_offset + ((pmain_buf->key_read_num & KEY_MASK) * sizeof(struct key_buf)));
        //last_input = curr_input;
        memcpy(&curr_input,keyp,sizeof(struct key_buf));
        pmain_buf->key_read_num++;
        return 0;
    }
    return -1;
}
#endif

/*
int update_input_form_buf(void)
{
	struct key_buf keyp;
#define  MP4_KEYBD_WAIT 0x7755
    if (pmain_buf->key_read_num != pmain_buf->key_write_num)
    {
		int i = pmain_buf->key_write_num-pmain_buf->key_read_num;
		if( i>5)
				pmain_buf->key_read_num = pmain_buf->key_write_num-2;
#if GG34_DEBUG
	printf( "key_read_num is %d\n",pmain_buf->key_read_num );
	printf( "key_write_num is %d\n",pmain_buf->key_write_num );
#endif
        //keyp =(struct key_buf*)(MP4_buf + pmain_buf->key_buf_offset + ((pmain_buf->key_read_num & KEY_MASK) * sizeof(struct key_buf)));
		ioctl( MP4_fd,MP4_KEYBD_WAIT,(unsigned long)(&keyp) );	
        memcpy(&curr_input,&keyp,sizeof(struct key_buf));

#if GG34_DEBUG
		//pmain_buf->key_read_num++;
	printf( "curr_input.x is 0x%04x\n",curr_input.x );
	printf( "curr_input.y is 0x%04x\n",curr_input.y );
	printf( "curr_input.key is 0x%04x\n",curr_input.key );
#endif
        return 0;
    }
    return -1;
}
*/

static u16 keys = 0;
static u16 keysold = 0;
static u16 keysrepeat = 0;

static u8 delay = 5, repeat = 4, count = 30;
//------------------------------------------------------------------------------
u32 keysDown(void) {
//------------------------------------------------------------------------------
	return (keys ^ keysold) & keys;
}

//------------------------------------------------------------------------------
int scanKeys(void) {
//------------------------------------------------------------------------------
	keysold = keys;
    if (update_input_form_buf() <0)
    {
        return -1;
    }
	keys = curr_input.key;

    if ( delay != 0 ) {
        if ( keys != keysold ) {
            count = delay ;
            keysrepeat = keysDown() ;
        }
        count--;
        if ( count == 0 ) {
            count = repeat;
            keysrepeat = keys;
        }
    }
return 0;
}

//------------------------------------------------------------------------------
u32 keysHeld(void) {
//------------------------------------------------------------------------------
	return keys;
}


//------------------------------------------------------------------------------
u32 keysDownRepeat(void) {
//------------------------------------------------------------------------------
	u32 tmp = keysrepeat;

    keysrepeat = 0;

    return tmp;
}

//------------------------------------------------------------------------------
void keysSetRepeat( u8 setDelay, u8 setRepeat ) {
//------------------------------------------------------------------------------
    delay = setDelay ;
    repeat = setRepeat ;
    count = delay ;
    keysrepeat = 0 ;
}

//------------------------------------------------------------------------------
u32 keysUp(void) {
//------------------------------------------------------------------------------
	return (keys ^ keysold) & (~keys);
}


static INPUT inputs;
static INPUT lastInputs;

void initInput(void)
{
    clear_input_buf();
    keysSetRepeat( 10, 1 );
    memset( &inputs, 0, sizeof(inputs) );
    memset( &lastInputs, 0, sizeof(inputs) );
}

#if 0
INPUT *updateInput(void)
{
    memset( &inputs, 0, sizeof(inputs) );
    if(scanKeys() <0) 
	{
#ifdef DEBUG
		//printf("lastInputs.touchPt.x is 0x%02x\n",lastInputs.touchPt.x);
		//printf("lastInputs.touchPt.y is 0x%02x\n",lastInputs.touchPt.y);
		//Delay(DELAYNUM);
#endif
		if(lastInputs.touchPt.x == 0 && lastInputs.touchPt.y == 0)
		{
			lastInputs = inputs;
			return &inputs;
		}
	}
	inputs.touchPt.x = curr_input.x;
	inputs.touchPt.y = curr_input.y;

	if( inputs.touchPt.x == 0 && inputs.touchPt.y == 0 ) 
	{
		if( lastInputs.touchHeld ) 
		{
#ifdef DEBUG
		//printf("inputs.touchUp = true;\n");
		//Delay(DELAYNUM);
#endif
			inputs.touchPt = lastInputs.touchPt;	
			inputs.touchUp = true;
		} 
		else 
		{
			inputs.touchUp = false;
		}
		inputs.touchDown = false;
		inputs.touchHeld = false;
	} 
	else 
	{
		if( !lastInputs.touchHeld ) 
		{
			inputs.touchDown = true;
		} 
		else 
		{
			inputs.movedPt.x = inputs.touchPt.x - lastInputs.touchPt.x;
			inputs.movedPt.y = inputs.touchPt.y - lastInputs.touchPt.y;
			inputs.touchMoved = (0 != inputs.movedPt.x) || (0 != inputs.movedPt.y);
			inputs.touchDown = false;
		}
		inputs.touchUp = false;
		inputs.touchHeld = true;
	}


	inputs.keysDown = keysDown();
	inputs.keysUp = keysUp();
	inputs.keysHeld = keysHeld();
	inputs.keysDownRepeat = keysDownRepeat();

	lastInputs = inputs;

#ifdef DEBUG
	/*	printf("inputs.touchPt.x is 0x%02x\n",inputs.touchPt.x);
		printf("inputs.touchPt.y is 0x%02x\n",inputs.touchPt.y);
		printf("inputs.touchUp is 0x%02x\n",inputs.touchUp);*/
#endif
	return &inputs;
}
#endif

int update_input_form_buf(void)
{
    struct key_buf *keyp;
    if (pmain_buf -> key_read_num != pmain_buf -> key_write_num)
    {
		int i = pmain_buf -> key_write_num - pmain_buf -> key_read_num;
		if(i > 5)
				pmain_buf -> key_read_num = pmain_buf -> key_write_num -2;
        keyp =(struct key_buf*)(MP4_buf + pmain_buf -> key_buf_offset +
              ((pmain_buf -> key_read_num & KEY_MASK) * sizeof(struct key_buf)));
        //last_input = curr_input;
        memcpy(&inputs, keyp, sizeof(struct key_buf));
        pmain_buf->key_read_num++;
        return 0;
    }
    return -1;
}

int updateInput(struct key_buf *inputs)
{
    struct key_buf *keyp;

    int i = pmain_buf -> key_write_num - pmain_buf -> key_read_num;
	if(i > 1)
		pmain_buf -> key_read_num = pmain_buf -> key_write_num -1;
    keyp =(struct key_buf*)(MP4_buf + pmain_buf -> key_buf_offset +
           ((pmain_buf -> key_read_num & KEY_MASK) * sizeof(struct key_buf)));

    memcpy(inputs, keyp, sizeof(struct key_buf));

    return 0;
}




INPUT * getInput(void)
{
    return &inputs;
}
int set_nds_var( u32 *data,int len)
{

    u32 *temp32p;

    int buf_num,flag,flag1;
    flag =0;
    while (flag == 0)
    {
        buf_num =get_nds_set_buf();
        if (buf_num >= 0) break;
    }
    temp32p = (u32*)(pmain_buf->buf_st_list[buf_num].offset + MP4_buf);

    memset(temp32p,0,512);
    len =min(len,512);
    memcpy(temp32p,data,len);
    flag =0;
    while (flag == 0)
    {
        flag1=update_buf(buf_num);
        if (flag1 >= 0) break;
    }
    return 0;
}

int getBrightness(void)
{
    return curr_input.brightness &3;
}
void setBrightness(int data)
{

    int flag,flag1;
    u32 tempbuf12[512/4];
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_BR;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=data & 3;

    set_nds_var(tempbuf12,512);

} 


int getswap(void)
{
    return (curr_input.brightness>>2) &1;
}
void setswap(int data)
{
    int flag,flag1;
    u32 tempbuf12[512/4];
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_SWAP;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=data & 1;

    set_nds_var(tempbuf12,512);



}
int getupBacklight(void)
{
    return (curr_input.brightness>>4) &1;
}
void setBacklight(int up,int down)
{
    int flag,flag1;
    u32 tempbuf12[512/4];
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_BACKLIGHT;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=((down & 1) | ((up & 1) <<1));

    set_nds_var(tempbuf12,512);

}
int getdownBacklight(void)
{
    return (curr_input.brightness>>3) &1;
}

void SetSupend(void)
{
    int flag,flag1;
    u32 tempbuf12[512/4];
    flag1=0;
    while (flag1 == 0)
    {
        ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)
        {
            break;
        }
    }
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_SUSPEND;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=1;

    set_nds_var(tempbuf12,512);
}

void WakeUp(void)
{
    int flag,flag1;
    u32 tempbuf12[512/4];
    flag1=0;
    while (flag1 == 0)
    {
        ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)
        {
            break;
        }
    }
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_WAKEUP;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=1;
    set_nds_var(tempbuf12,512);
}

void setshutdown(void)
{
    int flag,flag1;
    u32 tempbuf12[512/4];
    flag1=0;
    while (flag1 == 0)
    {
        ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)
        {
            break;
        }
    }
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_SHUTDOWN;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=1;
//printf("setshutdown set_nds_var\n");
    set_nds_var(tempbuf12,512);
    while(1);
}
void setVol(int l_vol,int r_vol,int mix_vol)
{
    int flag,flag1;
    struct audio_set_vol *audio_set_volp;
    u32 tempbuf12[512/4];
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_VOL;
    tempbuf12[2]=0x60;
    audio_set_volp =(struct audio_set_vol *)&tempbuf12[0x60/4];
    audio_set_volp->l_vol=l_vol;
    audio_set_volp->r_vol=r_vol;
    audio_set_volp->mix_vol=mix_vol;
    set_nds_var(tempbuf12,512);
}


int pause_play(void)
{
    int flag,flag1;
    u32 tempbuf12[512/4];
    flag1=0;
    while (flag1 == 0)
    {
        ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)
        {
            break;
        }
    }
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_ENABLE_VIDEO;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=0;

    tempbuf12[3]=IS_SET_ENABLE_AUDIO;
    tempbuf12[4]=0x64;
    tempbuf12[0x64/4]=0;
    set_nds_var(tempbuf12,512);
return 0;
}
int begin_pause_play(void)
{
    int flag,flag1;
    u32 tempbuf12[512/4];
    flag1=0;
    while (flag1 == 0)
    {
        ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)
        {
            break;
        }
    }
    memset(tempbuf12,0,512);
    tempbuf12[0]=0xc0;
    tempbuf12[1]=IS_SET_ENABLE_VIDEO;
    tempbuf12[2]=0x60;
    tempbuf12[0x60/4]=1;

    tempbuf12[3]=IS_SET_ENABLE_AUDIO;
    tempbuf12[4]=0x64;
    tempbuf12[0x64/4]=1;
    set_nds_var(tempbuf12,512);
return 0;
}

void init_fpga(void)
{
    int flag,flag1;

    u32 *temp32p;
    int offset;
    int offsetindex;
    struct video_set *video_setp;
    struct audio_set *audio_setp;

    u32 tempbuf12[512/4];
    flag1=0;
    while (flag1 == 0)
    {
        ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)
        {
            break;
        }
    }

   pmain_buf->nds_video_up_w=0;
   pmain_buf->nds_video_up_r=0;
   pmain_buf->nds_video_down_w=0;
   pmain_buf->nds_video_down_r=0;
   pmain_buf->nds_audio_w=0;
   pmain_buf->nds_audio_r=0;

   pmain_buf->key_write_num=0;
   pmain_buf->key_read_num=0;
   int i;
    for (i=0;i<10 ;i++ )
    {
        pmain_buf->buf_st_list[i].isused  =0;
    }


    memset(tempbuf12,0,512);
    temp32p=tempbuf12;

    temp32p[0]=0xc0;

    offset = 0x60;
    offsetindex=0x4;
    temp32p[offsetindex/4]=IS_SET_CLEAR_VAR;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=1;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_VIDEO;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=1;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_AUDIO;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=1;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_KEY;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=1;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_RTC;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=1;
    offset+=4;


    set_nds_var(tempbuf12,512);
}


int test_main(int argc, char **argv)
{
//    char filenamebuf[100];
//    char filenamebuf1[100];
//    if (argc < 3)
//    {
//        printf("err\n\n");
//        return -1;
//    }

//    strcpy(filenamebuf, &argv[1][0]);
//    strcpy(filenamebuf1, &argv[2][0]);
	dgprintf("begin\n");
    MP4_init_module();

    dgprintf("Module initialed\n");

    //get buff
	MP4_buf = kmalloc_ptr_uncache;//(u32)mmap(0, LEN, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, MP4_fd, 0);
	//if(MP4_buf == (u32)MAP_FAILED) {
	//	printf("mmap() failed\n");
	//	exit(1);  
	//}
    pmain_buf=(struct main_buf*)MP4_buf;//0x4000 * 4

    init_fpga();
	dgprintf("MP4_buf =%x",MP4_buf);
    dgprintf("pmain_buf[0] =%x,%x\n",pmain_buf->buf_st_list[0].offset,pmain_buf->buf_st_list[0].len);
    dgprintf("pmain_buf[1] =%x,%x\n",pmain_buf->buf_st_list[1].offset,pmain_buf->buf_st_list[1].len);
    dgprintf("pmain_buf[2] =%x,%x\n",pmain_buf->buf_st_list[2].offset,pmain_buf->buf_st_list[2].len);
    dgprintf("pmain_buf[3] =%x,%x\n",pmain_buf->buf_st_list[3].offset,pmain_buf->buf_st_list[3].len);
    dgprintf("pmain_buf[4] =%x,%x\n",pmain_buf->buf_st_list[4].offset,pmain_buf->buf_st_list[4].len);
    dgprintf("pmain_buf[5] =%x,%x\n",pmain_buf->buf_st_list[5].offset,pmain_buf->buf_st_list[5].len);
    dgprintf("pmain_buf[6] =%x,%x\n",pmain_buf->buf_st_list[6].offset,pmain_buf->buf_st_list[6].len);
    dgprintf("nds_video_up =%x,%x\n",pmain_buf->nds_video_up_w,pmain_buf->nds_video_up_r);
    dgprintf("nds_video_down =%x,%x\n",pmain_buf->nds_video_down_w,pmain_buf->nds_video_down_r);
    dgprintf("nds_audio =%x,%x\n",pmain_buf->nds_audio_w,pmain_buf->nds_audio_r);

    initInput();
//    test(filenamebuf,filenamebuf1);
#if 0
	JZFS_DIR * dir;
	dir = jzfs_OpenDir("mmc:\\_SYSTEM" );
	if( dir == NULL )
	{
		dgprintf("open _SYSTEM dir is NULL\n");
		 wait_press_b();
		 while(1);
	}
	jzfs_CloseDir( dir );

    JZFS_FILE *fp;
    dgprintf("p end\n");
#endif
    return 0;
}
int getmm1_ok(void)
{
    return (curr_input.brightness >>6) & 1;
}
int getmm2_ok(void)
{
    return (curr_input.brightness >>7) & 1;
}


