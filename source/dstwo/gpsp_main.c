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

uint32_t skip_next_frame_flag = 0;

uint32_t cpu_ticks = 0;
uint32_t frame_ticks = 0;

uint32_t execute_cycles = 960;
int32_t video_count = 960;

unsigned int pen = 0;
uint32_t frame_interval = 60; // For in-memory saved states used in rewinding

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

static bool caches_inited = false;

void init_main()
{
  uint32_t i;

  skip_next_frame_flag = 0;

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

  caches_inited = true;

  StatsInitGame();
}

void quit(void)
{
#ifdef USE_DEBUG
	fclose(g_dbg_file);
#endif

	exit(EXIT_SUCCESS);
}

int gpsp_main(int argc, char *argv[])
{
	char load_filename[MAX_FILE];

	if(gui_init(0) < 0)
		quit();
	// Initialidse paths
	initial_gpsp_config();

	init_video();

	// 初始化
	init_game_config();

	init_main();
	init_sound();

	// BIOS的读入
	char bios_filename[MAX_FILE];
	sprintf(bios_filename, "%s/%s", main_path, "gba_bios.bin");
	int32_t bios_ret = load_bios(bios_filename);
	if(bios_ret == -1) // 当读取失败
	{
		fprintf(stderr, "The GBA BIOS is not present\nPlease see README.md for\nmore information\n\nLe BIOS GBA n'est pas present\nLisez README.md pour plus\nd'information (en anglais)");
		DS2_AwaitAnyButtons();
		exit(EXIT_FAILURE);
	}

	init_cpu(gpsp_persistent_config.BootFromBIOS);
	init_memory();

	ReGBA_Menu(REGBA_MENU_ENTRY_REASON_NO_ROM);

	execute_arm_translate(execute_cycles);

	return 0;
}

#define V_BLANK   (0x01)
#define H_BLANK   (0x02)
#define V_COUNTER (0x04)

uint32_t to_skip = 0;

uint32_t update_gba()
{
  // TODO このあたりのログをとる必要があるかも
  IRQ_TYPE irq_raised = IRQ_NONE;

  do
    {
      cpu_ticks += execute_cycles;

      reg[CHANGED_PC_STATUS] = 0;

      if(gbc_sound_update)
        {
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
      uint32_t vcount = io_registers[REG_VCOUNT];
      uint32_t dispstat = io_registers[REG_DISPSTAT];

      if(!(dispstat & H_BLANK))
      { // 非 H BLANK
        // 转 H BLANK
        video_count += 272;
        dispstat |= H_BLANK; // 设置 H BLANK

        if((dispstat & 0x01) == 0)
        { // 非 V BLANK
          // 无跳帧时
          update_scanline();

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

          affine_reference_x[0] = (int32_t)(ADDRESS32(io_registers, 0x28) << 4) >> 4;
          affine_reference_y[0] = (int32_t)(ADDRESS32(io_registers, 0x2C) << 4) >> 4;
          affine_reference_x[1] = (int32_t)(ADDRESS32(io_registers, 0x38) << 4) >> 4;
          affine_reference_y[1] = (int32_t)(ADDRESS32(io_registers, 0x3C) << 4) >> 4;

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
								usleep(500000);

							loadstate_rewind();
							pen = 1;
							frame_ticks = 0;
							continue;
						}
					}
					else if(frame_ticks > 3)
					{
						uint32_t HotkeyRewind = game_persistent_config.HotkeyRewind != 0 ? game_persistent_config.HotkeyRewind : gpsp_persistent_config.HotkeyRewind;

						DS2_AwaitNotAllButtonsIn(HotkeyRewind);
					}
				}
				else if(frame_ticks ==0)
				{
					savestate_rewind();
				}
			}

          update_backup();

#if 0
          process_cheats();
#endif

          update_gbc_sound(cpu_ticks);

          vcount = 0; // TODO vcountを0にするタイミングを検討

            Stats.EmulatedFrames++;
            Stats.TotalEmulatedFrames++;
            ReGBA_RenderScreen();
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
  } while(reg[CPU_HALT_STATE] != CPU_ACTIVE);
  return execute_cycles;
}

void reset_gba()
{
  init_main();
  init_memory();
  init_cpu(gpsp_persistent_config.BootFromBIOS);
  reset_sound();
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
MAIN_SAVESTATE_BODY(READ_MEM)

void main_write_mem_savestate()
MAIN_SAVESTATE_BODY(WRITE_MEM)
