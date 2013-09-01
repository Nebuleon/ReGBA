/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licens e as
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

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include "common.h"

#define MAX_PATH 1024

// Blatantly stolen and trimmed from MZX (megazeux.sourceforge.net)
#ifdef ZAURUS
#define FILE_LIST_ROWS 20
#define FILE_LIST_POSITION 1
#define DIR_LIST_POSITION 240
#else
#define FILE_LIST_ROWS 25
#define FILE_LIST_POSITION 5
#define DIR_LIST_POSITION 360
#endif

#ifdef PSP_BUILD

#define color16(red, green, blue)                                             \
  (blue << 11) | (green << 5) | red                                           \

#else

#define color16(red, green, blue)                                             \
  (red << 11) | (green << 5) | blue                                           \

#endif

#define COLOR_BG            color16(20, 10, 10)
#define COLOR_ROM_INFO      color16(22, 36, 26)
#define COLOR_ACTIVE_ITEM   color16(31, 63, 31)
#define COLOR_INACTIVE_ITEM color16(13, 40, 18)
#define COLOR_FRAMESKIP_BAR color16(15, 31, 31)
#define COLOR_HELP_TEXT     color16(16, 40, 24)

int sort_function(const void *dest_str_ptr, const void *src_str_ptr)
{
  char *dest_str = *((char **)dest_str_ptr);
  char *src_str = *((char **)src_str_ptr);

  if(src_str[0] == '.')
    return 1;

  if(dest_str[0] == '.')
    return -1;

  return strcasecmp(dest_str, src_str);
}

s32 load_file(u8 **wildcards, u8 *result)
{
  DIR *current_dir;
  struct dirent *current_file;
  struct stat file_info;
  u8 current_dir_name[MAX_PATH];
  u8 current_dir_short[81];
  u32 current_dir_length;
  u32 total_filenames_allocated;
  u32 total_dirnames_allocated;
  u8 **file_list;
  u8 **dir_list;
  u32 num_files;
  u32 num_dirs;
  u8 *file_name;
  u32 file_name_length;
  u32 ext_pos = -1;
  u32 chosen_file, chosen_dir;
  u32 dialog_result = 1;
  s32 return_value = 1;
  u32 current_file_selection;
  u32 current_file_scroll_value;
  u32 current_dir_selection;
  u32 current_dir_scroll_value;
  u32 current_file_in_scroll;
  u32 current_dir_in_scroll;
  u32 current_file_number, current_dir_number;
  u32 current_column = 0;
  u32 repeat;
  u32 i;
  gui_action_type gui_action;

  while(return_value == 1)
  {
    current_file_selection = 0;
    current_file_scroll_value = 0;
    current_dir_selection = 0;
    current_dir_scroll_value = 0;
    current_file_in_scroll = 0;
    current_dir_in_scroll = 0;

    total_filenames_allocated = 32;
    total_dirnames_allocated = 32;
    file_list = (u8 **)malloc(sizeof(u8 *) * 32);
    dir_list = (u8 **)malloc(sizeof(u8 *) * 32);
    memset(file_list, 0, sizeof(u8 *) * 32);
    memset(dir_list, 0, sizeof(u8 *) * 32);

    num_files = 0;
    num_dirs = 0;
    chosen_file = 0;
    chosen_dir = 0;

	sprintf(current_dir_name, "%s/.gpsp", getenv("HOME"));
	mkdir(current_dir_name, 0755);
    current_dir = opendir(current_dir_name);

    do
    {
      if(current_dir)
        current_file = readdir(current_dir);
      else
        current_file = NULL;

      if(current_file)
      {
        file_name = current_file->d_name;
        file_name_length = strlen(file_name);

        if((stat(file_name, &file_info) >= 0) &&
         ((file_name[0] != '.') || (file_name[1] == '.')))
        {
          if(S_ISDIR(file_info.st_mode))
          {
            dir_list[num_dirs] =
             (u8 *)malloc(file_name_length + 1);
            strcpy(dir_list[num_dirs], file_name);

            num_dirs++;
          }
          else
          {
            // Must match one of the wildcards, also ignore the .
            if(file_name_length >= 4)
            {
              if(file_name[file_name_length - 4] == '.')
                ext_pos = file_name_length - 4;
              else

              if(file_name[file_name_length - 3] == '.')
                ext_pos = file_name_length - 3;

              else
                ext_pos = 0;

              for(i = 0; wildcards[i] != NULL; i++)
              {
                if(!strcasecmp((file_name + ext_pos),
                 wildcards[i]))
                {
                  file_list[num_files] =
                   (u8 *)malloc(file_name_length + 1);

                  strcpy(file_list[num_files], file_name);

                  num_files++;
                  break;
                }
              }
            }
          }
        }

        if(num_files == total_filenames_allocated)
        {
          file_list = (u8 **)realloc(file_list, sizeof(u8 *) *
           total_filenames_allocated * 2);
          memset(file_list + total_filenames_allocated, 0,
           sizeof(u8 *) * total_filenames_allocated);
          total_filenames_allocated *= 2;
        }

        if(num_dirs == total_dirnames_allocated)
        {
          dir_list = (u8 **)realloc(dir_list, sizeof(u8 *) *
           total_dirnames_allocated * 2);
          memset(dir_list + total_dirnames_allocated, 0,
           sizeof(u8 *) * total_dirnames_allocated);
          total_dirnames_allocated *= 2;
        }
      }
    } while(current_file);

    qsort((void *)file_list, num_files, sizeof(u8 *), sort_function);
    qsort((void *)dir_list, num_dirs, sizeof(u8 *), sort_function);

    closedir(current_dir);

    current_dir_length = strlen(current_dir_name);

    if(current_dir_length > 80)
    {
      memcpy(current_dir_short, "...", 3);
      memcpy(current_dir_short + 3,
       current_dir_name + current_dir_length - 77, 77);
      current_dir_short[80] = 0;
    }
    else
    {
      memcpy(current_dir_short, current_dir_name,
       current_dir_length + 1);
    }

    repeat = 1;

    if(num_files == 0)
      current_column = 1;

    clear_screen(COLOR_BG);
    u8 print_buffer[81];

    while(repeat)
    {
      flip_screen();

      print_string(current_dir_short, COLOR_ACTIVE_ITEM, COLOR_BG, 0, 0);

      print_string("Press Cancel to return to the main menu.",
       COLOR_HELP_TEXT, COLOR_BG, 20, 220);
      for(i = 0, current_file_number = i + current_file_scroll_value;
       i < FILE_LIST_ROWS; i++, current_file_number++)
      {
        if(current_file_number < num_files)
        {
          strncpy(print_buffer,file_list[current_file_number],38);
		  print_buffer[38] = 0;
          if((current_file_number == current_file_selection) &&
           (current_column == 0))
          {
            print_string(print_buffer, COLOR_ACTIVE_ITEM,
             COLOR_BG, FILE_LIST_POSITION, ((i + 1) * 10));
          }
          else
          {
            print_string(print_buffer, COLOR_INACTIVE_ITEM,
             COLOR_BG, FILE_LIST_POSITION, ((i + 1) * 10));
          }
        }
      }

      for(i = 0, current_dir_number = i + current_dir_scroll_value;
       i < FILE_LIST_ROWS; i++, current_dir_number++)
      {
        if(current_dir_number < num_dirs)
        {
          strncpy(print_buffer,dir_list[current_dir_number], 13);
		  print_buffer[14] = 0;
          if((current_dir_number == current_dir_selection) &&
           (current_column == 1))
          {
            print_string(print_buffer, COLOR_ACTIVE_ITEM,
             COLOR_BG, DIR_LIST_POSITION, ((i + 1) * 10));
          }
          else
          {
            print_string(print_buffer, COLOR_INACTIVE_ITEM,
             COLOR_BG, DIR_LIST_POSITION, ((i + 1) * 10));
          }
        }
      }

      gui_action = get_gui_input();

      switch(gui_action)
      {
        case CURSOR_DOWN:
          if(current_column == 0)
          {
            if(current_file_selection < (num_files - 1))
            {
              current_file_selection++;
              if(current_file_in_scroll == (FILE_LIST_ROWS - 1))
              {
                clear_screen(COLOR_BG);
                current_file_scroll_value++;
              }
              else
              {
                current_file_in_scroll++;
              }
            }
          }
          else
          {
            if(current_dir_selection < (num_dirs - 1))
            {
              current_dir_selection++;
              if(current_dir_in_scroll == (FILE_LIST_ROWS - 1))
              {
                clear_screen(COLOR_BG);
                current_dir_scroll_value++;
              }
              else
              {
                current_dir_in_scroll++;
              }
            }
          }

          break;

        case CURSOR_UP:
          if(current_column == 0)
          {
            if(current_file_selection)
            {
              current_file_selection--;
              if(current_file_in_scroll == 0)
              {
                clear_screen(COLOR_BG);
                current_file_scroll_value--;
              }
              else
              {
                current_file_in_scroll--;
              }
            }
          }
          else
          {
            if(current_dir_selection)
            {
              current_dir_selection--;
              if(current_dir_in_scroll == 0)
              {
                clear_screen(COLOR_BG);
                current_dir_scroll_value--;
              }
              else
              {
                current_dir_in_scroll--;
              }
            }
          }
          break;

        case CURSOR_RIGHT:
          if(current_column == 0)
          {
            if(num_dirs != 0)
              current_column = 1;
          }
          break;

        case CURSOR_LEFT:
          if(current_column == 1)
          {
            if(num_files != 0)
              current_column = 0;
          }
          break;

        case CURSOR_SELECT:
          if(current_column == 1)
          {
            repeat = 0;
            chdir(dir_list[current_dir_selection]);
          }
          else
          {
            if(num_files != 0)
            {
              repeat = 0;
              return_value = 0;
              strcpy(result, file_list[current_file_selection]);
            }
          }
          break;

        case CURSOR_BACK:
          repeat = 0;
          chdir("..");
          break;

        case CURSOR_EXIT:
          return_value = -1;
          repeat = 0;
          break;
      }
    }

    for(i = 0; i < num_files; i++)
    {
      free(file_list[i]);
    }
    free(file_list);

    for(i = 0; i < num_dirs; i++)
    {
      free(dir_list[i]);
    }
    free(dir_list);
  }

  clear_screen(COLOR_BG);
  return return_value;
}

typedef enum
{
  NUMBER_SELECTION_OPTION = 0x01,
  STRING_SELECTION_OPTION = 0x02,
  SUBMENU_OPTION          = 0x04,
  ACTION_OPTION           = 0x08
} menu_option_type_enum;

struct _menu_type
{
  void (* init_function)();
  void (* passive_function)();
  struct _menu_option_type *options;
  u32 num_options;
};

struct _menu_option_type
{
  void (* action_function)();
  void (* passive_function)();
  struct _menu_type *sub_menu;
  char *display_string;
  void *options;
  u32 *current_option;
  u32 num_options;
  char *help_string;
  u32 line_number;
  menu_option_type_enum option_type;
};

typedef struct _menu_option_type menu_option_type;
typedef struct _menu_type menu_type;

#define make_menu(name, init_function, passive_function)                      \
  menu_type name##_menu =                                                     \
  {                                                                           \
    init_function,                                                            \
    passive_function,                                                         \
    name##_options,                                                           \
    sizeof(name##_options) / sizeof(menu_option_type)                         \
  }                                                                           \

#define gamepad_config_option(display_string, number)                         \
{                                                                             \
  NULL,                                                                       \
  menu_fix_gamepad_help,                                                      \
  NULL,                                                                       \
  display_string ": %s",                                                      \
  gamepad_config_buttons,                                                     \
  gamepad_config_map + gamepad_config_line_to_psp_button[number],             \
  sizeof(gamepad_config_buttons) / sizeof(gamepad_config_buttons[0]),         \
  gamepad_help[gamepad_config_map[                                            \
   gamepad_config_line_to_psp_button[number]]],                               \
  number,                                                                     \
  STRING_SELECTION_OPTION                                                     \
}                                                                             \

#define analog_config_option(display_string, number)                          \
{                                                                             \
  NULL,                                                                       \
  menu_fix_gamepad_help,                                                      \
  NULL,                                                                       \
  display_string ": %s",                                                      \
  gamepad_config_buttons,                                                     \
  gamepad_config_map + number + 12,                                           \
  sizeof(gamepad_config_buttons) / sizeof(gamepad_config_buttons[0]),         \
  gamepad_help[gamepad_config_map[number + 12]],                              \
  number + 2,                                                                 \
  STRING_SELECTION_OPTION                                                     \
}                                                                             \

#define cheat_option(number)                                                  \
{                                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  NULL,                                                                       \
  cheat_format_str[number],                                                   \
  enable_disable_options,                                                     \
  &(cheats[number].cheat_active),                                             \
  2,                                                                          \
  "Activate/deactivate this cheat code.",                                     \
  number,                                                                     \
  STRING_SELECTION_OPTION                                                     \
}                                                                             \

#define action_option(action_function, passive_function, display_string,      \
 help_string, line_number)                                                    \
{                                                                             \
  action_function,                                                            \
  passive_function,                                                           \
  NULL,                                                                       \
  display_string,                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  0,                                                                          \
  help_string,                                                                \
  line_number,                                                                \
  ACTION_OPTION                                                               \
}                                                                             \

#define submenu_option(sub_menu, display_string, help_string, line_number)    \
{                                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  sub_menu,                                                                   \
  display_string,                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  sizeof(sub_menu) / sizeof(menu_option_type),                                \
  help_string,                                                                \
  line_number,                                                                \
  SUBMENU_OPTION                                                              \
}                                                                             \

#define selection_option(passive_function, display_string, options,           \
 option_ptr, num_options, help_string, line_number, type)                     \
{                                                                             \
  NULL,                                                                       \
  passive_function,                                                           \
  NULL,                                                                       \
  display_string,                                                             \
  options,                                                                    \
  option_ptr,                                                                 \
  num_options,                                                                \
  help_string,                                                                \
  line_number,                                                                \
  type                                                                        \
}                                                                             \

#define action_selection_option(action_function, passive_function,            \
 display_string, options, option_ptr, num_options, help_string, line_number,  \
 type)                                                                        \
{                                                                             \
  action_function,                                                            \
  passive_function,                                                           \
  NULL,                                                                       \
  display_string,                                                             \
  options,                                                                    \
  option_ptr,                                                                 \
  num_options,                                                                \
  help_string,                                                                \
  line_number,                                                                \
  type | ACTION_OPTION                                                        \
}                                                                             \


#define string_selection_option(passive_function, display_string, options,    \
 option_ptr, num_options, help_string, line_number)                           \
  selection_option(passive_function, display_string ": %s", options,          \
   option_ptr, num_options, help_string, line_number, STRING_SELECTION_OPTION)\

#define numeric_selection_option(passive_function, display_string,            \
 option_ptr, num_options, help_string, line_number)                           \
  selection_option(passive_function, display_string ": %d", NULL, option_ptr, \
   num_options, help_string, line_number, NUMBER_SELECTION_OPTION)            \

#define string_selection_action_option(action_function, passive_function,     \
 display_string, options, option_ptr, num_options, help_string, line_number)  \
  action_selection_option(action_function, passive_function,                  \
   display_string ": %s",  options, option_ptr, num_options, help_string,     \
   line_number, STRING_SELECTION_OPTION)                                      \

#define numeric_selection_action_option(action_function, passive_function,    \
 display_string, option_ptr, num_options, help_string, line_number)           \
  action_selection_option(action_function, passive_function,                  \
   display_string ": %d",  NULL, option_ptr, num_options, help_string,        \
   line_number, NUMBER_SELECTION_OPTION)                                      \

#define numeric_selection_action_hide_option(action_function,                 \
 passive_function, display_string, option_ptr, num_options, help_string,      \
 line_number)                                                                 \
  action_selection_option(action_function, passive_function,                  \
   display_string, NULL, option_ptr, num_options, help_string,                \
   line_number, NUMBER_SELECTION_OPTION)                                      \


#define GAMEPAD_MENU_WIDTH 15

u32 gamepad_config_line_to_psp_button[] =
 { 8, 6, 7, 9, 1, 2, 3, 0, 4, 5, 11, 10 };

s32 load_game_config_file()
{
  u8 game_config_filename[512];
  u32 file_loaded = 0;
  u32 i;
  change_ext(gamepak_filename, game_config_filename, ".cfg");

  FILE_OPEN(game_config_file, game_config_filename, read);

  if(file_check_valid(game_config_file))
  {
    u32 file_size = FILE_LENGTH(game_config_file);

    // Sanity check: File size must be the right size
    if(file_size == 56)
    {
      u32 file_options[file_size / 4];

      FILE_READ_ARRAY(game_config_file, file_options);
      current_frameskip_type = file_options[0] % 3;
      frameskip_value = file_options[1];
      random_skip = file_options[2] & 1;
      clock_speed = global_cycles_per_instruction = file_options[3];

      if(frameskip_value < 0)
        frameskip_value = 0;

      if(frameskip_value > 99)
        frameskip_value = 99;


      file_loaded = 1;
    }
    FILE_CLOSE(game_config_file);
  }

  if(file_loaded)
    return 0;

  current_frameskip_type = auto_frameskip;
  frameskip_value = 4;
  random_skip = 0;
  clock_speed = 4;

  for(i = 0; i < 10; i++)
  {
    cheats[i].cheat_active = 0;
  }

  return -1;
}

s32 load_config_file()
{
  u8 config_path[512];
    sprintf(config_path, "%s/%s", main_path, GPSP_CONFIG_FILENAME);

  FILE_OPEN(config_file, config_path, read);

  if(FILE_CHECK_VALID(config_file))
  {
    u32 file_size = FILE_LENGTH(config_file);

    // Sanity check: File size must be the right size
    if(file_size == 92)
    {
      u32 file_options[file_size / 4];
      u32 i;
      s32 menu_button = -1;
      file_read_array(config_file, file_options);

      screen_scale = file_options[0] % 2;
      screen_filter = file_options[1] % 2;
      global_enable_audio = file_options[2] % 2;
#ifdef ZAURUS
      audio_buffer_size_number = file_options[3] % 4;
#else
      audio_buffer_size_number = file_options[3] % 11;
#endif
      update_backup_flag = file_options[4] % 2;
      global_enable_analog = file_options[5] % 2;
      analog_sensitivity_level = file_options[6] % 8;

      // Sanity check: Make sure there's a MENU or FRAMESKIP
      // key, if not assign to triangle

      for(i = 0; i < 16; i++)
      {
        gamepad_config_map[i] = file_options[7 + i] %
         (BUTTON_ID_NONE + 1);

        if(gamepad_config_map[i] == BUTTON_ID_MENU)
        {
          menu_button = i;
        }
      }

      if(menu_button == -1)
      {
        gamepad_config_map[0] = BUTTON_ID_MENU;
      }
    }

    FILE_CLOSE(config_file);

    return 0;
  }

  return -1;
}

s32 save_game_config_file()
{
  u8 game_config_filename[512];
  u32 i;

  change_ext(gamepak_filename, game_config_filename, ".cfg");

  FILE_OPEN(game_config_file, game_config_filename, write);

  if(FILE_CHECK_VALID(game_config_file))
  {
    u32 file_options[14];

    file_options[0] = current_frameskip_type;
    file_options[1] = frameskip_value;
    file_options[2] = random_skip;
    file_options[3] = clock_speed;

    for(i = 0; i < 10; i++)
    {
      file_options[4 + i] = cheats[i].cheat_active;
    }

    FILE_WRITE_ARRAY(game_config_file, file_options);
    FILE_CLOSE(game_config_file);

    return 0;
  }

  return -1;
}

s32 save_config_file()
{
  u8 config_path[512];
    sprintf(config_path, "%s/%s", main_path, GPSP_CONFIG_FILENAME);

  FILE_OPEN(config_file, config_path, write);

  save_game_config_file();

  if(FILE_CHECK_VALID(config_file))
  {
    u32 file_options[23];
    u32 i;

    file_options[0] = screen_scale;
    file_options[1] = screen_filter;
    file_options[2] = global_enable_audio;
    file_options[3] = audio_buffer_size_number;
    file_options[4] = update_backup_flag;
    file_options[5] = global_enable_analog;
    file_options[6] = analog_sensitivity_level;

    for(i = 0; i < 16; i++)
    {
      file_options[7 + i] = gamepad_config_map[i];
    }

    FILE_WRITE_ARRAY(config_file, file_options);
    FILE_CLOSE(config_file);

    return 0;
  }

  return -1;
}

typedef enum
{
  MAIN_MENU,
  GAMEPAD_MENU,
  SAVESTATE_MENU,
  FRAMESKIP_MENU,
  CHEAT_MENU
} menu_enum;

u32 savestate_slot = 0;
u8  ssmsg[8];
u32 status_display = 0;

void get_savestate_snapshot(u8 *savestate_filename)
{
	/* Unimplemented. Need to figure out a way to show this screenshot on a
	 * 320x240 screen first. - Neb, 2013-08-19 */
#if 0
  u16 snapshot_buffer[240 * 160];
  u8 savestate_timestamp_string[80];

  FILE_OPEN(savestate_file, savestate_filename, read);

  if(FILE_CHECK_VALID(savestate_file))
  {
    u8 weekday_strings[7][11] =
    {
      "Sunday", "Monday", "Tuesday", "Wednesday",
      "Thursday", "Friday", "Saturday"
    };
    time_t savestate_time_flat;
    struct tm *current_time;
    file_read_array(savestate_file, snapshot_buffer);
    file_read_variable(savestate_file, savestate_time_flat);

    file_close(savestate_file);

    current_time = localtime(&savestate_time_flat);
    sprintf(savestate_timestamp_string,
     "%s  %02d/%02d/%04d  %02d:%02d:%02d                ",
     weekday_strings[current_time->tm_wday], current_time->tm_mon + 1,
     current_time->tm_mday, current_time->tm_year + 1900,
     current_time->tm_hour, current_time->tm_min, current_time->tm_sec);

    savestate_timestamp_string[40] = 0;
    print_string(savestate_timestamp_string, COLOR_HELP_TEXT, COLOR_BG,
     10, 40);
  }
  else
  {
    memset(snapshot_buffer, 0, 240 * 160 * 2);
    print_string_ext("No savestate exists for this slot.",
     0xFFFF, 0x0000, 15, 75, snapshot_buffer, 240, 0);
    print_string("---------- --/--/---- --:--:--          ", COLOR_HELP_TEXT,
     COLOR_BG, 10, 40);
  }
#endif
}

u32 ReGBA_Menu(enum ReGBA_MenuEntryReason EntryReason)
{
  u32 clock_speed_number = clock_speed - 1;
  u8 print_buffer[81];
  u32 _current_option = 0;
  gui_action_type gui_action;
  menu_enum _current_menu = MAIN_MENU;
  u32 i;
  u32 repeat = 1;
  u32 return_value = 0;
  u32 first_load = 0;
  u8 savestate_ext[16];
  u8 current_savestate_filename[512];
  u8 line_buffer[80];
  u8 cheat_format_str[10][41];

  menu_type *current_menu;
  menu_option_type *current_option;
  menu_option_type *display_option;
  u32 current_option_num;

  auto void choose_menu();
  auto void clear_help();

  u8 *gamepad_help[] =
  {
    "Up button on GBA d-pad.",
    "Down button on GBA d-pad.",
    "Left button on GBA d-pad.",
    "Right button on GBA d-pad.",
    "A button on GBA.",
    "B button on GBA.",
    "Left shoulder button on GBA.",
    "Right shoulder button on GBA.",
    "Start button on GBA.",
    "Select button on GBA.",
    "Brings up frameskip adjust bar and menu access.",
    "Jumps directly to the menu.",
    "Toggles fastforward on/off (don't expect it to do much or anything)",
    "Loads the game state from the current slot.",
    "Saves the game state to the current slot.",
    "Rapidly press/release the A button on GBA.",
    "Rapidly press/release the B button on GBA.",
    "Rapidly press/release the Left shoulder button on GBA.",
    "Rapidly press/release the Right shoulder button on GBA.",
    "Does nothing."
  };

  void menu_exit()
  {
    if(!first_load)
     repeat = 0;
  }

  void menu_quit()
  {
    clock_speed = clock_speed_number + 1;
    save_config_file();
    quit();
  }

  void menu_load()
  {
    u8 *file_ext[] = { ".gba", ".bin", ".zip", NULL };
    u8 load_filename[512];
    save_game_config_file();
    if(load_file(file_ext, load_filename) != -1)
    {
       if(load_gamepak(load_filename) == -1)
       {
         quit();
       }
       reset_gba();
       return_value = 1;
       repeat = 0;
       reg[CHANGED_PC_STATUS] = 1;
    }
    else
    {
      choose_menu(current_menu);
    }
  }

  void menu_restart()
  {
    if(!first_load)
    {
      reset_gba();
      reg[CHANGED_PC_STATUS] = 1;
      return_value = 1;
      repeat = 0;
    }
  }

  void menu_change_state()
  {
	  // TODO [Neb] Deal with this when creating the saved states menu
    // get_savestate_filename(savestate_slot, current_savestate_filename);
  }

  void menu_save_state()
  {
    if(!first_load)
    {
	  // TODO [Neb] Deal with this when creating the saved states menu
      // get_savestate_filename_noshot(savestate_slot,
      //  current_savestate_filename);
      save_state(current_savestate_filename, original_screen);
    }
    menu_change_state();
  }

  void menu_load_state()
  {
    if(!first_load)
    {
      load_state(current_savestate_filename);
      return_value = 1;
      repeat = 0;
    }
  }

  void menu_load_state_file()
  {
    u8 *file_ext[] = { ".svs", NULL };
    u8 load_filename[512];
    if(load_file(file_ext, load_filename) != -1)
    {
      load_state(load_filename);
      return_value = 1;
      repeat = 0;
    }
    else
    {
      choose_menu(current_menu);
    }
  }

  void menu_fix_gamepad_help()
  {
    clear_help();
    current_option->help_string =
     gamepad_help[gamepad_config_map[
     gamepad_config_line_to_psp_button[current_option_num]]];
  }

  void submenu_graphics_sound()
  {

  }

  void submenu_cheats_misc()
  {

  }

  void submenu_gamepad()
  {

  }

  void submenu_analog()
  {

  }

  void submenu_savestate()
  {
    print_string("Savestate options:", COLOR_ACTIVE_ITEM, COLOR_BG, 10, 70);
    menu_change_state();
  }

  void submenu_main()
  {
	  // TODO [Neb] Deal with this when creating the saved states menu
    // get_savestate_filename_noshot(savestate_slot,
    //  current_savestate_filename);
  }

  u8 *yes_no_options[] = { "no", "yes" };
  u8 *enable_disable_options[] = { "disabled", "enabled" };

  u8 *scale_options[] =
  {
    "normal", "fullscreen"
  };

  u8 *frameskip_options[] = { "automatic", "manual", "off" };
  u8 *frameskip_variation_options[] = { "uniform", "random" };

#if 0
  u8 *audio_buffer_options[] =
  {
    "3072 bytes", "4096 bytes", "5120 bystes", "6144 bytes", "7168 bytes",
    "8192 bytes", "9216 bytes", "10240 bytes", "11264 bytes", "12288 bytes"
  };

#endif
  u8 *update_backup_options[] = { "Exit only", "Automatic" };

#ifndef ZAURUS
  u8 *gamepad_config_buttons[] =
  {
    "UP",
    "DOWN",
    "LEFT",
    "RIGHT",
    "A",
    "B",
    "L",
    "R",
    "START",
    "SELECT",
    "MENU",
    "FASTFORWARD",
    "LOAD STATE",
    "SAVE STATE",
    "RAPIDFIRE A",
    "RAPIDFIRE B",
    "RAPIDFIRE L",
    "RAPIDFIRE R",
    "NOTHING"
  };
#endif

  // Marker for help information, don't go past this mark (except \n)------*
  menu_option_type graphics_sound_options[] =
  {
    string_selection_option(NULL, "Display scaling", scale_options,
     (u32 *)(&screen_scale), 2,
     "Determines how the GBA screen is resized in relation to the entire\n"
     "screen. Select unscaled 3:2 for GBA resolution, scaled 3:2 for GBA\n"
     "aspect ratio scaled to fill the height of the PSP screen, and\n"
     "fullscreen to fill the entire PSP screen.", 2),
#ifndef ZAURUS
    string_selection_option(NULL, "Screen filtering", yes_no_options,
     (u32 *)(&screen_filter), 2,
     "Determines whether or not bilinear filtering should be used when\n"
     "scaling the screen. Selecting this will produce a more even and\n"
     "smooth image, at the cost of being blurry and having less vibrant\n"
     "colors.", 3),
#endif
    string_selection_option(NULL, "Frameskip type", frameskip_options,
     (u32 *)(&current_frameskip_type), 3,
     "Determines what kind of frameskipping should be employed.\n"
     "Frameskipping may improve emulation speed of many games.\n"
     "Off: Do not skip any frames.\n"
     "Auto: Skip up to N frames (see next option) as needed.\n"
     "Manual: Always render only 1 out of N + 1 frames.", 5),
    numeric_selection_option(NULL, "Frameskip value", &frameskip_value, 100,
     "For auto frameskip, determines the maximum number of frames that\n"
     "are allowed to be skipped consecutively.\n"
     "For manual frameskip, determines the number of frames that will\n"
     "always be skipped.", 6),
    string_selection_option(NULL, "Framskip variation",
     frameskip_variation_options, &random_skip, 2,
     "If objects in the game flicker at a regular rate certain manual\n"
     "frameskip values may cause them to normally disappear. Change this\n"
     "value to 'random' to avoid this. Do not use otherwise, as it tends to\n"
     "make the image quality worse, especially in high motion games.", 7),
    string_selection_option(NULL, "Audio output", yes_no_options,
     &global_enable_audio, 2,
     "Select 'no' to turn off all audio output. This will not result in a\n"
     "significant change in performance.", 9),
#if 0
    string_selection_option(NULL, "Audio buffer", audio_buffer_options,
           &audio_buffer_size_number, 10,
     "Set the size (in bytes) of the audio buffer. Larger values may result\n"
     "in slightly better performance at the cost of latency; the lowest\n"
     "value will give the most responsive audio.\n"
     "This option requires gpSP to be restarted before it will take effect.",
     10),
#endif
	string_selection_option(NULL, "Status Display", enable_disable_options,
													&status_display, 2,
	"Display fps and some infomation.",12), 

    submenu_option(NULL, "Back", "Return to the main menu.", 14)
  };

  make_menu(graphics_sound, submenu_graphics_sound, NULL);

  menu_option_type cheats_misc_options[] =
  {
    cheat_option(0),
    cheat_option(1),
    cheat_option(2),
    cheat_option(3),
    cheat_option(4),
    cheat_option(5),
    cheat_option(6),
    cheat_option(7),
    cheat_option(8),
    cheat_option(9),
    string_selection_option(NULL, "Update backup",
     update_backup_options, &update_backup_flag, 2,
     "Determines when in-game save files should be written back to\n"
     "memstick. If set to 'automatic' writebacks will occur shortly after\n"
     "the game's backup is altered. On 'exit only' it will only be written\n"
     "back when you exit from this menu (NOT from using the home button).\n"
     "Use the latter with extreme care.", 12),
    submenu_option(NULL, "Back", "Return to the main menu.", 14)
  };

  make_menu(cheats_misc, submenu_cheats_misc, NULL);

  menu_option_type savestate_options[] =
  {
    numeric_selection_action_hide_option(menu_load_state, menu_change_state,
     "Load savestate from current slot", &savestate_slot, 10,
     "Select to load the game state from the current slot for this game.\n"
     "Press left + right to change the current slot.", 6),
    numeric_selection_action_hide_option(menu_save_state, menu_change_state,
     "Save savestate to current slot", &savestate_slot, 10,
     "Select to save the game state to the current slot for this game.\n"
     "Press left + right to change the current slot.", 7),
    numeric_selection_action_hide_option(menu_load_state_file,
      menu_change_state,
     "Load savestate from file", &savestate_slot, 10,
     "Restore gameplay from a savestate file.\n"
     "Note: The same file used to save the state must be present.\n", 9),
    numeric_selection_option(menu_change_state,
     "Current savestate slot", &savestate_slot, 10,
     "Change the current savestate slot.\n", 11),
    submenu_option(NULL, "Back", "Return to the main menu.", 13)
  };

  make_menu(savestate, submenu_savestate, NULL);

#ifndef ZAURUS
  menu_option_type gamepad_config_options[] =
  {
    gamepad_config_option("D-pad up     ", 0),
    gamepad_config_option("D-pad down   ", 1),
    gamepad_config_option("D-pad left   ", 2),
    gamepad_config_option("D-pad right  ", 3),
    gamepad_config_option("Circle       ", 4),
    gamepad_config_option("Cross        ", 5),
    gamepad_config_option("Square       ", 6),
    gamepad_config_option("Triangle     ", 7),
    gamepad_config_option("Left Trigger ", 8),
    gamepad_config_option("Right Trigger", 9),
    gamepad_config_option("Start        ", 10),
    gamepad_config_option("Select       ", 11),
    submenu_option(NULL, "Back", "Return to the main menu.", 13)
  };

  make_menu(gamepad_config, submenu_gamepad, NULL);

  menu_option_type analog_config_options[] =
  {
    analog_config_option("Analog up   ", 0),
    analog_config_option("Analog down ", 1),
    analog_config_option("Analog left ", 2),
    analog_config_option("Analog right", 3),
    string_selection_option(NULL, "Enable analog", yes_no_options,
     &global_enable_analog, 2,
     "Select 'no' to block analog input entirely.", 7),
    numeric_selection_option(NULL, "Analog sensitivity",
     &analog_sensitivity_level, 10,
     "Determine sensitivity/responsiveness of the analog input.\n"
     "Lower numbers are less sensitive.", 8),
    submenu_option(NULL, "Back", "Return to the main menu.", 11)
  };

  make_menu(analog_config, submenu_analog, NULL);
#endif

  menu_option_type main_options[] =
  {
    submenu_option(&graphics_sound_menu, "Graphics and Sound options",
     "Select to set display parameters and frameskip behavior,\n"
     "audio on/off, audio buffer size, and audio filtering.", 0),
    numeric_selection_action_option(menu_load_state, NULL,
     "Load state from slot", &savestate_slot, 10,
     "Select to load the game state from the current slot for this game,\n"
     "if it exists (see the extended menu for more information)\n"
     "Press left + right to change the current slot.", 2),
    numeric_selection_action_option(menu_save_state, NULL,
     "Save state to slot", &savestate_slot, 10,
     "Select to save the game state to the current slot for this game.\n"
     "See the extended menu for more information.\n"
     "Press left + right to change the current slot.", 3),
    submenu_option(&savestate_menu, "Savestate options",
     "Select to enter a menu for loading, saving, and viewing the\n"
     "currently active savestate for this game (or to load a savestate\n"
     "file from another game)", 4),
#ifndef ZAURUS
    submenu_option(&gamepad_config_menu, "Configure gamepad input",
     "Select to change the in-game behavior of the PSP buttons and d-pad.",
     6),
    submenu_option(&analog_config_menu, "Configure analog input",
     "Select to change the in-game behavior of the PSP analog nub.", 7),
#endif
    submenu_option(&cheats_misc_menu, "Cheats and Miscellaneous options",
     "Select to manage cheats, set backup behavior, and set device clock\n"
     "speed.", 9),
    action_option(menu_restart, NULL, "Restart game",
     "Select to reset the GBA with the current game loaded.", 11),
    action_option(menu_exit, NULL, "Return to game",
     "Select to exit this menu and resume gameplay.", 12),
    action_option(menu_quit, NULL, "Exit gpSP",
     "Select to exit gpSP and return to the PSP XMB/loader.", 14)
  };

  make_menu(main, submenu_main, NULL);

  void choose_menu(menu_type *new_menu)
  {
    if(new_menu == NULL)
      new_menu = &main_menu;

    clear_screen(COLOR_BG);

    blit_to_screen(original_screen, 240, 160, 230, 40);

    current_menu = new_menu;
    current_option = new_menu->options;
    current_option_num = 0;
    if(current_menu->init_function)
     current_menu->init_function();
  }

  void clear_help()
  {
#ifndef ZAURUS
    for(i = 0; i < 6; i++)
    {
      print_string_pad(" ", COLOR_BG, COLOR_BG, 30, 210 + (i * 10), 70);
    }
#endif
  }

  video_resolution_large();

  SDL_LockMutex(sound_mutex);
  SDL_PauseAudio(1);
  SDL_UnlockMutex(sound_mutex);

  if(gamepak_filename[0] == 0)
  {
    first_load = 1;
    memset(original_screen, 0x00, 240 * 160 * 2);
    print_string_ext("No game loaded yet.", 0xFFFF, 0x0000,
     60, 75,original_screen, 240, 0);
  }

  choose_menu(&main_menu);

  for(i = 0; i < 10; i++)
  {
    if(i >= num_cheats)
    {
      sprintf(cheat_format_str[i], "cheat %d (none loaded)", i);
    }
    else
    {
      sprintf(cheat_format_str[i], "cheat %d (%s): %%s", i,
       cheats[i].cheat_name);
    }
  }

  current_menu->init_function();

  while(repeat)
  {
    display_option = current_menu->options;

    for(i = 0; i < current_menu->num_options; i++, display_option++)
    {
      if(display_option->option_type & NUMBER_SELECTION_OPTION)
      {
        sprintf(line_buffer, display_option->display_string,
         *(display_option->current_option));
      }
      else

      if(display_option->option_type & STRING_SELECTION_OPTION && strchr(display_option->display_string, '%'))
      {
        sprintf(line_buffer, display_option->display_string,
         ((u32 *)display_option->options)[*(display_option->current_option)]);
      }
      else
      {
        strcpy(line_buffer, display_option->display_string);
      }

      if(display_option == current_option)
      {
        print_string_pad(line_buffer, COLOR_ACTIVE_ITEM, COLOR_BG, 10,
         (display_option->line_number * 10) + 40, 36);
      }
      else
      {
        print_string_pad(line_buffer, COLOR_INACTIVE_ITEM, COLOR_BG, 10,
         (display_option->line_number * 10) + 40, 36);
      }
    }
#ifndef ZAURUS
    print_string(current_option->help_string, COLOR_HELP_TEXT,
     COLOR_BG, 30, 210);
#endif
    flip_screen();

    gui_action = get_gui_input();

    switch(gui_action)
    {
      case CURSOR_DOWN:
        current_option_num = (current_option_num + 1) %
          current_menu->num_options;

        current_option = current_menu->options + current_option_num;
        clear_help();
        break;

      case CURSOR_UP:
        if(current_option_num)
          current_option_num--;
        else
          current_option_num = current_menu->num_options - 1;

        current_option = current_menu->options + current_option_num;
        clear_help();
        break;

      case CURSOR_RIGHT:
        if(current_option->option_type & (NUMBER_SELECTION_OPTION |
         STRING_SELECTION_OPTION))
        {
          *(current_option->current_option) =
           (*current_option->current_option + 1) %
           current_option->num_options;

          if(current_option->passive_function)
            current_option->passive_function();
        }
        break;

      case CURSOR_LEFT:
        if(current_option->option_type & (NUMBER_SELECTION_OPTION |
         STRING_SELECTION_OPTION))
        {
          u32 current_option_val = *(current_option->current_option);

          if(current_option_val)
            current_option_val--;
          else
            current_option_val = current_option->num_options - 1;

          *(current_option->current_option) = current_option_val;

          if(current_option->passive_function)
            current_option->passive_function();
        }
        break;

      case CURSOR_EXIT:
        if(current_menu == &main_menu)
          menu_exit();

        choose_menu(&main_menu);
        break;

      case CURSOR_SELECT:
        if(current_option->option_type & ACTION_OPTION)
          current_option->action_function();

        if(current_option->option_type & SUBMENU_OPTION)
          choose_menu(current_option->sub_menu);
        break;
    }
  }

  set_gba_resolution(screen_scale);
  video_resolution_small();

  global_cycles_per_instruction = clock_speed_number + 1;

  SDL_PauseAudio(0);

  clear_screen(0);
  flip_screen();
  flip_screen();

  return return_value;
}
