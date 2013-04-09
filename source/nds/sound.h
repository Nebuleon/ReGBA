/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
 * Copyright (C) 2007 ????? <?????>
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

/******************************************************************************
 * sound.h
 * サウンド周りの処理
 ******************************************************************************/
#ifndef SOUND_H
#define SOUND_H

/******************************************************************************
 * マクロ等の定義
 ******************************************************************************/
typedef enum
{
  DIRECT_SOUND_INACTIVE,
  DIRECT_SOUND_RIGHT,
  DIRECT_SOUND_LEFT,
  DIRECT_SOUND_LEFTRIGHT
} DIRECT_SOUND_STATUS_TYPE;

typedef enum
{
  DIRECT_SOUND_VOLUME_50,
  DIRECT_SOUND_VOLUME_100
} DIRECT_SOUND_VOLUME_TYPE;

typedef enum
{
  GBC_SOUND_INACTIVE,
  GBC_SOUND_RIGHT,
  GBC_SOUND_LEFT,
  GBC_SOUND_LEFTRIGHT
} GBC_SOUND_STATUS_TYPE;

typedef struct
{
  s8 fifo[32];
  u32 fifo_base;
  u32 fifo_top;
  FIXED16_16 fifo_fractional;
  // The + 1 is to give some extra room for linear interpolation
  // when wrapping around.
  u32 buffer_index;
  DIRECT_SOUND_STATUS_TYPE status;
  DIRECT_SOUND_VOLUME_TYPE volume;
  u32 last_cpu_ticks;
} DIRECT_SOUND_STRUCT;

typedef struct
{
  u32 rate;
  FIXED16_16 frequency_step;
  FIXED16_16 sample_index;
  FIXED16_16 tick_counter;
  u32 total_volume;
  u32 envelope_initial_volume;
  u32 envelope_volume;
  u32 envelope_direction;
  u32 envelope_status;
  u32 envelope_step;
  u32 envelope_ticks;
  u32 envelope_initial_ticks;
  u32 sweep_status;
  u32 sweep_direction;
  u32 sweep_ticks;
  u32 sweep_initial_ticks;
  u32 sweep_shift;
  u32 length_status;
  u32 length_ticks;
  u32 noise_type;
  u32 wave_type;
  u32 wave_bank;
  u32 wave_volume;
  GBC_SOUND_STATUS_TYPE status;
  u32 active_flag;
  u32 master_enable;
  s8 *sample_data;
} GBC_SOUND_STRUCT;

#define BUFFER_SIZE (0xffff)                        // バッファのバイト数。変更しないこと

#ifdef NDS_LAYER
#define AUDIO_LEN 1536
#endif

// Define NO_VOLATILE_SOUND somewhere if your port of gpSP does not need the
// sound to be volatile. This allows more compiler optimisations.
#ifdef NO_VOLATILE_SOUND
#define SOUND_RELATED
#else
#define SOUND_RELATED volatile
#endif

// This is the frequency of sound output by the GBA. It is stored in a
// buffer containing BUFFER_SIZE bytes, and AUDIO_LEN refers to samples
// at this frequency.
// The value should technically be 32768, but at least the GBA Video ROMs and
// Golden Sun - The Lost Age require this to be 2 times that.
#define SOUND_FREQUENCY (65536.0)

// OUTPUT_SOUND_FREQUENCY should be a power-of-2 fraction of SOUND_FREQUENCY;
// if not, sound.c's sound_update() needs to resample the output.
#define OUTPUT_SOUND_FREQUENCY 32768

#define OUTPUT_FREQUENCY_DIVISOR ((int) (SOUND_FREQUENCY) / (OUTPUT_SOUND_FREQUENCY))

#define GBC_SOUND_TONE_CONTROL_LOW(channel, address)                          \
{                                                                             \
  u32 initial_volume = (value >> 12) & 0x0F;                                  \
  u32 envelope_ticks = ((value >> 8) & 0x07) * 4;                             \
  gbc_sound_channel[channel].length_ticks = 64 - (value & 0x3F);              \
  gbc_sound_channel[channel].sample_data =                                    \
   square_pattern_duty[(value >> 6) & 0x03];                                  \
  gbc_sound_channel[channel].envelope_direction = (value >> 11) & 0x01;       \
  gbc_sound_channel[channel].envelope_initial_volume = initial_volume;        \
  gbc_sound_channel[channel].envelope_volume = initial_volume;                \
  gbc_sound_channel[channel].envelope_initial_ticks = envelope_ticks;         \
  gbc_sound_channel[channel].envelope_ticks = envelope_ticks;                 \
  gbc_sound_channel[channel].envelope_status = (envelope_ticks != 0);         \
  gbc_sound_channel[channel].envelope_volume = initial_volume;                \
  gbc_sound_update = 1;                                                       \
  ADDRESS16(io_registers, address) = value;                                   \
}                                                                             \

#define GBC_SOUND_TONE_CONTROL_HIGH(channel, address)                         \
{                                                                             \
  u32 rate = value & 0x7FF;                                                   \
  gbc_sound_channel[channel].rate = rate;                                     \
  gbc_sound_channel[channel].frequency_step = FLOAT_TO_FP16_16(((131072.0 / (2048.0 - rate)) * 8.0) / SOUND_FREQUENCY);  \
  gbc_sound_channel[channel].length_status = (value >> 14) & 0x01;            \
  if(value & 0x8000)                                                          \
  {                                                                           \
    gbc_sound_channel[channel].active_flag = 1;                               \
    gbc_sound_channel[channel].sample_index -= FLOAT_TO_FP16_16(1.0 / 12.0);  \
    gbc_sound_channel[channel].envelope_ticks =                               \
     gbc_sound_channel[channel].envelope_initial_ticks;                       \
    gbc_sound_channel[channel].envelope_volume =                              \
     gbc_sound_channel[channel].envelope_initial_volume;                      \
    /*gbc_sound_channel[channel].sweep_ticks =                                  \
     gbc_sound_channel[channel].sweep_initial_ticks;*/                          \
  }                                                                           \
                                                                              \
  gbc_sound_update = 1;                                                       \
  ADDRESS16(io_registers, address) = value;                                   \
}                                                                             \

#define GBC_SOUND_TONE_CONTROL_SWEEP()                                        \
{                                                                             \
  u32 sweep_ticks = ((value >> 4) & 0x07) * 2;                                \
  gbc_sound_channel[0].sweep_shift = value & 0x07;                            \
  gbc_sound_channel[0].sweep_direction = (value >> 3) & 0x01;                 \
  gbc_sound_channel[0].sweep_status = (value != 8);                           \
  gbc_sound_channel[0].sweep_ticks = sweep_ticks;                             \
  gbc_sound_channel[0].sweep_initial_ticks = sweep_ticks;                     \
  gbc_sound_update = 1;                                                       \
  ADDRESS16(io_registers, 0x60) = value;                                      \
}                                                                             \

#define GBC_SOUND_WAVE_CONTROL()                                              \
{                                                                             \
  gbc_sound_channel[2].wave_type = (value >> 5) & 0x01;                       \
  gbc_sound_channel[2].wave_bank = (value >> 6) & 0x01;                       \
  if(value & 0x80)                                                            \
  {                                                                           \
    gbc_sound_channel[2].master_enable = 1;                                   \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    gbc_sound_channel[2].master_enable = 0;                                   \
  }                                                                           \
                                                                              \
  gbc_sound_update = 1;                                                       \
  ADDRESS16(io_registers, 0x70) = value;                                      \
}                                                                             \

#define GBC_SOUND_TONE_CONTROL_LOW_WAVE()                                     \
{                                                                             \
  gbc_sound_channel[2].length_ticks = 256 - (value & 0xFF);                   \
  if((value >> 15) & 0x01)                                                    \
  {                                                                           \
    gbc_sound_channel[2].wave_volume = 12288;                                 \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    gbc_sound_channel[2].wave_volume =                                        \
     gbc_sound_wave_volume[(value >> 13) & 0x03];                             \
  }                                                                           \
  gbc_sound_update = 1;                                                       \
  ADDRESS16(io_registers, 0x72) = value;                                      \
}                                                                             \

#define GBC_SOUND_TONE_CONTROL_HIGH_WAVE()                                    \
{                                                                             \
  u32 rate = value & 0x7FF;                                                   \
  gbc_sound_channel[2].rate = rate;                                           \
  gbc_sound_channel[2].frequency_step = FLOAT_TO_FP16_16((2097152.0 / (2048.0 - rate)) / SOUND_FREQUENCY);         \
  gbc_sound_channel[2].length_status = (value >> 14) & 0x01;                  \
  if(value & 0x8000)                                                          \
  {                                                                           \
    gbc_sound_channel[2].sample_index = 0;                                    \
    gbc_sound_channel[2].active_flag = 1;                                     \
  }                                                                           \
  gbc_sound_update = 1;                                                       \
  ADDRESS16(io_registers, 0x74) = value;                                      \
}                                                                             \

#define GBC_SOUND_NOISE_CONTROL()                                             \
{                                                                             \
  u32 dividing_ratio = value & 0x07;                                          \
  u32 frequency_shift = (value >> 4) & 0x0F;                                  \
  if(dividing_ratio == 0)                                                     \
  {                                                                           \
    gbc_sound_channel[3].frequency_step =                                     \
     FLOAT_TO_FP16_16(1048576.0 / (1 << (frequency_shift + 1)) /              \
     SOUND_FREQUENCY);                                                        \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    gbc_sound_channel[3].frequency_step =                                     \
     FLOAT_TO_FP16_16(524288.0 / (dividing_ratio *                            \
     (1 << (frequency_shift + 1))) / SOUND_FREQUENCY);                        \
  }                                                                           \
  gbc_sound_channel[3].noise_type = (value >> 3) & 0x01;                      \
  gbc_sound_channel[3].length_status = (value >> 14) & 0x01;                  \
  if(value & 0x8000)                                                          \
  {                                                                           \
    gbc_sound_channel[3].sample_index = 0;                                    \
    gbc_sound_channel[3].active_flag = 1;                                     \
    gbc_sound_channel[3].envelope_ticks =                                     \
     gbc_sound_channel[3].envelope_initial_ticks;                             \
    gbc_sound_channel[3].envelope_volume =                                    \
     gbc_sound_channel[3].envelope_initial_volume;                            \
  }                                                                           \
  gbc_sound_update = 1;                                                       \
  ADDRESS16(io_registers, 0x7C) = value;                                      \
}                                                                             \

#define GBC_TRIGGER_SOUND_CHANNEL(channel)                                    \
  gbc_sound_channel[channel].status =                                         \
   ((value >> (channel + 11)) & 0x02) | ((value >> (channel + 8)) & 0x01)      \

#define GBC_TRIGGER_SOUND()                                                   \
{                                                                             \
  gbc_sound_master_volume_right = value & 0x07;                               \
  gbc_sound_master_volume_left = (value >> 4) & 0x07;                         \
                                                                              \
  GBC_TRIGGER_SOUND_CHANNEL(0);                                               \
  GBC_TRIGGER_SOUND_CHANNEL(1);                                               \
  GBC_TRIGGER_SOUND_CHANNEL(2);                                               \
  GBC_TRIGGER_SOUND_CHANNEL(3);                                               \
  ADDRESS16(io_registers, 0x80) = value;                                      \
}                                                                             \

#define TRIGGER_SOUND()                                                       \
{                                                                             \
  timer[0].direct_sound_channels =                                            \
   ((~value >> 13) & 0x02) | ((~value >> 10) & 0x01);                         \
  timer[1].direct_sound_channels =                                            \
   ((value >> 13) & 0x02) | ((value >> 10) & 0x01);                           \
                                                                              \
  direct_sound_channel[0].volume = (value >> 2) & 0x01;                       \
  direct_sound_channel[0].status = (value >> 8) & 0x03;                       \
  direct_sound_channel[1].volume = (value >> 3) & 0x01;                       \
  direct_sound_channel[1].status = (value >> 12) & 0x03;                      \
  gbc_sound_master_volume = value & 0x03;                                     \
                                                                              \
  if((value >> 11) & 0x01)                                                    \
    sound_reset_fifo(0);                                                      \
  if((value >> 15) & 0x01)                                                    \
    sound_reset_fifo(1);                                                      \
  ADDRESS16(io_registers, 0x82) = value;                                      \
}                                                                             \

#define SOUND_ON()                                                            \
  if(value & 0x80)                                                            \
    sound_on = 1;                                                           \
  else                                                                        \
  {                                                                           \
    u32 i;                                                                    \
    for(i = 0; i < 4; i++)                                                    \
    {                                                                         \
      gbc_sound_channel[i].active_flag = 0;                                   \
    }                                                                         \
    sound_on = 0;                                                             \
  }                                                                           \
  ADDRESS16(io_registers, 0x84) =                                             \
    (ADDRESS16(io_registers, 0x84) & 0x000F) | (value & 0xFFF0);              \

#define SOUND_UPDATE_FREQUENCY_STEP(timer_number)                             \
  timer[timer_number].frequency_step =                                        \
   FLOAT_TO_FP16_16(SYS_CLOCK / (timer_reload * SOUND_FREQUENCY))             \

/******************************************************************************
 * グローバル変数の宣言
 ******************************************************************************/
extern DIRECT_SOUND_STRUCT direct_sound_channel[2];
extern GBC_SOUND_STRUCT gbc_sound_channel[4];
extern s8 square_pattern_duty[4][8];
extern u32 gbc_sound_master_volume_left;
extern u32 gbc_sound_master_volume_right;
extern u32 gbc_sound_master_volume;

extern u32 sound_on;

extern u32 left_buffer;
extern u32 gbc_sound_wave_volume[4];

extern SOUND_RELATED u32 gbc_sound_buffer_index;

void sound_timer_queue32(u8 channel);
void sound_timer(FIXED16_16 frequency_step, u32 channel);
void sound_reset_fifo(u32 channel);
void update_gbc_sound(u32 cpu_ticks);
void init_sound();
void sound_read_mem_savestate();
void sound_write_mem_savestate();
void pause_sound(u32 flag);
void reset_sound();
void sound_exit();
void synchronize_sound();

#ifdef NDS_LAYER
extern int sound_queue_hook(void);
#endif 


#endif /* SOUND_H */
