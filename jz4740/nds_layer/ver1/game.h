/*
 * Include file for Ingenic Semiconductor's JZ4740 CPU.
 */
#ifndef __cpu_nds_H__
#define __cpu_nds_H__

//#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <jz4740.h>
//#include <os_api.h>
#include <ucos_ii.h>
#define BIT(x) (1UL << (x))

#define ENABLE_TYPEDEF
#ifndef _DEV_VAR_H
#define _DEV_VAR_H

#define MP4_MAJOR 88
#define MP4_MINOR 88
#if 0
typedef     unsigned char           u8;
typedef     unsigned short int      u16;
typedef     unsigned int            u32;
typedef     unsigned long long int  u64;

typedef     signed char             s8;
typedef     signed short int        s16;
typedef     signed int              s32;
typedef     signed long long int    s64;

typedef     float                   f32;
typedef     double                  f64;
#endif

//#define     vl                      volatile

typedef     volatile u8                   vu8;
typedef     volatile u16                  vu16;
typedef     volatile u32                  vu32;
typedef     volatile u64                  vu64;

typedef     volatile s8                   vs8;
typedef     volatile s16                  vs16;
typedef     volatile s32                  vs32;
typedef     volatile s64                  vs64;

//typedef     volatile f32                  vf32;
//typedef     volatile f64                  vf64;

//设备结构
#if 1
struct cdev
{
    void *p;
};
struct inode
{
    void *p;
};
struct file
{
    void *p;
};
#define __user 
#define loff_t unsigned long
#define ssize_t unsigned long
#endif
struct MP4_dev 
{
	OS_EVENT *mutex;     /* mutual exclusion semaphore     */
	OS_EVENT *sem4key;
//	wait_queue_head_t wq_read;
//	wait_queue_head_t wq_write;
	struct cdev cdev;	  /* Char device structure		*/
};
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
	COMPLETE_STATE,
    ENABLE_FIXRGB_UPVIDEO,
    ENABLE_FIXRGB_DOWNVIDEO

};
#ifndef ARM9
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

typedef struct touchPosition {
	s16	x;
	s16	y;
} touchPosition;

#endif

enum {
	buf_video_up_0= 0,
	buf_video_up_1,
	buf_video_down_0,
	buf_video_down_1,
    buf_audio_0,
    buf_audio_1,
    buf_c0_set,
    buf_max_num

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

struct nds_iqe_list_st
{
int num;
u32 c0_addr;
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
///////////////////////////////////////////////////////////////
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
        LOAD_FLAG_OK = 1,
        LOAD_FLAG_SD_NO_INSERT,
        LOAD_FLAG_SD_UNKNOW,
        LOAD_FLAG_SD_NO_NDS_FILE,
        LOAD_FLAG_SD_NO_LINUX_FILE,
        LOAD_FLAG_FIND_UPGRADE,
        LOAD_FLAG_UPGRADE_WAIT,
        LOAD_FLAG_UPGRADE_ERROR_LANGUAGE,
        LOAD_FLAG_UPGRADE_ERROR_WRITE_FLASH,
        LOAD_FLAG_UPGRADE_OK

};
#ifndef ARM9
//typedef     int  bool;
#endif


typedef enum {cmd_normal = 0,key1_cmd_enable,key2_cmd_enable} NDS_CMD_STATE;
typedef enum {data_normal = 0,key2_data_enable} NDS_DATA_STATE;

typedef enum {NDS_LOAD = 0,NDSI_LOAD,LINUX_LOAD} NDS_LOAD_STATE;

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#endif

#endif /*__cpu_nds_H__ */



/********************************************************
 * Macros to help debugging
 ********************************************************/

//设备号


//函数申明
//  ssize_t MP4_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
//  ssize_t MP4_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
//  loff_t  MP4_llseek(struct file *filp, loff_t off, int whence);
  int     MP4_ioctl(struct inode *inode, struct file *filp,
                    unsigned int cmd, unsigned long arg);

extern int __do_MP4_ioctl(struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg);

/*
 * Include file for Ingenic Semiconductor's JZ4740 CPU.
 */
#ifndef GAME_DEFINE_H 
#define GAME_DEFINE_H



#ifndef BIT
#define     BIT(aa)                (1<<aa)
#endif
#define		DMA_CHANNEL				0
#define		PA_PORT						0
#define		PB_PORT						1
#define		PC_PORT						2
#define		PD_PORT						3
#define		DATA_PORT				PA_PORT
#define		INTERRUPT_PORT			PD_PORT

//#define		Uart_Printf				serial_puts
typedef     volatile unsigned short int                   fpgaport;

#define cpld_base_addr (0x0c000000 | 0xa0000000)
#define cpld_base_step (1<<13)
//fifo base address write/read
#define cpld_fifo_read_ndswcmd_addr      (cpld_base_addr)
#define cpld_fifo_write_ndsrdata_addr      (cpld_base_addr + (cpld_base_step * 8))
#define cpld_fifo_read_ndswdata_addr      (cpld_base_addr + (cpld_base_step * 9))

//fifo control reg address write/read
#define cpld_ctr_addr       (cpld_base_addr + (cpld_base_step * 1))
#define nds_dir_bit             0
#define enable_fix_video_bit    1
#define key2_cmd_enable_bit     2
#define key2_data_enable_bit    3
#define key2_init_enable_bit    4
#define fifo_clear_bit          5
#define enable_fix_video_rgb_bit    6
//#define fpga_mode_bit         7
#define fpga_mode_bit         10
#define fifo_data_cpu_wd_clk_bit         8
#define reset_flag_bit                12

//fifo state reg address read
#define cpld_state_addr     (cpld_base_addr + (cpld_base_step * 2))
#define cpu_read_Empty_bit      0
#define cpu_write_Full_bit      1
#define cpu_datafifo_read_empty 2

#define nds_read_Empty_bit      3
#define nds_write_Full_bit      4
#define nds_datafifo_write_full 5

#define fifo_read_error_bit     6
#define fifo_write_error_bit    7
#define nds_cmd_complete_bit    8
#define data_begin_bit          9

#define spi_time_disable_bit     12
#define cpu_rtc_clk_enable_bit   13
#define ht_12m_disable_bit       14
#define nds_iqe_out_bit          15

#define cpld_state_cpu_read_Empty      ((*(fpgaport*)cpld_state_addr)&BIT(cpu_read_Empty_bit))
#define cpld_state_cpu_write_Full      ((*(fpgaport*)cpld_state_addr)&BIT(cpu_write_Full_bit))
#define cpld_state_fifo_read_error     ((*(fpgaport*)cpld_state_addr)&BIT(fifo_read_error_bit))
#define cpld_state_fifo_write_error    ((*(fpgaport*)cpld_state_addr)&BIT(fifo_write_error_bit))
#define cpld_state_nds_cmd_complete    ((*(fpgaport*)cpld_state_addr)&BIT(nds_cmd_complete_bit))
#define cpld_state_data_begin         ((*(fpgaport*)cpld_state_addr)&BIT(data_begin_bit))

//fifo cmp (data,(write address - read address)) > =1 else =0  write/read
#define write_addr_cmp_addr (cpld_base_addr + (cpld_base_step * 3))
#define cpld_spi_Count_addr (cpld_base_addr + (cpld_base_step * 4))
//eeprom data write/read
#define cpld_spi_addr       (cpld_base_addr + (cpld_base_step * 5))

//read: spi Count ,write X0 data
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

#define nds_cmd_iqe_pinx    0
#define fpga_cmd_port   (PC_PORT *32)

#define nds_data_iqe_pinx   1
#define fpga_data_port   (PC_PORT *32)

#define spi_cs_iqe_pinx     3
#define fpga_spics_port   (PC_PORT *32)

#define spi_data_iqe_pinx   5
#define fpga_spidata_port   (PC_PORT *32)

#define spi_clk_iqe_pinx     31
#define fpga_spiclk_port   (PD_PORT *32)

#define fpga_cs_pinx 27
#define fpga_cs_port   (PB_PORT *32)

#define sd_cd_pinx 30
#define sd_cd_port   (PD_PORT *32)
//read PIN Level 
#define		REG_GPIO_PXPINX(m,n)	( REG_GPIO_PXPIN(m) & BIT(n) )  /* PIN X Level  */

#define nds_cmd_iqe_pin  ( REG_GPIO_PXPINX(fpga_cmd_port/32,nds_cmd_iqe_pinx)  )
#define nds_data_iqe_pin ( REG_GPIO_PXPINX(fpga_data_port/32,nds_data_iqe_pinx) )
#define spi_cs_iqe_pin   ( REG_GPIO_PXPINX(fpga_spics_port/32,spi_cs_iqe_pinx)   )
#define spi_data_iqe_pin ( REG_GPIO_PXPINX(fpga_spidata_port/32,spi_data_iqe_pinx) )
#define sd_cd_pin ( REG_GPIO_PXPINX(sd_cd_port/32,sd_cd_pinx) )

#define       NDS_CMD_IRQ		(IRQ_GPIO_0 + fpga_cmd_port + nds_cmd_iqe_pinx)//GPD21
#define       NDS_DATA_IRQ		(IRQ_GPIO_0 + fpga_data_port + nds_data_iqe_pinx)//GPD18


#define RESET_FPGA_FIFO *(fpgaport*)cpld_ctr_addr = BIT(fifo_clear_bit); \
                        *(fpgaport*)cpld_ctr_addr = 0

#define IQE_FPGA_NDS_IQE *(fpgaport*)cpld_state_addr = BIT(nds_iqe_out_bit); \
                        nds_iqe_delay=0x2000;                           \
                        while(nds_iqe_delay--);                         \
                        *(fpgaport*)cpld_state_addr = 0

#define RESET_FPGA_NDS_IQE *(fpgaport*)cpld_state_addr = 0 


#define key_port (PD_PORT * 32)
#define key_pinx 0
#define key_pin ( REG_GPIO_PXPINX(PD_PORT,key_pinx))


//{2'b0,cpu_w_len,reset_flag[3:0],{2{1'b0}},nds_iqe_out_reg,fpga_mode,enable_fix_video,enable_fix_video_rgb,cpld_state[9:0]};
#define nds_fifo_read_full_bit  cpu_write_Full_bit
#define nds_fifo_read_empty_bit cpu_read_Empty_bit
#define nds_fifo_datafifo_read_empty cpu_datafifo_read_empty
#define nds_fifo_read_error_bit  fifo_read_error_bit
#define nds_fifo_write_error_bit  fifo_write_error_bit
#define nds_fifo_len_bit 19
#define nds_fifo_len_mask 0x3fe
#define nds_fifo_cpu_data_byte 2

#define nds_fifo_cmd_read_state  0xe0
#define nds_fifo_cmd_reset  0xe1
#define nds_fifo_cmd_read  0xe8
#define nds_fifo_cmd_write  0xe9


#define __gpio_as_disenable_irq(n)		\
do {						\
	unsigned int p, o;			\
	p = (n) / 32;				\
	o = (n) % 32;				\
	REG_GPIO_PXIMS(p) = (1 << o);		\
	REG_GPIO_PXTRGC(p) = (1 << o);		\
	REG_GPIO_PXDIRC(p) = (1 << o);		\
	REG_GPIO_PXFUNC(p) = (1 << o);		\
	REG_GPIO_PXSELC(p) = (1 << o);		\
} while (0)

#if 0
#define INITFPGAPORTTIME()                              \
    do{                                               \
    REG_EMC_SMCR3 = ((3<<24)|/*strv*/(3<<20)|/*taw r */(3<<16)|/*tbp w */(3<<12) |/*tah*/(3<<8) |/*tas */ (1<<6)|/*bw 16 */ (0<<3)|/*bcm */ (0<<1)|/*bl*/(0<<0) /*smt */ ); \
    REG_EMC_SACR3 =( (0xc<<8)|/*BASE*/(0xfc<<0));/*MASK  */\
    } while (0)
#else
#define INITFPGAPORTTIME()                              \
    do{                                               \
		REG_EMC_SMCR3 = ((0<<24)|/*strv*/(2<<20)|/*taw r */(1<<16)|/*tbp w */(1<<12) |/*tah*/(1<<8) |/*tas */ (1<<6)|/*bw 16 */ (0<<3)|/*bcm */ (0<<1)|/*bl*/(0<<0) /*smt */ );\
		REG_EMC_SACR3 =( (0xc<<8)|/*BASE*/(0xfc<<0));/*MASK  */\
    } while (0)
#endif

#define LEN (0xC8000) //(0x100000)
#define KEY_MASK (0x1f)

#define os_SemaphorePend OSSemPend
#define os_TimeGet OSTimeGet
#define os_SemaphorePost    OSSemPost
#define os_SemaphoreCreate  OSSemCreate

#define nds_load_set_cmd 0xc0

#define nds_load_block_cmd 0xc2
#define nds_load_key_cmd 0xc3
#define nds_load_exit_cmd 0xcf

#define flash_base_addr (0x08000000 | 0x80000000)
#endif /*__cpu_nds_H__ */

//#endif /*__cpu_nds_H__ */

