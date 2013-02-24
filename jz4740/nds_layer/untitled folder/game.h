/*
 * Include file for Ingenic Semiconductor's JZ4740 CPU.
 */
#ifndef __cpu_nds_H__
#define __cpu_nds_H__

#include "common.h"
//#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <jz4740.h>
//#include <os_api.h>
#define BIT(x) (1UL << (x))

/********************************************************
 * Macros to help debugging
 ********************************************************/

//设备号
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

        IS_SET_VAL,
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



//函数申明
//  ssize_t MP4_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
//  ssize_t MP4_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
//  loff_t  MP4_llseek(struct file *filp, loff_t off, int whence);
  int     MP4_ioctl(struct inode *inode, struct file *filp,
                    unsigned int cmd, unsigned long arg);


//#define     BIT(aa)                (1<<aa)
#define		DMA_CHANNEL				0

#define		PA_PORT						0
#define		PB_PORT						1
#define		PC_PORT						2
#define		PD_PORT						3

//#define		DATA_PORT				PA_PORT
//#define		INTERRUPT_PORT			PD_PORT

//#define		Uart_Printf				serial_puts

#define cpld_base_addr (0x08000000 | 0xa0000000)
#define cpld_base_step (1<<13)
//fifo base address write/read
#define cpld_fifo_addr      (cpld_base_addr)

//fifo control reg address write/read
#define cpld_ctr_addr       (cpld_base_addr + (cpld_base_step * 1))
#define nds_dir_bit             0
#define enable_fix_video_bit    1
#define key2_cmd_enable_bit     2
#define key2_data_enable_bit    3
#define key2_init_enable_bit    4
#define fifo_clear_bit          5
#define nds_cmd_iqe_type_bit    6
//#define waitdisable_bit         7

#define fpga_mode_bit         7

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

#define nds_iqe_out_bit         6

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
#define fpga_cmd_port   (PD_PORT *32)

#define nds_data_iqe_pinx   18
#define fpga_data_port   (PD_PORT *32)

#define spi_cs_iqe_pinx     21
#define fpga_spics_port   (PD_PORT *32)

#define spi_data_iqe_pinx   20
#define fpga_spidata_port   (PD_PORT *32)

#define spi_clk_iqe_pinx     31
#define fpga_spiclk_port   (PD_PORT *32)

#define fpga_cs_pinx 28
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

#define LEN (0xC8000) //(0x100000)
//#define LEN (64) //(0x100000)
#define KEY_MASK (0x1f)


#define os_SemaphorePend OSSemPend
#define os_TimeGet OSTimeGet
#define os_SemaphorePost    OSSemPost
#define os_SemaphoreCreate  OSSemCreate


#endif /*__cpu_nds_H__ */
