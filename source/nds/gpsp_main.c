/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
 * Copyright (C) 2007 NJ
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
 *s
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/******************************************************************************
 * 头文件
 ******************************************************************************/
#include "common.h"

//PSP_MODULE_INFO("gpSP", PSP_MODULE_USER, VERSION_MAJOR, VERSION_MINOR);
//PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
//PSP_MAIN_THREAD_PRIORITY(0x11);
//PSP_MAIN_THREAD_STACK_SIZE_KB(640);
//PSP_HEAP_SIZE_MAX();

/******************************************************************************
 * 全局变量定义
 ******************************************************************************/
TIMER_TYPE timer[4];                              // 定时器

/******************************************************************************
 * 全局变量定义
 ******************************************************************************/

u32 global_cycles_per_instruction = 1;
//u64 frame_count_initial_timestamp = 0;
//u64 last_frame_interval_timestamp;
u32 psp_fps_debug = 0;
u32 skip_next_frame_flag = 0;
u32 frameskip_0_hack_flag = 0;
//u32 frameskip_counter = 0;

u32 cpu_ticks = 0;
u32 frame_ticks = 0;

u32 execute_cycles = 960;
s32 video_count = 960;
//u32 ticks;

//u32 arm_frame = 0;
//u32 thumb_frame = 0;
u32 last_frame = 0;

u32 synchronize_flag = 1;

//const char main_path[]="mmc:\\NDSGBA";
char main_path[MAX_PATH];

//Removing rom_path due to confusion
//char rom_path[MAX_PATH];

//vu32 quit_flag;
vu32 power_flag;

vu32 real_frame_count = 0;
u32 virtual_frame_count = 0;
vu32 vblank_count = 0;
u32 num_skipped_frames = 0;
u32 frames;

unsigned int pen = 0;
unsigned int frame_interval = 60; // For in-memory saved states used in rewinding

int date_format= 2;

u32 prescale_table[] = { 0, 6, 8, 10 };

char *file_ext[] = { ".gba", ".bin", ".zip", NULL };

// PSP类型
//MODEL_TYPE psp_model;
const MODEL_TYPE psp_model= PSP_1000;

/******************************************************************************
 * 宏定义
 ******************************************************************************/

// 决定仿真周期
#define CHECK_COUNT(count_var)                                                \
  if((count_var) < execute_cycles)                                            \
    execute_cycles = count_var;                                               \

#define CHECK_TIMER(timer_number)                                             \
  if(timer[timer_number].status == TIMER_PRESCALE)                            \
    CHECK_COUNT(timer[timer_number].count);                                   \

// 更新定时器
//实际范围是0~0xFFFF，但是gpSP内部是(0x10000~1)<<prescale(0,6,8,10)的权值
#define update_timer(timer_number)                                            \
  if(timer[timer_number].status != TIMER_INACTIVE)                            \
  {                                                                           \
    /* 如果是使用中的定时器 */												  \
    if(timer[timer_number].status != TIMER_CASCADE)                           \
    {                                                                         \
      /* 如果定时器没有级联 */                                                \
      /* 改变定时器的计数值 */                                                \
      timer[timer_number].count -= execute_cycles;                            \
      /* 保存计数值 */                                                        \
      io_registers[REG_TM##timer_number##D] =                                 \
      0x10000 - (timer[timer_number].count >> timer[timer_number].prescale);  \
    }                                                                         \
                                                                              \
    if(timer[timer_number].count <= 0)                                        \
    {                                                                         \
      /* 如果定时器溢出 */                                                    \
      /* 是否触发中断 */                                                      \
      if(timer[timer_number].irq == TIMER_TRIGGER_IRQ)                        \
        irq_raised |= IRQ_TIMER##timer_number;                                \
                                                                              \
      if((timer_number != 3) &&                                               \
       (timer[timer_number + 1].status == TIMER_CASCADE))                     \
      {                                                                       \
        /* 定时器0～2 如果在定时器级联模式下 */                               \
        /* 改变定时器的计数值 */                                              \
        timer[timer_number + 1].count--;                                      \
        /* 保存计数值 */                                                      \
        io_registers[REG_TM0D + (timer_number + 1) * 2] =                     \
        0x10000 - (timer[timer_number + 1].count);                            \
      }                                                                       \
                                                                              \
      if(timer_number < 2)                                                    \
      {                                                                       \
        if(timer[timer_number].direct_sound_channels & 0x01)                  \
          sound_timer(timer[timer_number].frequency_step, 0);                 \
                                                                              \
        if(timer[timer_number].direct_sound_channels & 0x02)                  \
          sound_timer(timer[timer_number].frequency_step, 1);                 \
      }                                                                       \
                                                                              \
      /* 加载定时器 */                                                        \
      timer[timer_number].count +=                                            \
        (timer[timer_number].reload << timer[timer_number].prescale);         \
      io_registers[REG_TM##timer_number##D] =                                 \
      0x10000 - (timer[timer_number].count >> timer[timer_number].prescale);  \
    }                                                                         \
  }                                                                           \

// 局部函数声明
void vblank_interrupt_handler(u32 sub, u32 *parg);
void init_main();
int main(int argc, char *argv[]);
void print_memory_stats(u32 *counter, u32 *region_stats, u8 *stats_str);
u32 into_suspend();

//int exit_callback(int arg1, int arg2, void *common);
//SceKernelCallbackFunction power_callback(int unknown, int powerInfo, void *common);
//int CallbackThread(SceSize args, void *argp);
//int SetupCallbacks();
//int user_main(SceSize args, char *argp[]);
//void psp_exception_handler(PspDebugRegBlock *regs);

// 改变CPU时钟
//void set_cpu_clock(u32 num)
//{
//  const u32 clock_speed_table[4] = {120, 240, 360, 396};
//  const u32 sdram_speed_table[4] = {1, 2, 3, 3};
//  num = num % 4;
//  scePowerSetClockFrequency(clock_speed_table[num], clock_speed_table[num], clock_speed_table[num] / 2);
//  jz_pm_pllconvert(clock_speed_table[num]*1000000, sdram_speed_table[num]);	//pllin= clock, sdram_clk= pllin/div
//}

static u8 caches_inited = 0;

void init_main()
{
  u32 i;

  skip_next_frame_flag = 0;
  frameskip_0_hack_flag = 0;

  for(i = 0; i < 4; i++)
  {
    dma[i].start_type = DMA_INACTIVE;
    dma[i].direct_sound_channel = DMA_NO_DIRECT_SOUND;
    timer[i].status = TIMER_INACTIVE;
    timer[i].reload = 0x10000;
  }

  timer[0].direct_sound_channels = TIMER_DS_CHANNEL_BOTH;
  timer[1].direct_sound_channels = TIMER_DS_CHANNEL_NONE;

  cpu_ticks = 0;
  frame_ticks = 0;

  execute_cycles = 960;
  video_count = 960;

  // bios_mode = USE_BIOS;

  if (!caches_inited)
  {
    flush_translation_cache(TRANSLATION_REGION_READONLY, FLUSH_REASON_INITIALIZING);
    flush_translation_cache(TRANSLATION_REGION_WRITABLE, FLUSH_REASON_INITIALIZING);
  }
  else
  {
    flush_translation_cache(TRANSLATION_REGION_READONLY, FLUSH_REASON_LOADING_ROM);
    clear_metadata_area(METADATA_AREA_EWRAM, CLEAR_REASON_LOADING_ROM);
    clear_metadata_area(METADATA_AREA_IWRAM, CLEAR_REASON_LOADING_ROM);
    clear_metadata_area(METADATA_AREA_VRAM, CLEAR_REASON_LOADING_ROM);
  }

  caches_inited = 1;

  StatsInitGame();
}

void quit(void)
{
/*
  u32 reg_ra;

  __asm__ __volatile__("or %0, $0, $ra"
                        : "=r" (reg_ra)
                        :);

  dbg_printf("return address= %08x\n", reg_ra);
*/

#ifdef USE_DEBUG
	fclose(g_dbg_file);
#endif

	ds2_plug_exit();
	while(1);
}

/*
void sceKernelRegisterSubIntrHandler(void* func, void* para)
{

}
*/

int plug_valid= 0;

int gpsp_main(int argc, char *argv[])
{
  char load_filename[MAX_FILE];

/*
unsigned short ch[2500];
FILE *fp;
u32 i;

    memset((unsigned char*)ch, 0, 5000);

    fp= fopen("mmc:\\newfile.txt", "r");
    if(fp == NULL)
        printf("Open newfile.txt failure\n");
    else
    {
        i= fread((unsigned char*)ch, 2, 2500, fp);
        printf("0 read %d\n", i);

//        i= fread((unsigned char*)&ch[50], 2, 1500, fp);
//        printf("1 read %d\n", i);

//        i= fread((unsigned char*)&ch[1550], 2, 650, fp);
//        printf("2 read %d\n", i);

        fclose(fp);

        for(i= 0; i < 2500; i++)
        {
            if((i % 256) == 0)
            {
                printf("\n\n\nsector%d\n\n", i/256);
            }

            if((i % 8) == 0)
                printf("\n%d : ", i/8);
            printf("%d ", ch[i]);
            if(ch[i] != i) printf("Read error %d\n", i);
        }
    }

while(1);
*/

  // Copy the directory path of the executable into main_path
//  getcwd(main_path, MAX_FILE);
//  chdir(main_path);

//    OSTimeDly(OS_TICKS_PER_SEC*3);

//  init_progress(7, "");

    if(gui_init(0) < 0)
        quit();
  // Initial path information
  initial_gpsp_config();
  // 初始化进度条
//  update_progress();

    init_video();

 // 读取gpSP配置文件
    //config is loaded in gui_init
  //load_config_file();

//  psp_model = get_model();

//  quit_flag = 0;				//commented already
  power_flag = 0;
//  SetupCallbacks();	<----电池电量不足等紧急事件处理进程，以后再说吧!

//  sceKernelRegisterSubIntrHandler(PSP_VBLANK_INT, 0, vblank_interrupt_handler, NULL);
//  sceKernelEnableSubIntr(PSP_VBLANK_INT, 0);
//  sceKernelRegisterSubIntrHandler(vblank_interrupt_handler, NULL);

#ifdef USE_DEBUG
  // 打开调试输出文件
  g_dbg_file = fopen(DBG_FILE_NAME, "awb");
  DBGOUT("\nStart gpSP\n");
#endif

  // 初始化
  init_game_config();

  init_main();
  init_sound();
  init_input();

//  video_resolution(FRAME_MENU);
//  video_resolution(FRAME_GAME);
//  video_resolution(FRAME_MENU);

/*
  clear_screen(COLOR_BG);
  flip_screen();
  OSTimeDly(OS_TICKS_PER_SEC/10);
  clear_screen(COLOR_BG);
  flip_screen();
*/

  // 分配仿真ROM
  init_gamepak_buffer();
//  update_progress();

  // ROM文件名初始化
  gamepak_filename[0] = 0;

  // BIOS的读入
  char bios_filename[MAX_FILE];
  sprintf(bios_filename, "%s/%s", main_path, "gba_bios.bin");
  u32 bios_ret = load_bios(bios_filename);
  if(bios_ret == -1) // 当读取失败
  {
    err_msg(DOWN_SCREEN, "The GBA BIOS is not present\nPlease see README.md for\nmore information\n\nLe BIOS GBA n'est pas present\nLisez README.md pour plus\nd'information (en anglais)");
    ds2_flipScreen(DOWN_SCREEN, DOWN_SCREEN_UPDATE_METHOD);
    wait_Anykey_press(0);
    quit();
  }
//  update_progress();
#if 0
  if(bios_ret == -2) // MD5校验不符
  {
    error_msg("MD5 check not match\n");
  }
#endif
//  update_progress();

//  show_progress(msg[MSG_INIT_END]);
    //OSTimeDly(OS_TICKS_PER_SEC*2);

	// Huh? What's all of this for? [Neb]
#if 0
dgprintf("Global cmd: %s\n", get_gba_file());
    char *ppt;
    ppt= get_gba_file();
    if(ppt != NULL)
    {
        strcpy(load_filename, ppt);
        ppt= strrchr(load_filename, '\\');
        if(ppt != NULL)
        {
            strcpy(gamepak_filename, ppt+1);
            *ppt = '\0';
            strcpy(rom_path, load_filename);
            plug_valid= 1;
        }
    }
#endif

#if 0
  if(argc > 1)
  {
    if(load_gamepak((char *)argv[1]) == -1)
    {
      quit();
    }

    clear_screen(0);
    flip_screen();
    clear_screen(0);
    flip_screen();
//    video_resolution(FRAME_GAME);

    init_cpu();
    init_memory();
    reset_sound();
  }
  else
  {
    if(load_file(file_ext, load_filename, g_default_rom_dir) == -1)
    {
      u16 screen_copy[GBA_SCREEN_BUFF_SIZE];
      copy_screen(screen_copy);
      menu(screen_copy);
    }
    else
    {
      if(load_gamepak(load_filename) == -1)
      {
        video_resolution(FRAME_MENU);
        quit();
      }

      clear_screen(0);
      flip_screen();
      clear_screen(0);
      flip_screen();
//      video_resolution(FRAME_GAME);

      init_cpu();
      init_memory();
      reset_sound();
    }
  }
#endif

	//strcpy(rom_path, "fat:");
	//if(load_gamepak("test.gba") == -1)
  //  {
	//	error_msg("Test.gba could not be loaded.\n");
  //      quit();
  //  }

    init_cpu();
    init_memory();
    reset_sound();

    u16 screen_copy[GBA_SCREEN_BUFF_SIZE];
    memset((char*)screen_copy, 0, sizeof(screen_copy));
    menu(screen_copy, 1 /* first invocation: yes */);

  last_frame = 0;

  pause_sound(0);
  real_frame_count = 0;
  virtual_frame_count = 0;

//  set_cpu_clock(2);
  //
#ifdef USE_C_CORE
  if(gpsp_config.emulate_core == ASM_CORE)
  {
    execute_arm_translate(execute_cycles);
  }
  else
  {
    execute_arm(execute_cycles);
  }
#else
  execute_arm_translate(execute_cycles);
#endif

  return 0;
}

// 暂停处理进入菜单
u32 into_suspend()
{
//  if (power_flag == 0) return 0; // TODO この処理はupdate_gba()側に移動
  FILE_CLOSE(gamepak_file_large);
  u16 screen_copy[GBA_SCREEN_BUFF_SIZE];
  copy_screen(screen_copy);
  u32 ret_val = menu(screen_copy, 0 /* first invocation: no */);
  FILE_OPEN(gamepak_file_large, gamepak_filename_full_path, READ);
  return ret_val;
}

u32 sync_flag = 0;

#define V_BLANK   (0x01)
#define H_BLANK   (0x02)
#define V_COUNTER (0x04)

// video_count的初始值是960(行显示时钟周期)
//#define SKIP_RATE 2
//u32 SKIP_RATE= 2;
u32 to_skip= 0;

u32 update_gba()
{
  // TODO このあたりのログをとる必要があるかも
  IRQ_TYPE irq_raised = IRQ_NONE;

  do
    {
      cpu_ticks += execute_cycles;

      reg[CHANGED_PC_STATUS] = 0;

      if(gbc_sound_update)
        {
//printf("update sound\n");
          update_gbc_sound(cpu_ticks);
          gbc_sound_update = 0;
        }

    update_timer(0);
    update_timer(1);
    update_timer(2);
    update_timer(3);

    video_count -= execute_cycles;

    if(video_count <= 0)
    { // 状態移行の発生
      u32 vcount = io_registers[REG_VCOUNT];
      u32 dispstat = io_registers[REG_DISPSTAT];

      if(!(dispstat & H_BLANK))
      { // 非 H BLANK
        // 转 H BLANK
        video_count += 272;
        dispstat |= H_BLANK; // 设置 H BLANK

        if((dispstat & 0x01) == 0)
        { // 非 V BLANK
          // 无跳帧时
          if(!skip_next_frame_flag)
         {
              update_scanline();
          }

          // If in visible area also fire HDMA
          if(dma[0].start_type == DMA_START_HBLANK)
            dma_transfer(dma);
          if(dma[1].start_type == DMA_START_HBLANK)
            dma_transfer(dma + 1);
          if(dma[2].start_type == DMA_START_HBLANK)
            dma_transfer(dma + 2);
          if(dma[3].start_type == DMA_START_HBLANK)
            dma_transfer(dma + 3);

          if(dispstat & 0x10)
            irq_raised |= IRQ_HBLANK; // HBLANK 中断不会发生在 VBLANK 中
        }

      } // 非 HBLANK
      else
      { // HBLANK
        // 场扫描增加一行
        video_count += 960;
        dispstat &= ~H_BLANK;

        vcount++;

        if(vcount == 160)
        {
          dispstat |= 0x01;
          if(dispstat & 0x8)
            irq_raised |= IRQ_VBLANK;

          affine_reference_x[0] = (s32)(ADDRESS32(io_registers, 0x28) << 4) >> 4;
          affine_reference_y[0] = (s32)(ADDRESS32(io_registers, 0x2C) << 4) >> 4;
          affine_reference_x[1] = (s32)(ADDRESS32(io_registers, 0x38) << 4) >> 4;
          affine_reference_y[1] = (s32)(ADDRESS32(io_registers, 0x3C) << 4) >> 4;

          if(dma[0].start_type == DMA_START_VBLANK)
            dma_transfer(dma);
          if(dma[1].start_type == DMA_START_VBLANK)
            dma_transfer(dma + 1);
          if(dma[2].start_type == DMA_START_VBLANK)
            dma_transfer(dma + 2);
          if(dma[3].start_type == DMA_START_VBLANK)
            dma_transfer(dma + 3);

       }
       else if(vcount == 228)
	   {
          // Transition from vblank to next screen
          dispstat &= ~0x01;
          frame_ticks++;
			if(frame_ticks >= frame_interval)
				frame_ticks = 0;

//          sceKernelDelayThread(10);

          if(update_input() != 0)
            continue;

			if(game_config.backward)
			{
				if(fast_backward) // Rewinding requested
				{
					fast_backward = 0;
					if(savefast_queue_len > 0)
					{
						if(frame_ticks > 3)
						{
							if(pen)
								mdelay(500);

							loadstate_fast();
							pen = 1;
							frame_ticks = 0;
							continue;
						}
					}
					else if(frame_ticks > 3)
					{
						u32 HotkeyRewind = game_persistent_config.HotkeyRewind != 0 ? game_persistent_config.HotkeyRewind : gpsp_persistent_config.HotkeyRewind;

						while(readkey() & HotkeyRewind);
					}
				}
				else if(frame_ticks ==0)
				{
					savestate_fast();
				}
			}
#if 0
          if((power_flag == 1) && (into_suspend() != 0))
            continue;
#endif

          if(game_config.update_backup_flag == 1)
            update_backup();

          process_cheats();

          update_gbc_sound(cpu_ticks);

          vcount = 0; // TODO vcountを0にするタイミングを検討

          //TODO 調整必要
//          if((video_out_mode == 0) || (gpsp_config.screen_interlace == PROGRESSIVE))
//            synchronize();
//          else
//          {
//            if(sync_flag == 0)
//              synchronize();
//            sync_flag ^= 1;
//         }

//          if(!skip_next_frame_flag)
//            flip_gba_screen();
            Stats.EmulatedFrames++;
            Stats.TotalEmulatedFrames++;
            if(!skip_next_frame_flag)
            {
                Stats.RenderedFrames++;
                StatsDisplayFPS();
                flip_gba_screen();
                to_skip= 0;
            }
            else
              to_skip ++;

		skip_next_frame_flag = to_skip < SKIP_RATE;

//printf("SKIP_RATE %d %d\n", SKIP_RATE, to_skip);
        } //(vcount == 228)

        // vcountによる割込
        if(vcount == (dispstat >> 8))
        {
          // vcount trigger
          dispstat |= 0x04;
          if(dispstat & 0x20)
          {
            irq_raised |= IRQ_VCOUNT;
          }
        }
        else
        {
          dispstat &= ~0x04;
        }

        io_registers[REG_VCOUNT] = vcount;
      }
      io_registers[REG_DISPSTAT] = dispstat;
    }

    execute_cycles = video_count;

    if(irq_raised)
      raise_interrupt(irq_raised);

    CHECK_TIMER(0);
    CHECK_TIMER(1);
    CHECK_TIMER(2);
    CHECK_TIMER(3);

//        synchronize_sound();
    // 画面のシンクロ・フリップ・ウェイト処理はここで行うべきでは？

  } while(reg[CPU_HALT_STATE] != CPU_ACTIVE);
  return execute_cycles;
}

void vblank_interrupt_handler(u32 sub, u32 *parg)
{
  real_frame_count++;
  vblank_count++;
}

// TODO:最適化/タイマー使ったものに変更
// GBAの描画サイクルは 16.743 ms, 280896 cycles(16.78MHz) 59.737 Hz
#if 0
void synchronize()
{
//  char char_buffer[64];
  static u32 fps = 60;
  static u32 frames_drawn = 0;
  static u32 frames_drawn_count = 0;

  // FPS等の表示
  if(psp_fps_debug)
  {
    char print_buffer[256];
    sprintf(print_buffer, "FPS:%02d DRAW:(%02d) S_BUF:%02d PC:%08X", (int)fps, (int)frames_drawn, (int)left_buffer, (int)reg[REG_PC]);
    PRINT_STRING_BG(print_buffer, 0xFFFF, 0x000, 0, 0);
  }

  // 初始化跳帧标志
  skip_next_frame_flag = 0;
  // 保持内部帧增加
  frames++;

  switch(game_config.frameskip_type)
  {
  // 自动跳帧时
    case auto_frameskip:
      virtual_frame_count++;

      // 内部帧有延迟
      if(real_frame_count > virtual_frame_count)
      {
        if(num_skipped_frames < game_config.frameskip_value)  // 小于设定值
        {
          // 跳到下一帧
          skip_next_frame_flag = 1;
          // 增加跳帧
          num_skipped_frames++;
        }
        else
        {
          // 达到设定最大值
//          real_frame_count = virtual_frame_count;
          // 跳帧设置为0
          num_skipped_frames = 0;
          frames_drawn_count++;
        }
      }

      // 内部帧与实际相同
      if(real_frame_count == virtual_frame_count)
      {
        // 跳帧设置为0
        num_skipped_frames = 0;
        frames_drawn_count++;
      }

      // 内部帧率大于实际帧率
      if(real_frame_count < virtual_frame_count)
      {
        num_skipped_frames = 0;
        frames_drawn_count++;
      }

      // 内部帧率大于实际帧率时
      if((real_frame_count < virtual_frame_count) && (synchronize_flag) && (skip_next_frame_flag == 0))
      {
        // 等待 VBANK
        synchronize_sound();
        sceDisplayWaitVblankStart();
        real_frame_count = 0;
        virtual_frame_count = 0;
      }
      break;

    // 手动设置跳帧时
    case manual_frameskip:
      virtual_frame_count++;
      // 增加跳帧数
      num_skipped_frames = (num_skipped_frames + 1) % (game_config.frameskip_value + 1);
      if(game_config.random_skip)
      {
        if(num_skipped_frames != (rand() % (game_config.frameskip_value + 1)))
          skip_next_frame_flag = 1;
        else
          frames_drawn_count++;
      }
      else
      {
        // フレームスキップ数=0の時だけ画面更新
        if(num_skipped_frames != 0)
          skip_next_frame_flag = 1;
        else
          frames_drawn_count++;
      }

      // 内部帧率大于实际帧率时
      if((real_frame_count < virtual_frame_count) && (synchronize_flag) && (skip_next_frame_flag == 0))
      {
        // 等待 VBANK
        synchronize_sound();
        sceDisplayWaitVblankStart();
      }
      real_frame_count = 0;
      virtual_frame_count = 0;
      break;

    // 没有跳帧
    case no_frameskip:
      frames_drawn_count++;
      virtual_frame_count++;
      if((real_frame_count < virtual_frame_count) && (synchronize_flag))
      {
        // 内部フレーム数が実機を上回る場合
        // VBANK待ち
        synchronize_sound();
        sceDisplayWaitVblankStart();
      }
      real_frame_count = 0;
      virtual_frame_count = 0;
      break;
  }

  // FPSのカウント
  // 1/60秒のVBLANK割込みがあるので、タイマは使用しないようにした
  if(frames == 60)
  {
    frames = 0;
    fps = 3600 / vblank_count;
    vblank_count = 0;
    frames_drawn = frames_drawn_count;
    frames_drawn_count = 0;
  }

  if(!synchronize_flag)
    PRINT_STRING_BG("--FF--", 0xFFFF, 0x000, 0, 10);
}
#endif

void reset_gba()
{
  init_main();
  init_memory();
  init_cpu();
  reset_sound();
}

#if 0
u32 file_length(const char *filename)
{
  SceIoStat stats;
  sceIoGetstat(filename, &stats);
  return stats.st_size;
}
#endif

u32 file_length(FILE_ID file)
{
  u32 pos, size;
  pos= ftell(file);
  fseek(file, 0, SEEK_END);
  size= ftell(file);
  fseek(file, pos, SEEK_SET);

  return size;
}


void change_ext(char *src, char *buffer, char *extension)
{
  char *dot_position;
  strcpy(buffer, src);
  dot_position = strrchr(buffer, '.');

  if(dot_position)
    strcpy(dot_position, extension);
}

// type = READ / WRITE_MEM
#define MAIN_SAVESTATE_BODY(type)                                             \
{                                                                             \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, cpu_ticks);                      \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, execute_cycles);                 \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, video_count);                    \
  FILE_##type##_ARRAY(g_state_buffer_ptr, timer);                             \
}                                                                             \

void main_read_mem_savestate()
MAIN_SAVESTATE_BODY(READ_MEM);

void main_write_mem_savestate()
MAIN_SAVESTATE_BODY(WRITE_MEM);

void error_msg(char *text)
{
    gui_action_type gui_action = CURSOR_NONE;

    printf(text);

    while(gui_action == CURSOR_NONE)
    {
      gui_action = get_gui_input();
//      sceKernelDelayThread(15000); /* 0.0015s */
    }
}

void set_cpu_mode(CPU_MODE_TYPE new_mode)
{
  u32 i;
  CPU_MODE_TYPE cpu_mode = reg[CPU_MODE];

  if(cpu_mode != new_mode)
  {
    if(new_mode == MODE_FIQ)
    {
      for(i = 8; i < 15; i++)
      {
        reg_mode[cpu_mode][i - 8] = reg[i];
      }
    }
    else
    {
      reg_mode[cpu_mode][5] = reg[REG_SP];
      reg_mode[cpu_mode][6] = reg[REG_LR];
    }

    if(cpu_mode == MODE_FIQ)
    {
      for(i = 8; i < 15; i++)
      {
        reg[i] = reg_mode[new_mode][i - 8];
      }
    }
    else
    {
      reg[REG_SP] = reg_mode[new_mode][5];
      reg[REG_LR] = reg_mode[new_mode][6];
    }

    reg[CPU_MODE] = new_mode;
  }
}

void raise_interrupt(IRQ_TYPE irq_raised)
{
  // The specific IRQ must be enabled in IE, master IRQ enable must be on,
  // and it must be on in the flags.
  io_registers[REG_IF] |= irq_raised;

  if((io_registers[REG_IME] & 0x01) && (io_registers[REG_IE] & io_registers[REG_IF]) && ((reg[REG_CPSR] & 0x80) == 0))
  {
    bios_read_protect = 0xe55ec002;

    // Interrupt handler in BIOS
    reg_mode[MODE_IRQ][6] = reg[REG_PC] + 4;
    spsr[MODE_IRQ] = reg[REG_CPSR];
    reg[REG_CPSR] = 0xD2;
    reg[REG_PC] = 0x00000018;
    bios_region_read_allow();
    set_cpu_mode(MODE_IRQ);
    reg[CPU_HALT_STATE] = CPU_ACTIVE;
    reg[CHANGED_PC_STATUS] = 1;
  }
}

#if 0
MODEL_TYPE get_model()
{
  if((kuKernelGetModel() <= 0 /* original PSP */) || ( gpsp_config.fake_fat == YES))
  {
    return PSP_1000;
  }
  else
    if(sceKernelDevkitVersion() < 0x03070110 || sctrlSEGetVersion() < 0x00001012)
    {
      return PSP_1000;
    }
    else
    {
      return PSP_2000;
    }
}
#endif

char* FS_FGets(char *buffer, int num, FILE *stream)
{
	int m;
	char *s;

//    printf("In fgets\n");

	if(num <= 0)
		return (NULL);

	num--;
	m= fread(buffer, 1, num, stream);
	*(buffer +m) = '\0';

//    printf("fread= %s\n", buffer);

    if(m == 0)
      return (NULL);

	s = strchr(buffer, '\n');

	if(m < num)						//at the end of file
	{
		if(s == NULL)
			return (buffer);

		*(++s)= '\0';				//string include '\n'
		m -= s - buffer;
		fseek(stream, -m, SEEK_CUR);//fix fread pointer
		return (buffer);
	}
	else
	{
		if(s)
		{
			*(++s)= '\0';				//string include '\n'
			m -= s - buffer;
			fseek(stream, -m, SEEK_CUR);//fix fread pointer
		}

		return (buffer);
	}
}

#if 0
void syscall_fun(int num, u32 *ptr, u32* sp)
{
    if(num == -10)
    {
        printf("pc= %08x, at= %08x\n", sp[9], ptr);
    }
}
#endif

	// Duplicates syscall_fun from the DS2 SDK [Neb]
#if 0
void syscall_fun(int num, u32 *ptr, u32* sp)
{
    u32 a, b;

    printf("SYSCALL %d  %08x\n", num, ptr);
    if(num <0)
    {
        printf("AT= %08x  ra= %08x  fp= %08x  gp= %08x\n",sp[27],sp[0],sp[1],sp[2]);
        printf("t9= %08x  t8= %08x  s7= %08x  S6= %08x\n",sp[3],sp[4],sp[5],sp[6]);
        printf("s5= %08x  s4= %08x  s3= %08x  s2= %08x\n",sp[7],sp[8],sp[9],sp[10]);
        printf("s1= %08x  s0= %08x  t7= %08x  t6= %08x\n",sp[11],sp[12],sp[13],sp[14]);
        printf("t5= %08x  t4= %08x  t3= %08x  t2= %08x\n",sp[15],sp[16],sp[17],sp[18]);
        printf("t1= %08x  t0= %08x  a3= %08x  a2= %08x\n",sp[19],sp[20],sp[21],sp[22]);
        printf("a1= %08x  a0= %08x  v1= %08x  v0= %08x\n",sp[23],sp[24],sp[25], sp[26]);
    }
    switch(num)
    {
      case 0:
        printf("PC= %08x\n", ptr);
        break;
      case 2:
        printf("S RA= %08x\n", sp[0]);
        break;
      case 3:
        printf("L RA= %08x INS= %08x\n", sp[0], *((u32*)sp[0]));
        break;
      case 4:
        printf("RA= %08x\n", sp[0]);
        break;
      case 5:
        printf("GP= %08x\n", sp[2]);
        break;
      case 6:
        printf("R1= %08x RA= %08x\n", sp[27], sp[0]);
/*        __asm__ __volatile__( "cache 0x04, 0(%2)\n\t"
                              "mfc0  %0, $28, 0\n\t"
                              "mfc0  %1, $28, 1\n\t"
                              : "=r" (a), "=r" (b)
                              : "r" (*((u32*)(sp[0]-8)))
        );
        printf("TagL %08x DataL %08x\n", a, b);*/
        break;
      case 7:
        printf("R1= %08x R4= %08x\n", sp[27], sp[24]);
        break;
      case 8:
        printf("GP= %08x\n", sp[2]);
        break;
      case 9:
        printf("INS= %08x RA= %08x\n", *((u32*)(sp[0] | 0x20000000)), sp[0]);
        printf("GP= %08x\n", sp[2]);
/*        __asm__ __volatile__( "cache 0x04, 0(%2)\n\t"
                              "mfc0  %0, $28, 0\n\t"
                              "mfc0  %1, $28, 1\n\t"
                              : "=r" (a), "=r" (b)
                              : "r" (*((u32*)sp[0]))
        );
        printf("TagL %08x DataL %08x\n", a, b);*/
        break;
      case 10:
        printf("R1= %08x RA= %08x\n", sp[27], sp[0]);
        break;
      case 11:
        printf("INS= %08x RA= %08x\n", *((u32*)(sp[0] | 0x20000000)), sp[0]);
        break;
      case 30:
        printf("Bad error RA= %08x\n", sp[0]);
      case 31:
        printf("RA= %08x Dual PC= %08x\n", sp[0], sp[24]);
      case 40:
        printf("PUSH\n");
        printf("AT= %08x  ra= %08x  fp= %08x  gp= %08x\n",sp[27],sp[0],sp[1],sp[2]);
        printf("t9= %08x  t8= %08x  s7= %08x  S6= %08x\n",sp[3],sp[4],sp[5],sp[6]);
        printf("s5= %08x  s4= %08x  s3= %08x  s2= %08x\n",sp[7],sp[8],sp[9],sp[10]);
        printf("s1= %08x  s0= %08x  t7= %08x  t6= %08x\n",sp[11],sp[12],sp[13],sp[14]);
        printf("t5= %08x  t4= %08x  t3= %08x  t2= %08x\n",sp[15],sp[16],sp[17],sp[18]);
        printf("t1= %08x  t0= %08x  a3= %08x  a2= %08x\n",sp[19],sp[20],sp[21],sp[22]);
        printf("a1= %08x  a0= %08x  v1= %08x  v0= %08x\n",sp[23],sp[24],sp[25], sp[26]);
        break;
      case 41:
        printf("POP\n");
        printf("AT= %08x  ra= %08x  fp= %08x  gp= %08x\n",sp[27],sp[0],sp[1],sp[2]);
        printf("t9= %08x  t8= %08x  s7= %08x  S6= %08x\n",sp[3],sp[4],sp[5],sp[6]);
        printf("s5= %08x  s4= %08x  s3= %08x  s2= %08x\n",sp[7],sp[8],sp[9],sp[10]);
        printf("s1= %08x  s0= %08x  t7= %08x  t6= %08x\n",sp[11],sp[12],sp[13],sp[14]);
        printf("t5= %08x  t4= %08x  t3= %08x  t2= %08x\n",sp[15],sp[16],sp[17],sp[18]);
        printf("t1= %08x  t0= %08x  a3= %08x  a2= %08x\n",sp[19],sp[20],sp[21],sp[22]);
        printf("a1= %08x  a0= %08x  v1= %08x  v0= %08x\n",sp[23],sp[24],sp[25], sp[26]);
        break;
      case 42:
        printf("mips_update_gba GP= %08x\n", sp[2]);
        break;
      case 43:
        printf("mips_branch_arm GP= %08x\n", sp[2]);
        break;
      case 44:
        printf("mips_branch_thumb GP= %08x\n", sp[2]);
        break;
      case 45:
        printf("mips_branch_dual GP= %08x\n", sp[2]);
        break;
    }
}
#endif

void show_reg(u32 num, u32 *ptr, u32*sp)
{
    printf("Show %d %08x\n", num, ptr);
    printf("AT= %08x  ra= %08x  fp= %08x  gp= %08x\n",sp[30],sp[0],sp[1],sp[3]);
    printf("t9= %08x  t8= %08x  s7= %08x  S6= %08x\n",sp[6],sp[7],sp[8],sp[9]);
    printf("s5= %08x  s4= %08x  s3= %08x  s2= %08x\n",sp[10],sp[11],sp[12],sp[13]);
    printf("s1= %08x  s0= %08x  t7= %08x  t6= %08x\n",sp[14],sp[15],sp[16],sp[17]);
    printf("t5= %08x  t4= %08x  t3= %08x  t2= %08x\n",sp[18],sp[19],sp[20],sp[21]);
    printf("t1= %08x  t0= %08x  a3= %08x  a2= %08x\n",sp[22],sp[23],sp[24],sp[25]);
    printf("a1= %08x  a0= %08x  v1= %08x  v0= %08x\n",sp[26],sp[27],sp[28], sp[29]);
}

void show_printf_reg(u32 num, u32*sp)
{
//  if(num < 0)
//  {
    printf("Print %d sp= %08x\n", num, sp[2]);
    printf("AT= %08x  ra= %08x  fp= %08x  gp= %08x\n",sp[30],sp[0],sp[1],sp[3]);
    printf("t9= %08x  t8= %08x  s7= %08x  S6= %08x\n",sp[6],sp[7],sp[8],sp[9]);
    printf("s5= %08x  s4= %08x  s3= %08x  s2= %08x\n",sp[10],sp[11],sp[12],sp[13]);
    printf("s1= %08x  s0= %08x  t7= %08x  t6= %08x\n",sp[14],sp[15],sp[16],sp[17]);
    printf("t5= %08x  t4= %08x  t3= %08x  t2= %08x\n",sp[18],sp[19],sp[20],sp[21]);
    printf("t1= %08x  t0= %08x  a3= %08x  a2= %08x\n",sp[22],sp[23],sp[24],sp[25]);
    printf("a1= %08x  a0= %08x  v1= %08x  v0= %08x\n",sp[26],sp[27],sp[28], sp[29]);
//  }
}



