/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MAIN_H
#define MAIN_H

#include "memory.h"

typedef enum
{
  auto_frameskip,
  manual_frameskip,
  no_frameskip
} frameskip_type;

extern u32 cpu_ticks;
extern u32 frame_ticks;
extern u32 execute_cycles;
extern frameskip_type current_frameskip_type;
extern u32 frameskip_value;
extern u32 random_skip;
extern u32 global_cycles_per_instruction;
extern u32 synchronize_flag;
extern u32 skip_next_frame;

extern u64 base_timestamp;

extern char main_path[MAX_PATH];

extern u32 update_backup_flag;
extern u32 clock_speed;

u32 update_gba();
void reset_gba();
void synchronize();
void quit();
void delay_us(u32 us_count);
void get_ticks_us(u64 *tick_return);
void game_name_ext(u8 *src, u8 *buffer, u8 *extension);
void main_write_mem_savestate();
void main_read_mem_savestate();

#define count_timer(timer_number)                                             \
  timer[timer_number].reload = 0x10000 - value;                               \
  if(timer_number < 2)                                                        \
  {                                                                           \
    u32 timer_reload =                                                        \
     timer[timer_number].reload << timer[timer_number].prescale;              \
    sound_update_frequency_step(timer_number);                                \
  }                                                                           \

#define adjust_sound_buffer(timer_number, channel)                            \
  if(timer[timer_number].direct_sound_channels & (0x01 << channel))           \
  {                                                                           \
    direct_sound_channel[channel].buffer_index =                              \
     (direct_sound_channel[channel].buffer_index + buffer_adjust) %           \
     BUFFER_SIZE;                                                             \
  }                                                                           \

#endif


