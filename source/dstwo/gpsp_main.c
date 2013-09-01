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

char *file_ext[] = { ".gba", ".bin", ".zip", NULL };

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

	if(gui_init(0) < 0)
		quit();
	// Initialidse paths
	initial_gpsp_config();

    init_video();
	power_flag = 0;

	// 初始化
	init_game_config();

	init_main();
	init_sound();

	// 分配仿真ROM
	init_gamepak_buffer();

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

	init_cpu(gpsp_persistent_config.BootFromBIOS);
	init_memory();
	reset_sound();

	ReGBA_Menu(REGBA_MENU_ENTRY_REASON_NO_ROM);

	last_frame = 0;

	real_frame_count = 0;
	virtual_frame_count = 0;

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
					if(rewind_queue_len > 0)
					{
						if(frame_ticks > 3)
						{
							if(pen)
								mdelay(500);

							loadstate_rewind();
							pen = 1;
							frame_ticks = 0;
							continue;
						}
					}
					else if(frame_ticks > 3)
					{
						u32 HotkeyRewind = game_persistent_config.HotkeyRewind != 0 ? game_persistent_config.HotkeyRewind : gpsp_persistent_config.HotkeyRewind;

						struct key_buf inputdata;
						ds2_getrawInput(&inputdata);

						while (inputdata.key & HotkeyRewind)
						{
							ds2_getrawInput(&inputdata);
						}
					}
				}
				else if(frame_ticks ==0)
				{
					savestate_rewind();
				}
			}
#if 0
          if((power_flag == 1) && (into_suspend() != 0))
            continue;
#endif

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

            Stats.EmulatedFrames++;
            Stats.TotalEmulatedFrames++;
            if(!skip_next_frame_flag)
            {
                Stats.RenderedFrames++;
                ReGBA_DisplayFPS();
                ReGBA_RenderScreen();
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

void reset_gba()
{
  init_main();
  init_memory();
  init_cpu(gpsp_persistent_config.BootFromBIOS);
  reset_sound();
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

char* FS_FGets(char *buffer, int num, FILE_TAG_TYPE stream)
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
