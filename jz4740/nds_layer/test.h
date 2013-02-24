/*
 * Include file for Ingenic Semiconductor's JZ4740 CPU.
 */
#ifndef __TEST_H__
#define __TEST_H__

//#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

/********************************************************
 * Macros to help debugging
 ********************************************************/

//…Ë±∏∫≈
#define MP4_MAJOR 88
#define MP4_MINOR 88

	#define min(x, y)	(((x)<(y))?(x):(y))
	#define max(x, y)	(((x)>(y))?(x):(y))



#define  false 0
#define  true 1

#ifndef BIT
#define BIT(a) (1<<a)
#endif
//typedef     unsigned char           u8;
//typedef     unsigned short int      u16;
//typedef     unsigned int            u32;
//typedef     unsigned long long int  u64;

//typedef     signed char             s8;
//typedef     signed short int        s16;
//typedef     signed int              s32;
//typedef     signed long long int    s64;

typedef     float                   f32;
typedef     double                  f64;


#define     vl                      volatile

typedef     vl u8                   vu8;
typedef     vl u16                  vu16;
typedef     vl u32                  vu32;
typedef     vl u64                  vu64;

typedef     vl s8                   vs8;
typedef     vl s16                  vs16;
typedef     vl s32                  vs32;
typedef     vl s64                  vs64;

typedef     vl f32                  vf32;
typedef     vl f64                  vf64;


typedef struct touchPosition {
	s16	x;
	s16	y;
} touchPosition;
typedef struct T_INPUT
{
    u32 keysHeld;
    u32 keysUp;
    u32 keysDown;
    u32 keysDownRepeat;
    touchPosition touchPt;
    touchPosition movedPt;
    int touchDown;
    int touchUp;
    int touchHeld;
    int touchMoved;
}INPUT;

enum {
	SET_CTR = 1,
	CLR_CTR,
	READ_STATE,
	READ_IO,
	WRITE_IO,
	IQE_WRITE,
	IQE_READ,
	READ_KEY,
	WRITE_BUF,
	READ_BUF,
    IQE_UPDATE,
	COMPLETE_STATE

};
typedef enum KEYPAD_BITS {
  KEY_A      = BIT(0),  //!< Keypad A button.
  KEY_B      = BIT(1),  //!< Keypad B button.
  KEY_SELECT = BIT(2),  //!< Keypad SELECT button.
  KEY_START  = BIT(3),  //!< Keypad START button.
  KEY_RIGHT  = BIT(4),  //!< Keypad RIGHT button.
  KEY_LEFT   = BIT(5),  //!< Keypad LEFT button.
  KEY_UP     = BIT(6),  //!< Keypad UP button.
  KEY_DOWN   = BIT(7),  //!< Keypad DOWN button.
  KEY_R      = BIT(8),  //!< Right shoulder button.
  KEY_L      = BIT(9),  //!< Left shoulder button.
  KEY_X      = BIT(10), //!< Keypad X button.
  KEY_Y      = BIT(11), //!< Keypad Y button.
  KEY_TOUCH  = BIT(12), //!< Touchscreen pendown.
  KEY_LID    = BIT(13)  //!< Lid state.
} KEYPAD_BITS;


enum {
	buf_video_up_0= 0,
	buf_video_up_1,
	buf_video_down_0,
	buf_video_down_1,
    buf_audio_0,
    buf_audio_1,
    buf_c0_set

};
enum {
        VIDEO_UP = 1,
        VIDEO_DOWN,
        AUDIO_G
};
enum {
        IS_SET_VIDEO=1, 
        IS_SET_AUDIO,
        IS_SET_ENABLE_VIDEO,
        IS_SET_ENABLE_AUDIO,
        IS_SET_ENABLE_KEY,
        IS_SET_ENABLE_RTC,
        IS_SET_CLEAR_VAR,

        IS_SET_VOL,
        IS_SET_BR,
        IS_SET_SWAP,
        IS_SET_BACKLIGHT,

        IS_SET_SUSPEND,
        IS_SET_WAKEUP,
        IS_SET_SHUTDOWN,
        IS_WRITE_IO
};
struct nds_iqe_st 
{
    u32 iqe_cmd;
    u32 iqe_ndsaddr;
    u32 iqe_cpuaddr;
    u32 iqe_datatype;
    u32 iqe_len;
    u32 iqe_num;
};
struct buf_st 
{
    u32 isused;
    u32 offset;
    u32 len;
    u32 use_len;
    u32 nds_max_len;
    u32 nds_cmd;
    u32 type;

}; 

struct rtc{
    vu8 year;		//add 2000 to get 4 digit year
    vu8 month;		//1 to 12
    vu8 day;		//1 to (days in month)

    vu8 weekday;	// day of week
    vu8 hours;		//0 to 11 for AM, 52 to 63 for PM
    vu8 minutes;	//0 to 59
    vu8 seconds;	//0 to 59
} ;
struct key_buf 
{
    u16 key;
    u16 x;
    u16 y;
    u16 brightness;

};
struct audio_set_vol
{
    int l_vol;
    int r_vol;
    int mix_vol;
};

struct main_buf
{
    u32 key_buf_offset;  
    u32 key_buf_len;
    u32 key_write_num;
    u32 key_read_num;

    struct buf_st buf_st_list[0x10];
    u32 tempbuff;
    u32 tempbuff_len;

    u32 nds_video_up_w;
    u32 nds_video_up_r;
    u32 nds_video_down_w;
    u32 nds_video_down_r;
    u32 nds_audio_w;
    u32 nds_audio_r;
    struct rtc nds_rtc;

    u32 nds_iqe_list_c0[0x10][512/4];


};


struct video_set
{
    int frequence;
    int timer_l_data;
    int timer_l_ctr;
    int timer_h_data;
    int timer_h_ctr;
    int width;
    int height;
    int data_type;
    int play_buf;//a=0 b=1
    int swap;//0 a up

};
struct audio_set
{
    int sample;
    int timer_l_data;
    int timer_l_ctr;
    int timer_h_data;
    int timer_h_ctr;
    int sample_size;
    int sample_bit;
    int data_type;
    int stereo;
};


//#define     BIT(aa)                (1<<aa)
#define		DMA_CHANNEL				0
#define		PA						0
#define		PB						1
//#define		PC						2
#define		PD						3
#define		DATA_PORT				PA
#define		INTERRUPT_PORT			PD

//#define		Uart_Printf				serial_puts

#define cpld_base_addr (0x08000000 | 0xa0000000)
#define cpld_base_step (1<<13)
//fifo base address write/read
#define cpld_fifo_addr      (cpld_base_addr)

//fifo control reg address write/read
#define cpld_ctr_addr       (cpld_base_addr + (cpld_base_step * 1))
#define nds_dir_bit             0
#define nds_iqe_out_bit         1
#define key2_cmd_enable_bit     2
#define key2_data_enable_bit    3
#define key2_init_enable_bit    4
#define fifo_clear_bit          5
#define nds_cmd_iqe_type_bit    6
#define waitdisable_bit         7

//fifo state reg address read
#define cpld_state_addr     (cpld_base_addr + (cpld_base_step * 2))
#define cpu_read_Empty_bit      0
#define cpu_write_Full_bit      1
#define fifo_read_error_bit     2
#define fifo_write_error_bit    3
#define nds_cmd_complete_bit    4
#define data_begin_bit          5
#define nds_ecs_bit             6   
#define nds_rcs_bit             7

#define cpld_state_cpu_read_Empty      ((*(vu8*)cpld_state_addr)&BIT(cpu_read_Empty_bit))
#define cpld_state_cpu_write_Full      ((*(vu8*)cpld_state_addr)&BIT(cpu_write_Full_bit))
#define cpld_state_fifo_read_error     ((*(vu8*)cpld_state_addr)&BIT(fifo_read_error_bit))
#define cpld_state_fifo_write_error    ((*(vu8*)cpld_state_addr)&BIT(fifo_write_error_bit))
#define cpld_state_nds_cmd_complete    ((*(vu8*)cpld_state_addr)&BIT(nds_cmd_complete_bit))
#define cpld_state_data_begin         ((*(vu8*)cpld_state_addr)&BIT(data_begin_bit))
#define cpld_state_nds_ecs             ((*(vu8*)cpld_state_addr)&BIT(nds_ecs_bit))
#define cpld_state_nds_rcs             ((*(vu8*)cpld_state_addr)&BIT(nds_rcs_bit))

//fifo cmp (data,(write address - read address)) > =1 else =0  write/read
#define write_addr_cmp_addr (cpld_base_addr + (cpld_base_step * 3))
#define write_addr_cmp_step_addr (cpld_base_addr + (cpld_base_step * 4))
//eeprom data write/read
#define cpld_spi_addr       (cpld_base_addr + (cpld_base_step * 5))
//read: spi Count ,write X0 data
#define cpld_spi_Count_addr (cpld_base_addr + (cpld_base_step * 6))
#define cpld_x0_addr        (cpld_base_addr + (cpld_base_step * 6))
#define cpld_x1_addr        (cpld_x0_addr + cpld_base_step)
#define cpld_x2_addr        (cpld_x1_addr + cpld_base_step)
#define cpld_x3_addr        (cpld_x2_addr + cpld_base_step)
#define cpld_x4_addr        (cpld_x3_addr + cpld_base_step)

#define cpld_y0_addr        (cpld_x4_addr + cpld_base_step)
#define cpld_y1_addr        (cpld_y0_addr + cpld_base_step)
#define cpld_y2_addr        (cpld_y1_addr + cpld_base_step)
#define cpld_y3_addr        (cpld_y2_addr + cpld_base_step)
#define cpld_y4_addr        (cpld_y3_addr + cpld_base_step)


//#define ndsport ((char*)cpld_base_addr)
//#define isreadnds 1
//#define iswritends 0

#define nds_cmd_iqe_pinx    19
#define nds_data_iqe_pinx   18
#define spi_cs_iqe_pinx     21
#define spi_data_iqe_pinx   20

//read PIN Level 
#define		REG_GPIO_PXPINX(m,n)	( REG_GPIO_PXPIN(m) & BIT(n) )  /* PIN X Level  */

#define nds_cmd_iqe_pin  ( REG_GPIO_PXPINX(PD,nds_cmd_iqe_pinx)  )//GPD21: 0 valid
#define nds_data_iqe_pin ( REG_GPIO_PXPINX(PD,nds_data_iqe_pinx) )//GPD18
#define spi_cs_iqe_pin   ( REG_GPIO_PXPINX(PD,spi_cs_iqe_pinx)   )//GPD23
#define spi_data_iqe_pin ( REG_GPIO_PXPINX(PD,spi_data_iqe_pinx) )//GPD20

#define       NDS_CMD_IRQ		(IRQ_GPIO_0 + (32*3) + nds_cmd_iqe_pinx)//GPD21
#define       NDS_DATA_IRQ		(IRQ_GPIO_0 + (32*3) + nds_data_iqe_pinx)//GPD18

#define LEN (0x100000)
#define KEY_MASK (0x1f)




extern struct main_buf *pmain_buf;
extern int MP4_fd;
extern int MP4_buf;

#ifndef RGB15
#define RGB15(r,g,b)  (((r)|((g)<<5)|((b)<<10))|BIT(15))
#endif


extern u32 get_buf_form_bufnum(int num);
extern int get_video_up_buf(void);
extern int get_video_down_buf(void);
extern int get_audio_buf(void);
extern int get_nds_set_buf(void);
extern int update_buf(int bufnum);
extern void initInput(void);
//extern INPUT *updateInput(void);
extern int updateInput(struct key_buf *inputs);
extern INPUT * getInput(void);
extern int set_nds_var( u32 *data,int len);
extern int getBrightness(void);
extern void setBrightness(int data);
extern int getswap(void);
extern void setswap(int data);
extern int getupBacklight(void);
extern void setBacklight(int up,int down);
extern int getdownBacklight(void);
extern void setSupend(void);
extern void setshutdown(void);
extern void setVol(int l_vol,int r_vol,int mix_vol);
extern int pause_play(void);
extern int begin_pause_play(void);  

extern int test_main(int argc, char **argv);
extern void get_nds_time(struct rtc *time);

extern int getmm1_ok(void);
extern int getmm2_ok(void);

#define ioctl(a, b, c)  __do_MP4_ioctl(0, 0,(unsigned int) (b), (unsigned long) (c))

#if 0
#define FILE JZFS_FILE
#define fseek(a,b,c)    jzfs_Seek(a,b,c)
#define fread(a,b,c,d)   jzfs_Read(a,b,c,d)
#define ftell(a) jzfs_Tell(a)
#define fwrite(a,b,c,d) jzfs_Write(a,b,c,d)
#define fopen(a,b)  jzfs_Open(a,b)
#define fclose(a)   jzfs_Close(a)


#define SEEK_CUR    JZFS_SEEK_CUR 
#define SEEK_END    JZFS_SEEK_END 
#define SEEK_SET    JZFS_SEEK_SET 
#endif

#define malloc alloc
#define free   deAlloc

#endif /*__TEST_H__*/
