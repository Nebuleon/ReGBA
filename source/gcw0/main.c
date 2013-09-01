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

#include "common.h"
#include <sys/time.h>
#include <stdlib.h>

TIMER_TYPE timer[4];

frameskip_type current_frameskip_type = auto_frameskip;
u32 frameskip_value = 4;
u32 random_skip = 0;
u32 global_cycles_per_instruction = 3;
u32 skip_next_frame = 0;

u32 frameskip_counter = 0;

u32 cpu_ticks = 0;
u32 frame_ticks = 0;

u32 execute_cycles = 960;
s32 video_count = 960;
u32 ticks;

u32 arm_frame = 0;
u32 thumb_frame = 0;
u32 last_frame = 0;

u32 synchronize_flag = 1;

char main_path[MAX_PATH];

#define check_count(count_var)                                                \
  if(count_var < execute_cycles)                                              \
    execute_cycles = count_var;                                               \

#define check_timer(timer_number)                                             \
  if(timer[timer_number].status == TIMER_PRESCALE)                            \
    check_count(timer[timer_number].count);                                   \

#define update_timer(timer_number)                                            \
  if(timer[timer_number].status != TIMER_INACTIVE)                            \
  {                                                                           \
    if(timer[timer_number].status != TIMER_CASCADE)                           \
    {                                                                         \
      timer[timer_number].count -= execute_cycles;                            \
      io_registers[REG_TM##timer_number##D] =                                 \
       -(timer[timer_number].count >> timer[timer_number].prescale);          \
    }                                                                         \
                                                                              \
    if(timer[timer_number].count <= 0)                                        \
    {                                                                         \
      if(timer[timer_number].irq == TIMER_TRIGGER_IRQ)                        \
        irq_raised |= IRQ_TIMER##timer_number;                                \
                                                                              \
      if((timer_number != 3) &&                                               \
       (timer[timer_number + 1].status == TIMER_CASCADE))                     \
      {                                                                       \
        timer[timer_number + 1].count--;                                      \
        io_registers[REG_TM0D + (timer_number + 1) * 2] =                     \
         -(timer[timer_number + 1].count);                                    \
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
      timer[timer_number].count +=                                            \
       (timer[timer_number].reload << timer[timer_number].prescale);          \
    }                                                                         \
  }                                                                           \

char *file_ext[] = { ".gba", ".bin", ".zip", NULL };

static bool caches_inited = false;

void init_main()
{
  u32 i;

  skip_next_frame = 0;

  for(i = 0; i < 4; i++)
  {
    dma[i].start_type = DMA_INACTIVE;
    dma[i].direct_sound_channel = DMA_NO_DIRECT_SOUND;
    timer[i].status = TIMER_INACTIVE;
    timer[i].reload = 0x10000;
    timer[i].stop_cpu_ticks = 0;
  }

  timer[0].direct_sound_channels = TIMER_DS_CHANNEL_BOTH;
  timer[1].direct_sound_channels = TIMER_DS_CHANNEL_NONE;

  cpu_ticks = 0;
  frame_ticks = 0;

  execute_cycles = 960;
  video_count = 960;

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

int main(int argc, char *argv[])
{
  u32 i;
  u32 vcount = 0;
  u32 ticks;
  u32 dispstat;
  char load_filename[512];
  char bios_file[512];

  init_gamepak_buffer();

  // Copy the directory path of the executable into main_path
  sprintf(main_path, "%s/.gpsp", getenv("HOME"));
  mkdir(main_path, 0755);
#if 0
  load_config_file();
#endif

  sprintf(bios_file, "%s/gba_bios.bin", main_path);
  if(load_bios(bios_file) == -1)
  {
    printf("Sorry, but ReGBA requires a Gameboy Advance BIOS image to run\n");
    printf("correctly. Make sure to get an authentic one (search the web,\n");
    printf("beg other people if you want, but don't hold me accountable\n");
    printf("if you get hated or banned for it), it'll be exactly 16384\n");
    printf("bytes large and should have the following md5sum value:\n\n");
    printf("a860e8c0b6d573d191e4ec7db1b1e4f6\n\n");
    printf("Other BIOS files might work either partially completely, I\n");
    printf("really don't know.\n\n");
    printf("When you do get it name it gba_bios.bin and put it in\n");
    printf("/boot/local/home/.gpsp.\n\n");
    printf("Good luck.\n");

    quit();
  }

  init_main();

#if 0
  video_resolution_large();
#endif

  if(argc > 1)
  {
    if(load_gamepak(argv[1]) == -1)
    {
      printf("Failed to load gamepak %s, exiting.\n", load_filename);
      quit();
    }

	init_video();
#if 0
	init_sound();
	init_input();

    set_gba_resolution(screen_scale);
    video_resolution_small();
#endif

    init_cpu(1 /* boot from BIOS: yes */);
    init_memory();
  }
  else
  {
	    quit();
#if 0
	init_video();
	init_sound();
	init_input();

    if(load_file(file_ext, load_filename) == -1)
    {
		ReGBA_Menu(REGBA_MENU_ENTRY_REASON_NO_ROM);
    }
    else
    {
      if(load_gamepak(load_filename) == -1)
      {
        printf("Failed to load gamepak %s, exiting.\n", load_filename);
        delay_us(5000000);
        quit();
      }

      set_gba_resolution(screen_scale);
      video_resolution_small();
      clear_screen(0);
      flip_screen();
      init_cpu(1 /* boot from BIOS: yes */);
      init_memory();
    }
#endif
  }

#if 0
  last_frame = 0;
#endif

  // We'll never actually return from here.

  execute_arm_translate(execute_cycles);
  return 0;
}

u32 update_gba()
{
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
    {
      u32 vcount = io_registers[REG_VCOUNT];
      u32 dispstat = io_registers[REG_DISPSTAT];

      if((dispstat & 0x02) == 0)
      {
        // Transition from hrefresh to hblank
        video_count += (272);
        dispstat |= 0x02;

        if((dispstat & 0x01) == 0)
        {
          u32 i;

          update_scanline();

          // If in visible area also fire HDMA
          for(i = 0; i < 4; i++)
          {
            if(dma[i].start_type == DMA_START_HBLANK)
              dma_transfer(dma + i);
          }
        }

        if(dispstat & 0x10)
          irq_raised |= IRQ_HBLANK;
      }
      else
      {
        // Transition from hblank to next line
        video_count += 960;
        dispstat &= ~0x02;

        vcount++;

        if(vcount == 160)
        {
          // Transition from vrefresh to vblank
          u32 i;

          dispstat |= 0x01;
          if(dispstat & 0x8)
          {
            irq_raised |= IRQ_VBLANK;
          }

          affine_reference_x[0] =
           (s32)(ADDRESS32(io_registers, 0x28) << 4) >> 4;
          affine_reference_y[0] =
           (s32)(ADDRESS32(io_registers, 0x2C) << 4) >> 4;
          affine_reference_x[1] =
           (s32)(ADDRESS32(io_registers, 0x38) << 4) >> 4;
          affine_reference_y[1] =
           (s32)(ADDRESS32(io_registers, 0x3C) << 4) >> 4;

          for(i = 0; i < 4; i++)
          {
            if(dma[i].start_type == DMA_START_VBLANK)
              dma_transfer(dma + i);
          }
        }
        else

        if(vcount == 228)
        {
          // Transition from vblank to next screen
          dispstat &= ~0x01;
          frame_ticks++;

  #if 0
        printf("frame update (%x), %d instructions total, %d RAM flushes\n",
           reg[REG_PC], instruction_count - last_frame, flush_ram_count);
          last_frame = instruction_count;

/*          printf("%d gbc audio updates\n", gbc_update_count);
          printf("%d oam updates\n", oam_update_count); */
          gbc_update_count = 0;
          oam_update_count = 0;
          flush_ram_count = 0;
  #endif

          if(update_input())
            continue;

          update_gbc_sound(cpu_ticks);
#if 0
          synchronize();
#endif

		ReGBA_RenderScreen();

          update_backup();

#if 0
          process_cheats();
#endif

          vcount = 0;
        }

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

    if(irq_raised)
      raise_interrupt(irq_raised);

    execute_cycles = video_count;

    check_timer(0);
    check_timer(1);
    check_timer(2);
    check_timer(3);
  } while(reg[CPU_HALT_STATE] != CPU_ACTIVE);
  return execute_cycles;
}

u64 last_screen_timestamp = 0;
u32 frame_speed = 15000;

u32 ticks_needed_total = 0;
float us_needed = 0.0;
u32 frames = 0;
u32 skipped_num_frame = 60;
const u32 frame_interval = 60;
u32 skipped_num = 0;

#if 0
void synchronize()
{
  u64 new_ticks;
  char char_buffer[64];
  u64 time_delta = 16667;
  u32 fpsw = 60;

  get_ticks_us(&new_ticks);
  time_delta = new_ticks - last_screen_timestamp;
  last_screen_timestamp = new_ticks;
  ticks_needed_total += time_delta;

  skip_next_frame = 0;
  if((time_delta < frame_speed) && synchronize_flag)
  {
    delay_us(frame_speed - time_delta);
  }
  frames++;

  if(frames == 60)
  {
    if(status_display) {
      us_needed = (float)ticks_needed_total / frame_interval;
      fpsw = (u32)(1000000.0 / us_needed);
      ticks_needed_total = 0;
      if(current_frameskip_type == manual_frameskip) {
        sprintf(char_buffer, "%s%3dfps %s:%d slot:%d ", synchronize_flag?"  ":">>", fpsw,
          current_frameskip_type==auto_frameskip?"Auto":current_frameskip_type==manual_frameskip?"Manu":"Off ",
          frameskip_value, savestate_slot);
      } else {
        sprintf(char_buffer, "%s%3dfps %s:%d slot:%d ", synchronize_flag?"  ":">>",
         (skipped_num_frame==60&&fpsw>60)?fpsw:skipped_num_frame,
          current_frameskip_type==auto_frameskip?"Auto":current_frameskip_type==manual_frameskip?"Manu":"Off ",
          frameskip_value, savestate_slot);
      }
    } else
      strcpy(char_buffer, "                             ");
    print_string(char_buffer, 0xFFFF, 0x000, 40, 30);
    print_string(ssmsg, 0xF000, 0x000, 180, 30);
    strcpy(ssmsg, "     ");
    frames = 0;
	skipped_num_frame = 60;
  }

  if(current_frameskip_type == manual_frameskip)
  {
    frameskip_counter = (frameskip_counter + 1) %
     (frameskip_value + 1);
    if(random_skip)
    {
      if(frameskip_counter != (rand() % (frameskip_value + 1)))
        skip_next_frame = 1;
    }
    else
    {
      if(frameskip_counter)
        skip_next_frame = 1;
    }
  } else if(current_frameskip_type == auto_frameskip) {
    static struct timeval next1 = {0, 0};
    static struct timeval now;

    gettimeofday(&now, NULL);
    if(next1.tv_sec == 0) {
      next1 = now;
      next1.tv_usec++;
    }
    if(timercmp(&next1, &now, >)) {
      //struct timeval tdiff;
     if(synchronize_flag)
       do {
          synchronize_sound();
	      gettimeofday(&now, NULL);
       } while (timercmp(&next1, &now, >));
	 else
      gettimeofday(&now, NULL);
      //timersub(&next1, &now, &tdiff);
	  //usleep(tdiff.tv_usec/2);
	  //gettimeofday(&now, NULL);
	  skipped_num = 0;
	  next1 = now;
    } else {
      if(skipped_num < frameskip_value) {
        skipped_num++;
	    skipped_num_frame--;
        skip_next_frame = 1;
      } else {
        //synchronize_sound();
        skipped_num = 0;
	    next1 = now;
      }
	}
    next1.tv_usec += 16667;
    if(next1.tv_usec >= 1000000) {
      next1.tv_sec++;
      next1.tv_usec -= 1000000;
    }
  }
}
#endif

void quit()
{
  if(IsGameLoaded)
    update_backup_force();

#if 0
  sound_exit();
#endif

  SDL_Quit();
  exit(0);
}

void reset_gba()
{
  init_main();
  init_memory();
  init_cpu(1 /* boot from BIOS: yes */);
  reset_sound();
}

size_t FILE_LENGTH(FILE_TAG_TYPE fp)
{
  u32 length;

  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  return length;
}

void delay_us(u32 us_count)
{
  SDL_Delay(us_count / 1000); // for dingux
  // sleep(0);
}

void get_ticks_us(u64 *ticks_return)
{
  *ticks_return = (SDL_GetTicks() * 1000);
}

void change_ext(const char *src, char *buffer, char *extension)
{
  char *position;

  strcpy(buffer, main_path);
  strcat(buffer, "/");

  position = strrchr(src, '/');
  if (position)
	src = position+1;

  strcat(buffer, src);
  position = strrchr(buffer, '.');

  if(position)
    strcpy(position, extension);
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
