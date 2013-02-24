#include "jz4740.h"
#include "game.h"

#define NUM_GPIO 128
#define IRQ_MAX	(IRQ_GPIO_0 + NUM_GPIO)

extern int MP4_fd;

typedef struct run_plugin_head_t {
u32 cpu_code_offset;
u32 cpu_code_len;
u32 cpu_load_addr;
u32 cpu_jmp_addr;

u32 nds_code_offset;
u32 nds_code_len;

} Run_Plugin_Head;

typedef struct run_plugin_argc {
u32 load_argc;
u32 load_argv[5];

u32 cpu_argc;
u32 cpu_argv[5];
u32 check1;
u32 check2;
u32 check3;
u32 check4;
u32 tt[(128/4) - 16];

char file_path[256];
char cpu_argv0[256];
char cpu_argv1[256];
char cpu_argv2[256];
} Run_Plugin_Argc;

void disable_irq_all(void)
{
    int i;
    for (i=0;i< IRQ_MAX;i++ )
    {
        disable_irq(i);
    }
    spin_lock_irqsave();
}

#define nds_ioctl(a, b, c)  MP4_ioctl(0, 0,(unsigned int) (b), (unsigned long) (c))

int disenable_nds(void)
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
        nds_ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)
        {
            break;
        }
    }
    memset(tempbuf12,0,512);
    temp32p=tempbuf12;

    temp32p[0]=0xc0;

    offset = 0x60;
    offsetindex=0x4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_VIDEO;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=0;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_AUDIO;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=0;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_KEY;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=0;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_ENABLE_RTC;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=0;
    offset+=4;

    temp32p[offsetindex/4]=IS_SET_CLEAR_VAR;offsetindex+=4;
    temp32p[offsetindex/4]=offset;offsetindex+=4;
    temp32p[offset/4]=1;
    offset+=4;


    set_nds_var(tempbuf12,512);
    flag1=0;
    while (flag1 == 0)
    {
        nds_ioctl(MP4_fd, COMPLETE_STATE, &flag);
        if ((flag &2) == 2)
        {
            break;
        }
    }

    return 0;
}

int _do_reset(void)
{
printf( "##Reseting ...\n\n" );
	__wdt_select_extalclk();
	__wdt_select_clk_div64();
	__wdt_set_data(100);
	__wdt_set_count(0);
	__tcu_start_wdt_clock();
	__wdt_start();
    REG_EMC_DMCR |= EMC_DMCR_RMODE | EMC_DMCR_RFSH;

printf("To exit\n");
	//while(1);
	printf("*** reset failed ***\n");

	return 0;
}
#define fpga_mode_bit         10

#define argc_addr (0x80000000 + (32*1024*1024) -0x1000)
#define reset_power ((0<<fpga_mode_bit) | (0<< 11) | (0<<12))
#define reset_main ((1<<fpga_mode_bit) | (0<< 11) | (0<<12))
#define reset_test ((0<<fpga_mode_bit) | (1<< 11) | (0<<12))

#define reset_main_nonds ((1<<fpga_mode_bit) | (1<< 11) | (0<<12))
#define reset_plugin ((0<<fpga_mode_bit) | (0<< 11) | (1<<12))
#define cpu_rtc_clk_enable_bit   13

void run_plugin(void)
{
//printf("In run_plugin\n");
    disenable_nds();
//printf("In run_plugin 0\n");
    disable_irq_all();

    __dcache_writeback_all();
    __icache_invalidate_all();  

    *(volatile unsigned short int*)cpld_ctr_addr = (1<<fifo_clear_bit);

    *(fpgaport*)cpld_ctr_addr = reset_main_nonds;
    *(fpgaport*)cpld_state_addr = BIT(cpu_rtc_clk_enable_bit);
//printf("In run_plugin 1\n");
   _do_reset();
}

char* get_gba_file(void)
{
    Run_Plugin_Argc *Run_Plugin_Argcp;
    Run_Plugin_Argcp = (Run_Plugin_Argc *)argc_addr;

    if ((Run_Plugin_Argcp->check1 != 0x5aa55aa5)|| (Run_Plugin_Argcp->check2 != 0xa55aa55a))
    {
        return 0;
    }
    char **cpu_arv;
    cpu_arv =(char **) Run_Plugin_Argcp->load_argv[2];
    return (char*) cpu_arv[0];
}
