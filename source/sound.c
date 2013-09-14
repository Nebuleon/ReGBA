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
 * sound.c
 * 外围声音处理
 ******************************************************************************/

/******************************************************************************
 *
 ******************************************************************************/
#include "common.h"

/******************************************************************************
 * 宏定义
 ******************************************************************************/
#define GBC_NOISE_WRAP_FULL 32767
#define GBC_NOISE_WRAP_HALF 126

#define RENDER_SAMPLE_NULL()                                                  \

// 音频缓冲区左声道的数据
#define RENDER_SAMPLE_LEFT()                                                  \
  sound_buffer[sound_write_offset] += current_sample +                        \
   FP16_16_TO_U32((next_sample - current_sample) * fifo_fractional)           \

// 音频缓冲区右声道的数据
#define RENDER_SAMPLE_RIGHT()                                                 \
  sound_buffer[sound_write_offset + 1] += current_sample +                    \
   FP16_16_TO_U32((next_sample - current_sample) * fifo_fractional)           \

// 音频缓冲区左右声道的数据
#define RENDER_SAMPLE_BOTH()                                                  \
  dest_sample = current_sample +                                              \
   FP16_16_TO_U32((next_sample - current_sample) * fifo_fractional);          \
  sound_buffer[sound_write_offset] += dest_sample;                            \
  sound_buffer[sound_write_offset + 1] += dest_sample                         \

#define RENDER_SAMPLES(type)                                                  \
  while(fifo_fractional <= FP16_16_MAX_FRACTIONAL_PART)                       \
  {                                                                           \
    RENDER_SAMPLE_##type();                                                   \
    fifo_fractional += frequency_step;                                        \
    sound_write_offset = (sound_write_offset + 2) & BUFFER_SIZE_MASK;         \
  }                                                                           \

#define UPDATE_VOLUME_CHANNEL_ENVELOPE(channel)                               \
  volume_##channel = gbc_sound_envelope_volume_table[envelope_volume] *       \
   gbc_sound_channel_volume_table[gbc_sound_master_volume_##channel] *        \
   gbc_sound_master_volume_table[gbc_sound_master_volume]                     \

#define UPDATE_VOLUME_CHANNEL_NOENVELOPE(channel)                             \
  volume_##channel = gs->wave_volume *                                        \
   gbc_sound_channel_volume_table[gbc_sound_master_volume_##channel] *        \
   gbc_sound_master_volume_table[gbc_sound_master_volume]                     \

#define UPDATE_VOLUME(type)                                                   \
  UPDATE_VOLUME_CHANNEL_##type(left);                                         \
  UPDATE_VOLUME_CHANNEL_##type(right)                                         \

#define UPDATE_TONE_SWEEP()                                                   \
  if(gs->sweep_status)                                                        \
  {                                                                           \
    u32 sweep_ticks = gs->sweep_ticks - 1;                                    \
                                                                              \
    if(sweep_ticks == 0)                                                      \
    {                                                                         \
      u32 rate = gs->rate;                                                    \
                                                                              \
      if(gs->sweep_direction)                                                 \
        rate = rate - (rate >> gs->sweep_shift);                              \
      else                                                                    \
        rate = rate + (rate >> gs->sweep_shift);                              \
                                                                              \
      if (rate > 2047)                                                        \
      {                                                                       \
        rate = 2047;                                                          \
        gs->active_flag = 0;                                                  \
        break;                                                                \
      }                                                                       \
                                                                              \
      frequency_step = FLOAT_TO_FP16_16((1048576.0 / SOUND_FREQUENCY) / (2048 - rate)); \
                                                                              \
      gs->frequency_step = frequency_step;                                    \
      gs->rate = rate;                                                        \
                                                                              \
      sweep_ticks = gs->sweep_initial_ticks;                                  \
    }                                                                         \
    gs->sweep_ticks = sweep_ticks;                                            \
  }                                                                           \

#define UPDATE_TONE_NOSWEEP()                                                 \

#define UPDATE_TONE_ENVELOPE()                                                \
  if(gs->envelope_status)                                                     \
  {                                                                           \
    u32 envelope_ticks = gs->envelope_ticks - 1;                              \
    envelope_volume = gs->envelope_volume;                                    \
                                                                              \
    if(envelope_ticks == 0)                                                   \
    {                                                                         \
      if(gs->envelope_direction)                                              \
      {                                                                       \
        if(envelope_volume != 15)                                             \
          envelope_volume = gs->envelope_volume + 1;                          \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        if(envelope_volume != 0)                                              \
          envelope_volume = gs->envelope_volume - 1;                          \
      }                                                                       \
                                                                              \
      UPDATE_VOLUME(ENVELOPE);                                                \
                                                                              \
      gs->envelope_volume = envelope_volume;                                  \
      gs->envelope_ticks = gs->envelope_initial_ticks;                        \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      gs->envelope_ticks = envelope_ticks;                                    \
    }                                                                         \
  }                                                                           \

#define UPDATE_TONE_NOENVELOPE()                                              \

#define UPDATE_TONE_COUNTERS(envelope_op, sweep_op)                           \
  tick_counter += gbc_sound_tick_step;                                        \
  if(tick_counter > 0xFFFF)                                                   \
  {                                                                           \
    tick_counter &= 0xFFFF;                                                   \
    if(gs->length_status)                                                     \
    {                                                                         \
      u32 length_ticks = gs->length_ticks - 1;                                \
      gs->length_ticks = length_ticks;                                        \
                                                                              \
      if(length_ticks == 0)                                                   \
      {                                                                       \
        gs->active_flag = 0;                                                  \
        break;                                                                \
      }                                                                       \
    }                                                                         \
    UPDATE_TONE_##envelope_op();                                              \
    UPDATE_TONE_##sweep_op();                                                 \
  }                                                                           \

// サウンドバッファにLEFT CHANNELのデータを書込む
#define GBC_SOUND_RENDER_SAMPLE_LEFT()                                        \
  sound_buffer[sound_write_offset] += (current_sample * volume_left) >> 22    \

// サウンドバッファにRIGHT CHANNELのデータを書込む
#define GBC_SOUND_RENDER_SAMPLE_RIGHT()                                       \
  sound_buffer[sound_write_offset + 1] += (current_sample * volume_right) >> 22 \

// サウンドバッファにLEFT/RIGHT CHANNELのデータを書込む
#define GBC_SOUND_RENDER_SAMPLE_BOTH()                                        \
  GBC_SOUND_RENDER_SAMPLE_RIGHT();                                            \
  GBC_SOUND_RENDER_SAMPLE_LEFT()                                              \

#define GBC_SOUND_RENDER_SAMPLES(type, sample_length, envelope_op, sweep_op)  \
  for(i = 0; i < buffer_ticks; i++)                                           \
  {                                                                           \
    current_sample =                                                          \
     sample_data[FP16_16_TO_U32(sample_index) % sample_length];               \
    GBC_SOUND_RENDER_SAMPLE_##type();                                         \
                                                                              \
    sample_index += frequency_step;                                           \
    sound_write_offset = (sound_write_offset + 2) & BUFFER_SIZE_MASK;         \
                                                                              \
    UPDATE_TONE_COUNTERS(envelope_op, sweep_op);                              \
  }                                                                           \

#define GET_NOISE_SAMPLE_FULL()                                               \
  current_sample =                                                            \
   ((s32)(noise_table15[FP16_16_TO_U32(sample_index) >> 5] <<                 \
   (FP16_16_TO_U32(sample_index) & 0x1F)) >> 31) & 0x0F                       \

#define GET_NOISE_SAMPLE_HALF()                                               \
  current_sample =                                                            \
   ((s32)(noise_table7[FP16_16_TO_U32(sample_index) >> 5] <<                  \
   (FP16_16_TO_U32(sample_index) & 0x1F)) >> 31) & 0x0F                       \

#define GBC_SOUND_RENDER_NOISE(type, noise_type, envelope_op, sweep_op)       \
  for(i = 0; i < buffer_ticks; i++)                                           \
  {                                                                           \
    GET_NOISE_SAMPLE_##noise_type();                                          \
    GBC_SOUND_RENDER_SAMPLE_##type();                                         \
                                                                              \
    sample_index += frequency_step;                                           \
                                                                              \
    if(sample_index >= U32_TO_FP16_16(GBC_NOISE_WRAP_##noise_type))           \
      sample_index -= U32_TO_FP16_16(GBC_NOISE_WRAP_##noise_type);            \
                                                                              \
    sound_write_offset = (sound_write_offset + 2) & BUFFER_SIZE_MASK;         \
                                                                              \
    UPDATE_TONE_COUNTERS(envelope_op, sweep_op);                              \
  }                                                                           \

#define GBC_SOUND_RENDER_CHANNEL(type, sample_length, envelope_op, sweep_op)  \
  sound_write_offset = gbc_sound_buffer_index;                                \
  sample_index = gs->sample_index;                                            \
  frequency_step = gs->frequency_step;                                        \
  tick_counter = gs->tick_counter;                                            \
                                                                              \
  UPDATE_VOLUME(envelope_op);                                                 \
                                                                              \
  switch(gs->status)                                                          \
  {                                                                           \
    case GBC_SOUND_INACTIVE:                                                  \
      break;                                                                  \
                                                                              \
    case GBC_SOUND_LEFT:                                                      \
      GBC_SOUND_RENDER_##type(LEFT, sample_length, envelope_op, sweep_op);    \
      break;                                                                  \
                                                                              \
    case GBC_SOUND_RIGHT:                                                     \
      GBC_SOUND_RENDER_##type(RIGHT, sample_length, envelope_op, sweep_op);   \
      break;                                                                  \
                                                                              \
    case GBC_SOUND_LEFTRIGHT:                                                 \
      GBC_SOUND_RENDER_##type(BOTH, sample_length, envelope_op, sweep_op);    \
      break;                                                                  \
  }                                                                           \
                                                                              \
  gs->sample_index = sample_index;                                            \
  gs->tick_counter = tick_counter;                                            \

#define GBC_SOUND_LOAD_WAVE_RAM(bank)                                         \
  wave_bank = wave_samples + (bank * 32);                                     \
  for(i = 0, i2 = 0; i < 16; i++, i2 += 2)                                    \
  {                                                                           \
    current_sample = wave_ram[i];                                             \
    wave_bank[i2] = (((current_sample >> 4) & 0x0F) - 8);                     \
    wave_bank[i2 + 1] = ((current_sample & 0x0F) - 8);                        \
  }                                                                           \

#define sound_savestate_body(type)                                            \
{                                                                             \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, sound_on);                       \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, sound_read_offset);              \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, sound_last_cpu_ticks);           \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, gbc_sound_buffer_index);         \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, gbc_sound_last_cpu_ticks);       \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, gbc_sound_partial_ticks);        \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, gbc_sound_master_volume_left);   \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, gbc_sound_master_volume_right);  \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, gbc_sound_master_volume);        \
  FILE_##type##_ARRAY(g_state_buffer_ptr, wave_samples);                      \
  FILE_##type##_ARRAY(g_state_buffer_ptr, direct_sound_channel);              \
  FILE_##type##_ARRAY(g_state_buffer_ptr, gbc_sound_channel);                 \
}                                                                             \

/******************************************************************************
 * 定义全局变量
 ******************************************************************************/
DIRECT_SOUND_STRUCT direct_sound_channel[2];
GBC_SOUND_STRUCT gbc_sound_channel[4];
u32 sound_on = 0;
u32 gbc_sound_wave_volume[4] = { 0, 16384, 8192, 4096 };
// u32 left_buffer;

/******************************************************************************
 * 局部变量定义
 ******************************************************************************/
static s16 sound_buffer[BUFFER_SIZE];       // 音频缓冲区 2n = Left / 2n+1 = Right
static u32 sound_read_offset = 0;  // 音频缓冲区读指针
static u32 sound_last_cpu_ticks = 0;
static FIXED16_16 gbc_sound_tick_step;

/******************************************************************************
 * 本地函数声明
 ******************************************************************************/
static void init_noise_table(u32 *table, u32 period, u32 bit_length);

/******************************************************************************
 * 全局函数定义
 ******************************************************************************/
// Queue 1, 2, or 4 samples to the top of the DS FIFO, wrap around circularly
// マジカルバケーションの不具合修正
void sound_timer_queue32(u8 channel)
  {
    DIRECT_SOUND_STRUCT *ds = direct_sound_channel + channel;
    u8 offset = channel * 4;
    u8 i;

    for (i = 0xA0; i <= 0xA3; i++)
    {
      ds->fifo[ds->fifo_top] = ADDRESS8(io_registers, i + offset);
      ds->fifo_top = (ds->fifo_top + 1) % 32;
    }
  }

void sound_timer(FIXED16_16 frequency_step, u32 channel)
  {
    DIRECT_SOUND_STRUCT *ds = direct_sound_channel + channel;

    FIXED16_16 fifo_fractional = ds->fifo_fractional;
    u32 sound_write_offset = ds->buffer_index;
    s16 current_sample, next_sample, dest_sample;

    current_sample = ds->fifo[ds->fifo_base] << 4;
    ds->fifo_base = (ds->fifo_base + 1) % 32;
    next_sample = ds->fifo[ds->fifo_base] << 4;

    if (sound_on == 1)
    {
      if (ds->volume == DIRECT_SOUND_VOLUME_50)
      {
        current_sample >>= 1;
        next_sample >>= 1;
      }

      switch (ds->status)
        {
          case DIRECT_SOUND_INACTIVE:
            RENDER_SAMPLES(NULL);
            break;

          case DIRECT_SOUND_RIGHT:
            RENDER_SAMPLES(RIGHT);
            break;

          case DIRECT_SOUND_LEFT:
            RENDER_SAMPLES(LEFT);
            break;

          case DIRECT_SOUND_LEFTRIGHT:
            RENDER_SAMPLES(BOTH);
            break;
        }
    }
    else
    {
      RENDER_SAMPLES(NULL);
    }

    ds->buffer_index = sound_write_offset;
    ds->fifo_fractional = FP16_16_FRACTIONAL_PART(fifo_fractional);

    // マジカルバケーションで動作が遅くなるのが改善される
    u8 fifo_length;

    if (ds->fifo_top > ds->fifo_base)
      fifo_length = ds->fifo_top - ds->fifo_base;
    else
      fifo_length = ds->fifo_top + (32 - ds->fifo_base);

      if (fifo_length <= 16)
      {
        if (dma[1].direct_sound_channel == channel)
          dma_transfer(dma + 1);

        if (dma[2].direct_sound_channel == channel)
          dma_transfer(dma + 2);
      }
  }

void sound_reset_fifo(u32 channel)
  {
    DIRECT_SOUND_STRUCT *ds = direct_sound_channel + channel;
    memset(ds->fifo, 0, 32);
  }

// Initial pattern data = 4bits (signed)
// Channel volume = 12bits
// Envelope volume = 14bits
// Master volume = 2bits

// Recalculate left and right volume as volume changes.
// To calculate the current sample, use (sample * volume) >> 16

// Square waves range from -8 (low) to 7 (high)

s8 square_pattern_duty[4][8] =
  {
    { 0xF8, 0xF8, 0xF8, 0xF8, 0x07, 0xF8, 0xF8, 0xF8 },
    { 0xF8, 0xF8, 0xF8, 0xF8, 0x07, 0x07, 0xF8, 0xF8 },
    { 0xF8, 0xF8, 0x07, 0x07, 0x07, 0x07, 0xF8, 0xF8 },
    { 0x07, 0x07, 0x07, 0x07, 0xF8, 0xF8, 0x07, 0x07 },
  };

s8 wave_samples[64];

u32 noise_table15[1024];
u32 noise_table7[4];

u32 gbc_sound_master_volume_table[4] = { 1, 2, 4, 0 };

u32 gbc_sound_channel_volume_table[8] =
  { FIXED_DIV(0, 7, 12),
    FIXED_DIV(1, 7, 12),
    FIXED_DIV(2, 7, 12),
    FIXED_DIV(3, 7, 12),
    FIXED_DIV(4, 7, 12),
    FIXED_DIV(5, 7, 12),
    FIXED_DIV(6, 7, 12),
    FIXED_DIV(7, 7, 12)
  };

u32 gbc_sound_envelope_volume_table[16] =
  { FIXED_DIV(0, 15, 14),
    FIXED_DIV(1, 15, 14),
    FIXED_DIV(2, 15, 14),
    FIXED_DIV(3, 15, 14),
    FIXED_DIV(4, 15, 14),
    FIXED_DIV(5, 15, 14),
    FIXED_DIV(6, 15, 14),
    FIXED_DIV(7, 15, 14),
    FIXED_DIV(8, 15, 14),
    FIXED_DIV(9, 15, 14),
    FIXED_DIV(10, 15, 14),
    FIXED_DIV(11, 15, 14),
    FIXED_DIV(12, 15, 14),
    FIXED_DIV(13, 15, 14),
    FIXED_DIV(14, 15, 14),
    FIXED_DIV(15, 15, 14) };

u32 gbc_sound_buffer_index = 0;

u32 gbc_sound_last_cpu_ticks = 0;
u32 gbc_sound_partial_ticks = 0;

u32 gbc_sound_master_volume_left;
u32 gbc_sound_master_volume_right;
u32 gbc_sound_master_volume;

void update_gbc_sound(u32 cpu_ticks)
  {
    // TODO 実数部のビット数を多くした方がいい？
    FIXED16_16 buffer_ticks= FLOAT_TO_FP16_16((cpu_ticks - gbc_sound_last_cpu_ticks) * (SOUND_FREQUENCY / SYS_CLOCK));
    u32 i, i2;
    GBC_SOUND_STRUCT *gs = gbc_sound_channel;
    FIXED16_16 sample_index, frequency_step;
    FIXED16_16 tick_counter;
    u32 sound_write_offset;
    s32 volume_left, volume_right;
    u32 envelope_volume;
    s32 current_sample;
    u32 sound_status= ADDRESS16(io_registers, 0x84) & 0xFFF0;
    s8 *sample_data;
    s8 *wave_bank;
    u8 *wave_ram = ((u8 *)io_registers) + 0x90;

    gbc_sound_partial_ticks += FP16_16_FRACTIONAL_PART(buffer_ticks);
    buffer_ticks = FP16_16_TO_U32(buffer_ticks);

    if (gbc_sound_partial_ticks > FP16_16_MAX_FRACTIONAL_PART)
    {
      buffer_ticks += FP16_16_TO_U32(gbc_sound_partial_ticks);
      gbc_sound_partial_ticks &= FP16_16_MAX_FRACTIONAL_PART;
    }

    if (sound_on == 1)
    {
      // Channel 0
      gs = gbc_sound_channel + 0;
      if (gs->active_flag)
      {
        sound_status |= 0x01;
        sample_data = gs->sample_data;
        envelope_volume = gs->envelope_volume;
        GBC_SOUND_RENDER_CHANNEL(SAMPLES, 8, ENVELOPE, SWEEP);
      }

      // Channel 1
      gs = gbc_sound_channel + 1;
      if (gs->active_flag)
      {
        sound_status |= 0x02;
        sample_data = gs->sample_data;
        envelope_volume = gs->envelope_volume;
        GBC_SOUND_RENDER_CHANNEL(SAMPLES, 8, ENVELOPE, NOSWEEP);
      }

      // Channel 2
      gs = gbc_sound_channel + 2;
      if (gbc_sound_wave_update)
      {
        GBC_SOUND_LOAD_WAVE_RAM(gs->wave_bank);
        gbc_sound_wave_update = 0;
      }

      if ((gs->active_flag) && (gs->master_enable))
      {
        sound_status |= 0x04;
        sample_data = wave_samples;
        if (gs->wave_type == 0)
        {
          if (gs->wave_bank == 1)
            sample_data += 32;

          GBC_SOUND_RENDER_CHANNEL(SAMPLES, 32, NOENVELOPE, NOSWEEP);
        }
        else
        {
          GBC_SOUND_RENDER_CHANNEL(SAMPLES, 64, NOENVELOPE, NOSWEEP);
        }
      }

      // Channel 3
      gs = gbc_sound_channel + 3;
      if (gs->active_flag)
      {
        sound_status |= 0x08;
        envelope_volume = gs->envelope_volume;

        if (gs->noise_type == 1)
        {
          GBC_SOUND_RENDER_CHANNEL(NOISE, HALF, ENVELOPE, NOSWEEP);
        }
        else
        {
          GBC_SOUND_RENDER_CHANNEL(NOISE, FULL, ENVELOPE, NOSWEEP);
        }
      }
    }

    ADDRESS16(io_registers, 0x84) = sound_status;

    gbc_sound_last_cpu_ticks = cpu_ticks;
    // サウンドタイミングの調整
    gbc_sound_buffer_index =(gbc_sound_buffer_index + (buffer_ticks << 1)) & BUFFER_SIZE_MASK;

    ReGBA_AudioUpdate();

	/* Work around a synchronisation issue between the Direct Sound channels
	 * and the Game Boy-style beeper channels. The Game Boy-style beepers
	 * shall have the reference buffer offsets, as they are always correct.
	 */
	for (i = 0; i < 2; i++)
	{
		if (direct_sound_channel[i].buffer_index > gbc_sound_buffer_index
		    /* but don't consider it too late if the ring buffer goes back to the start */
		 && direct_sound_channel[i].buffer_index - gbc_sound_buffer_index < BUFFER_SIZE / 2
		)
		{
#ifdef TRACE_SOUND
			ReGBA_Trace("I: Direct Sound channel %u write offset %u -> %u", i, direct_sound_channel[i].buffer_index, gbc_sound_buffer_index);
#endif
			direct_sound_channel[i].buffer_index = gbc_sound_buffer_index;
		}
	}
  }

void init_sound()
  {
    gbc_sound_tick_step = FLOAT_TO_FP16_16(256.0 / SOUND_FREQUENCY);

    init_noise_table(noise_table15, 32767, 14);
    init_noise_table(noise_table7, 127, 6);

    // 局部变量等初始化
    reset_sound();
  }

void reset_sound()
  {
    DIRECT_SOUND_STRUCT *ds = direct_sound_channel;
    GBC_SOUND_STRUCT *gs = gbc_sound_channel;
    u32 i;

    sound_on = 0;
    memset(sound_buffer, 0, sizeof(sound_buffer));

    for (i = 0; i < 2; i++, ds++)
    {
      ds->buffer_index = 0;
      ds->status = DIRECT_SOUND_INACTIVE;
      ds->fifo_top = 0;
      ds->fifo_base = 0;
      ds->fifo_fractional = 0;
      ds->last_cpu_ticks = 0;
      memset(ds->fifo, 0, sizeof(ds->fifo));
    }

    gbc_sound_buffer_index = 0;
    gbc_sound_last_cpu_ticks = 0;
    gbc_sound_partial_ticks = 0;
    sound_read_offset = 0;

    gbc_sound_master_volume_left = 0;
    gbc_sound_master_volume_right = 0;
    gbc_sound_master_volume = 0;
    memset(wave_samples, 0, sizeof(wave_samples));

    sound_on = 1;

    for (i = 0; i < 4; i++, gs++)
    {
      gs->status = GBC_SOUND_INACTIVE;
      gs->sample_data = square_pattern_duty[2];
      gs->active_flag = 0;
    }
  }

void sound_read_mem_savestate()
  sound_savestate_body(READ_MEM);

void sound_write_mem_savestate()
  sound_savestate_body(WRITE_MEM);

u32 ReGBA_GetAudioSamplesAvailable()
{
	return ((gbc_sound_buffer_index - sound_read_offset) & BUFFER_SIZE_MASK) / 2;
}

u32 ReGBA_LoadNextAudioSample(s16* Left, s16* Right)
{
	if (sound_read_offset == gbc_sound_buffer_index)
		return 0;

	*Left  = sound_buffer[sound_read_offset];
	sound_buffer[sound_read_offset] = 0;
	sound_read_offset = (sound_read_offset + 1) & BUFFER_SIZE_MASK;
	*Right = sound_buffer[sound_read_offset];
	sound_buffer[sound_read_offset] = 0;
	sound_read_offset = (sound_read_offset + 1) & BUFFER_SIZE_MASK;
	return 1;
}

  // Special thanks to blarrg for the LSFR frequency used in Meridian, as posted
  // on the forum at http://meridian.overclocked.org:
  // http://meridian.overclocked.org/cgi-bin/wwwthreads/showpost.pl?Board=merid
  // angeneraldiscussion&Number=2069&page=0&view=expanded&mode=threaded&sb=4
  // Hope you don't mind me borrowing it ^_-

void init_noise_table(u32 *table, u32 period, u32 bit_length)
  {
    u32 shift_register = 0xFF;
    u32 mask = ~(1 << bit_length);
    s32 table_pos, bit_pos;
    u32 current_entry;
    u32 table_period = (period + 31) / 32;

    // Bits are stored in reverse  order so they can be more easily moved to
    // bit 31, for sign extended shift down.

    for (table_pos = 0; table_pos < table_period; table_pos++)
    {
      current_entry = 0;
      for (bit_pos = 31; bit_pos >= 0; bit_pos--)
      {
        current_entry |= (shift_register & 0x01) << bit_pos;

        shift_register =((1 & (shift_register ^ (shift_register >> 1)))
            << bit_length) |((shift_register >> 1) & mask);
      }

      table[table_pos] = current_entry;
    }
  }
