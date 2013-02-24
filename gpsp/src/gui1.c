/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
 *
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

/******************************************************************************
 * gui.c
 * GUI処理
 ******************************************************************************/

/******************************************************************************
 * 头文件
 ******************************************************************************/
#include "common.h"

/******************************************************************************
 * 宏定义
 ******************************************************************************/
#define STATUS_ROWS 0
#define CURRENT_DIR_ROWS 2
//#define FILE_LIST_ROWS 25
#define FILE_LIST_ROWS 5
#define FILE_LIST_ROWS_CENTER   (FILE_LIST_ROWS/2)
#define FILE_LIST_POSITION 50
//#define DIR_LIST_POSITION 360
#define DIR_LIST_POSITION 130
#define PAGE_SCROLL_NUM 5

#define FILE_BACKGROUND "mmc:\\GPSP\\GUI\\filebg.bmp"
#define MENU_BACKGROUND "mmc:\\GPSP\\GUI\\menubg.bmp"
#define SUBM_BACKGROUND "mmc:\\GPSP\\GUI\\submbg.bmp"

#define GPSP_CONFIG_FILENAME "sys.cfg"

// gpSP的一套头文件
// 标题 4字节
#define GPSP_CONFIG_HEADER     0xC33C5AA5
#define GPSP_CONFIG_HEADER_U32 0xC33C5AA5
const u32 gpsp_config_ver = 0x00010000;
GPSP_CONFIG gpsp_config;

// GAME的一套头文件
// 标题 4字节
#define GAME_CONFIG_HEADER     "gcfg"
#define GAME_CONFIG_HEADER_U32 0x67666367
const u32 game_config_ver = 0x00010000;
GAME_CONFIG game_config;

#ifdef TEST_MODE
#define VER_RELEASE "test"
#else
#define VER_RELEASE "release"
#endif

#define MAKE_MENU(name, init_function, passive_function)                      \
  MENU_TYPE name##_menu =                                                     \
  {                                                                           \
    init_function,                                                            \
    passive_function,                                                         \
    name##_options,                                                           \
    sizeof(name##_options) / sizeof(MENU_OPTION_TYPE)                         \
  }                                                                           \

#define GAMEPAD_CONFIG_OPTION(display_string, number)                         \
{                                                                             \
  NULL,                                                                       \
  menu_fix_gamepad_help,                                                      \
  NULL,                                                                       \
  display_string,                                                             \
  gamepad_config_buttons,                                                     \
  gamepad_config_map + gamepad_config_line_to_button[number],                 \
  sizeof(gamepad_config_buttons) / sizeof(gamepad_config_buttons[0]),         \
  gamepad_help[gamepad_config_map[                                            \
   gamepad_config_line_to_button[number]]],                                   \
  number,                                                                     \
  STRING_SELECTION_TYPE                                                       \
}                                                                             \

#define ANALOG_CONFIG_OPTION(display_string, number)                          \
{                                                                             \
  NULL,                                                                       \
  menu_fix_gamepad_help,                                                      \
  NULL,                                                                       \
  display_string,                                                             \
  gamepad_config_buttons,                                                     \
  gamepad_config_map + number + 12,                                           \
  sizeof(gamepad_config_buttons) / sizeof(gamepad_config_buttons[0]),         \
  gamepad_help[gamepad_config_map[number + 12]],                              \
  number + 2,                                                                 \
  STRING_SELECTION_TYPE                                                     \
}                                                                             \

#define CHEAT_OPTION(number, line_number)                                     \
{                                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  NULL,                                                                       \
  cheat_format_str[number],                                                   \
  enable_disable_options,                                                     \
  &(game_config.cheats_flag[number].cheat_active),                            \
  2,                                                                          \
  msg[MSG_CHEAT_MENU_HELP_0],                                                 \
  line_number,                                                              \
  STRING_SELECTION_TYPE                                                     \
}                                                                             \

#define ACTION_OPTION(action_function, passive_function, display_string,      \
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
  ACTION_TYPE                                                               \
}                                                                             \

#define SUBMENU_OPTION(sub_menu, display_string, help_string, line_number)    \
{                                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  sub_menu,                                                                   \
  display_string,                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  sizeof(sub_menu) / sizeof(MENU_OPTION_TYPE),                                \
  help_string,                                                                \
  line_number,                                                                \
  SUBMENU_TYPE                                                              \
}                                                                             \

#define SELECTION_OPTION(passive_function, display_string, options,           \
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

#define ACTION_SELECTION_OPTION(action_function, passive_function,            \
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
  type | ACTION_TYPE                                                        \
}                                                                             \

#define STRING_SELECTION_OPTION(action_function, passive_function,            \
    display_string, options, option_ptr, num_options, help_string, line_number)\
  ACTION_SELECTION_OPTION(action_function, passive_function, display_string,   \
    options, option_ptr, num_options, help_string, line_number, STRING_SELECTION_TYPE)  \

#define NUMERIC_SELECTION_OPTION(passive_function, display_string,            \
 option_ptr, num_options, help_string, line_number)                           \
  SELECTION_OPTION(passive_function, display_string, NULL, option_ptr,        \
   num_options, help_string, line_number, NUMBER_SELECTION_TYPE)              \

#define STRING_SELECTION_HIDEN_OPTION(action_function, passive_function,      \
 display_string, options, option_ptr, num_options, help_string, line_number)  \
  ACTION_SELECTION_OPTION(action_function, passive_function,                  \
   display_string,  options, option_ptr, num_options, help_string,            \
   line_number, (STRING_SELECTION_TYPE | HIDEN_TYPE))                         \

#define NUMERIC_SELECTION_ACTION_OPTION(action_function, passive_function,    \
 display_string, option_ptr, num_options, help_string, line_number)           \
  ACTION_SELECTION_OPTION(action_function, passive_function,                  \
   display_string,  NULL, option_ptr, num_options, help_string,               \
   line_number, NUMBER_SELECTION_TYPE)                                        \

#define NUMERIC_SELECTION_HIDE_OPTION(action_function, passive_function,      \
    display_string, option_ptr, num_options, help_string, line_number)        \
  ACTION_SELECTION_OPTION(action_function, passive_function,                  \
   display_string, NULL, option_ptr, num_options, help_string,                \
   line_number, NUMBER_SELECTION_TYPE)                                        \

#define GAMEPAD_MENU_WIDTH 15

typedef enum
{
  NUMBER_SELECTION_TYPE = 0x01,
  STRING_SELECTION_TYPE = 0x02,
  SUBMENU_TYPE          = 0x04,
  ACTION_TYPE           = 0x08,
  HIDEN_TYPE            = 0x10
} MENU_OPTION_TYPE_ENUM;

struct _MENU_OPTION_TYPE
{
  void (* action_function)();
  void (* passive_function)();
  struct _MENU_TYPE *sub_menu;
  char **display_string;
  void *options;
  u32 *current_option;
  u32 num_options;
  char **help_string;
  u32 line_number;
  MENU_OPTION_TYPE_ENUM option_type;
};

struct _MENU_TYPE
{
  void (* init_function)();
  void (* passive_function)();
  struct _MENU_OPTION_TYPE *options;
  u32 num_options;
};

typedef struct _MENU_OPTION_TYPE MENU_OPTION_TYPE;
typedef struct _MENU_TYPE MENU_TYPE;


typedef enum
{
  MAIN_MENU,
  GAMEPAD_MENU,
  SAVESTATE_MENU,
  FRAMESKIP_MENU,
  CHEAT_MENU,
  ADHOC_MENU,
  MISC_MENU
} MENU_ENUM;

/******************************************************************************
 * 定义全局变量
 ******************************************************************************/
char g_default_rom_dir[MAX_PATH];
char DEFAULT_SAVE_DIR[MAX_PATH];
char DEFAULT_CFG_DIR[MAX_PATH];
char DEFAULT_SS_DIR[MAX_PATH];
char DEFAULT_CHEAT_DIR[MAX_PATH];
u32 SAVESTATE_SLOT = 0;

/******************************************************************************
 * 局部变量的定义
 ******************************************************************************/
static char m_font[MAX_PATH];
static char s_font[MAX_PATH];
static u32 menu_cheat_page = 0;
static u32 gamepad_config_line_to_button[] = { 8, 6, 7, 9, 1, 2, 3, 0, 4, 5, 11, 10 };

/******************************************************************************
 * 本地函数的声明
 ******************************************************************************/
static void get_savestate_snapshot(char *savestate_filename, u32 slot_num);
static void get_savestate_filename(u32 slot, char *name_buffer);
static int sort_function(const void *dest_str_ptr, const void *src_str_ptr);
static u32 parse_line(char *current_line, char *current_str);
static void print_status(u32 mode);
static void get_timestamp_string(char *buffer, u16 msg_id, u16 year, u16 mon, u16 day, u16 wday, u16 hour, u16 min, u16 sec, u32 msec);
static void save_ss_bmp(u16 *image);
void _flush_cache();
void button_up_wait();
static void init_default_gpsp_config();

static int sort_function(const void *dest_str_ptr, const void *src_str_ptr)
{
  char *dest_str = *((char **)dest_str_ptr);
  char *src_str = *((char **)src_str_ptr);

  if(src_str[0] == '.')
    return 1;

  if(dest_str[0] == '.')
    return -1;

  return strcasecmp(dest_str, src_str);
}

static void strupr(char *str)
{
    while(*str)
    {
        if(*str <= 0x7A && *str >= 0x61) *str -= 0x20;
        str++;
    }
}
/******************************************************************************
 * 全局函数定义
 ******************************************************************************/
#define FS_FAT_ATTR_DIRECTORY   0x10
#define FILE_LIST_MAX  256
#define DIR_LIST_MAX   64
#define NAME_MEM_SIZE  256*256
/*--------------------------------------------------------
  通用文件阅读
--------------------------------------------------------*/
s32 load_file(char **wildcards, char *result,char *default_dir_name)
{
  DIR *current_dir;
  struct dirent *current_file;
//  struct stat file_info;
  char current_dir_name[MAX_PATH];
  char current_dir_name_cpy[MAX_PATH];
//  u32 total_filenames_allocated;
//  u32 total_dirnames_allocated;
//  char **file_list;
//  char **dir_list;
  u32 num_files;                      // 当前目录的文件数量
  u32 num_dirs;                       // 当前目录的文件夹数量
  char *file_name;
  u32 file_name_length;
//  s32 ext_pos = -1;
  char *ext_pos;
  u32 chosen_file, chosen_dir;
  s32 return_value = 1;
  u32 repeat;
    u32 current_selection_item;
    u32 current_in_scroll_value;
    u32 current_file_in_scroll;
    u32 current_dir_in_scroll;
    u32 i, j, k;
  gui_action_type gui_action;
  char utf8[1024];
  char name_mem[NAME_MEM_SIZE];
  char* file_list[FILE_LIST_MAX];
  char* dir_list[DIR_LIST_MAX];
  char* name_mem_top;
  char* name_mem_base;
  u16 *screenp;

    screenp= (u16*)malloc(256*192*2);
    if(screenp == NULL)
    {
        screenp = screen_address;
    printf("screenp == NULL\n");
    }


  name_mem_top = &name_mem[NAME_MEM_SIZE-1];

  strcpy(current_dir_name, default_dir_name);
  strcpy(current_dir_name_cpy, default_dir_name);

  while(return_value == 1)
  {
//    total_filenames_allocated = 32;
//    total_dirnames_allocated = 32;
//    file_list = (char **)malloc(sizeof(char *) * 32);
//    dir_list = (char **)malloc(sizeof(char *) * 32);
//    memset(file_list, 0, sizeof(char *) * 32);
//    memset(dir_list, 0, sizeof(char *) * 32);
    name_mem_base = &name_mem[0];
    num_files = 0;
    num_dirs = 0;
    chosen_file = 0;
    chosen_dir = 0;

    current_dir = opendir(current_dir_name_cpy);

    do
    {
      if(current_dir)
        current_file = readdir(current_dir);
      else
        break;

      if(current_file)
      {
        file_name = current_file->d_name;
        file_name_length = strlen(file_name);
        
        if((name_mem_base + file_name_length) > name_mem_top)   //no enough memory
          break;

//        if((stat(file_name, &file_info) >= 0) &&
//         ((file_name[0] != '.') || (file_name[1] == '.')))
//        if(((file_name[0] != '.') || (file_name[1] == '.')))
        if(file_name[0] != '.')
        {
//          if(S_ISDIR(file_info.st_mode))
          if((current_file -> FAT_DirAttr) & FS_FAT_ATTR_DIRECTORY)     //Is directory
          {
//            dir_list[num_dirs] = (char *)malloc(file_name_length + 1);
//             sprintf(dir_list[num_dirs], "%s", file_name);
              dir_list[num_dirs] = name_mem_base;
              sprintf(dir_list[num_dirs], "%s", file_name);
              num_dirs++;
              name_mem_base += file_name_length + 1;
          }
          else
          {
            // Must match one of the wildcards, also ignore the .
//            if(file_name_length >= 4)
//            {
//              if(file_name[file_name_length - 4] == '.')
//                ext_pos = file_name_length - 4;
//              else if(file_name[file_name_length - 3] == '.')
//                ext_pos = file_name_length - 3;
//              else
//                ext_pos = 0;
              ext_pos = strrchr(file_name, '.');

              for(i = 0; wildcards[i] != NULL; i++)
              {
//                if(!strcasecmp((file_name + ext_pos), wildcards[i]))
                if(!strcasecmp(ext_pos, wildcards[i]))
                {
//                    file_list[num_files] = (char *)malloc(file_name_length + 1);
                    file_list[num_files] = name_mem_base;
                    sprintf(file_list[num_files], "%s", file_name);
                    num_files++;
                    name_mem_base += file_name_length + 1;
                  break;
                }
              }
//            }
          }
        }

//        if(num_files == total_filenames_allocated)
//        {
//          file_list = (char **)realloc(file_list, sizeof(char *) *
//           total_filenames_allocated * 2);
//          memset(file_list + total_filenames_allocated, 0,
//           sizeof(u8 *) * total_filenames_allocated);
//          total_filenames_allocated *= 2;
//        }

//        if(num_dirs == total_dirnames_allocated)
//        {
//          dir_list = (char **)realloc(dir_list, sizeof(char *) *
//           total_dirnames_allocated * 2);
//          memset(dir_list + total_dirnames_allocated, 0,
//           sizeof(char *) * total_dirnames_allocated);
//          total_dirnames_allocated *= 2;
//        }
        if((num_dirs >= DIR_LIST_MAX) || (num_files >= FILE_LIST_MAX))
          break;
      }
    } while(current_file);

    qsort((void *)file_list, num_files, sizeof(u8 *), sort_function);
    qsort((void *)dir_list, num_dirs, sizeof(u8 *), sort_function);
    closedir(current_dir);

    current_selection_item = 0;
    current_in_scroll_value= 0;
    repeat = 1;

    u32 last_in_scroll_value= -1;
    u32 last_selection_item = -1;
    u32 list_string_num= 0;
    u32 dir_changed= 1;
    u32 shift_flag[FILE_LIST_ROWS];

    while(repeat)
    {
      gui_action = get_gui_input();

      switch(gui_action)
      {
        case CURSOR_UP:
            if(current_in_scroll_value > 0)
            {
                i= current_in_scroll_value + FILE_LIST_ROWS_CENTER;
                if(i == current_selection_item)
                {
                    current_in_scroll_value--;
                    current_selection_item--;
                }
                else if(i < current_selection_item)
                {
                    current_selection_item--;
                }
                else
                {
                    current_in_scroll_value--;
                }
            }
            else if(current_selection_item > 0)
                current_selection_item--;
          break;

        case CURSOR_DOWN:
            if(current_selection_item + 1 < num_files + num_dirs)
            {
                current_selection_item++;
            }
            else
            {
                if(current_in_scroll_value < current_selection_item)
                    current_in_scroll_value++;
            }
        
            if((current_in_scroll_value + FILE_LIST_ROWS +1) <= num_files + num_dirs)
            {
                if((current_in_scroll_value + FILE_LIST_ROWS_CENTER) < current_selection_item)
                    current_in_scroll_value++;
            }
          break;

        case CURSOR_RTRIGGER:
            if((current_in_scroll_value + FILE_LIST_ROWS) < (num_files + num_dirs))
            {
                current_in_scroll_value += FILE_LIST_ROWS - 1;
                current_selection_item += FILE_LIST_ROWS - 1;
                if(current_selection_item > (num_files + num_dirs -1))
                    current_selection_item = (num_files + num_dirs -1);
            }
          break;

        case CURSOR_LTRIGGER:
            if(current_in_scroll_value >= (FILE_LIST_ROWS - 1))
            {
                current_selection_item -= FILE_LIST_ROWS - 1;
                current_in_scroll_value -= FILE_LIST_ROWS - 1;
            }
            else
            {   
                current_selection_item -= current_in_scroll_value;
                current_in_scroll_value = 0;
            }
          break;

        case CURSOR_RIGHT:
            draw_hscroll(current_selection_item - current_in_scroll_value +1, -3);
          break;

        case CURSOR_LEFT:
            draw_hscroll(current_selection_item - current_in_scroll_value +1, 3);
          break;

        case CURSOR_SELECT:
            //file selected
            if(current_selection_item + 1 < num_files)
            {
                if(num_files != 0)
                {
                  repeat = 0;
                  return_value = 0;
                  strcpy(result, file_list[current_selection_item]);
                  // 获取 ROM 文件的路径
//<---              getcwd(rom_path, MAX_PATH);
                  strcpy(rom_path, current_dir_name);
                  strcpy(default_dir_name, current_dir_name);
                }
            }
            else if(num_dirs > 0) //dir selected
            {
                u32 current_dir_selection;

                repeat = 0;
                current_dir_selection= current_selection_item -num_files;
                if(current_dir)//Only last open dir not failure
                {
                    strcat(current_dir_name, "\\");
                    strupr(dir_list[current_dir_selection]);
                    strcat(current_dir_name, dir_list[current_dir_selection]);
                }
                strcpy(current_dir_name_cpy, current_dir_name);
                dir_changed= 1;
            }
          break;

        case CURSOR_BACK:
            if(strcmp(current_dir_name, "mmc:"))
            {
                repeat = 0;
//              chdir("..");
                ext_pos = strrchr(current_dir_name, '\\');
                *ext_pos = 0;
                strcpy(current_dir_name_cpy, current_dir_name);
            }
          break;

        case CURSOR_EXIT:
          return_value = -1;
          repeat = 0;
          break;

        default:
          break;
      }

    if(current_in_scroll_value != last_in_scroll_value || current_selection_item != last_selection_item)
    {
        for(i= 0; i < (FILE_LIST_ROWS +1); i++)
        {
            draw_hscroll_over(i);
            shift_flag[i]= 0;
        }

//      print_status(1);
        show_background(screenp, FILE_BACKGROUND);
        memcpy(screen_address, screenp, 256*192*2);
        show_icon(screen_address, ICON_FILEPATH, 18, 34);

//        if(dir_changed)
//        {
            //file path
//            sjis_to_cut(text, current_dir_name, 50);
//            PRINT_STRING_BG_UTF8(text, COLOR_ACTIVE_ITEM, COLOR_TRANS, 30, 32);
            draw_hscroll_init(screen_address, screenp, 30, 32, 180, COLOR_TRANS, 
                COLOR_ACTIVE_ITEM, current_dir_name);
            shift_flag[0]= 1;
            dir_changed= 0;
//        }

        sprintf(utf8, "%d/%d", current_selection_item +(num_files+num_dirs ? 1:0), num_files+num_dirs);
//        PRINT_STRING_BG(utf8, COLOR_ACTIVE_ITEM, COLOR_TRANS, 222, 37);
        BDF_render_string(screen_address, 222, 37, COLOR_TRANS, COLOR_ACTIVE_ITEM, utf8);

//      scrollbar(2, 32, 7, 167, num_files, FILE_LIST_ROWS, current_file_scroll_value);
        show_Vscrollbar(screen_address, 0, 0, current_selection_item, num_files+num_dirs);

        j= 0;
        k= current_in_scroll_value;
        i = num_files + num_dirs - k;
        if(i > FILE_LIST_ROWS)      i= FILE_LIST_ROWS;
        list_string_num= i +1;

        for( ; i> 0; i--)
        {
            u16 color;

            if(k == current_selection_item)
            {
                show_icon((char*)screen_address, ICON_FILE_SELECT_BG, 29, 51 + j*24);
                show_icon((char*)screen_address, ICON_MENUINDEX, 17, 57 + j*24);
                color= COLOR_ACTIVE_ITEM;
            }
            else
                color= COLOR_INACTIVE_ITEM;

            if(k < num_files)
            {
//                PRINT_STRING_BG_UTF8(file_list[k], color, COLOR_TRANS, FILE_LIST_POSITION, 
//                    56 + j*24);

                draw_hscroll_init(screen_address, screenp, FILE_LIST_POSITION, 56 + j*24, 180, 
                    COLOR_TRANS, color, file_list[k]);

                ext_pos= strrchr(file_list[k], '.');
                if(!strcasecmp(ext_pos, ".gba"))
                    show_icon((char*)screen_address, ICON_GBAFILE, 29, 53 + j*24);
                if(!strcasecmp(ext_pos, ".zip"))
                    show_icon((char*)screen_address, ICON_ZIPFILE, 29, 53 + j*24);
            }
            else
            {
//                PRINT_STRING_BG_UTF8(dir_list[k-num_files], color, COLOR_TRANS, 
//                    FILE_LIST_POSITION, 56 + j*24);

                draw_hscroll_init(screen_address, screenp, FILE_LIST_POSITION, 56 + j*24, 180, 
                    COLOR_TRANS, color, dir_list[k-num_files]);

                show_icon((char*)screen_address, ICON_DIRECTORY, 29, 53 + j*24);
            }

            show_icon((char*)screen_address, ICON_FILE_ISOLATOR, 17, 72 + j*24);
            k++, j++;
            shift_flag[j]= 1;
        }

        last_in_scroll_value = current_in_scroll_value;
        last_selection_item = current_selection_item;

        flip_screen();
    }
    else
    {
        s32 ret;
        static int path_scroll= 20;

        if(shift_flag[i])
        {
            if(path_scroll > 0) path_scroll -= 1;
            else
            {
                ret= draw_hscroll(i, -1);
                if(ret <= 0)    {shift_flag[i]= 0,  path_scroll= 20;}
//                flip_screen();
            }
        }
        else
        {
            if(path_scroll > 0) path_scroll -= 1;
            else
            {
                ret= draw_hscroll(i, 1);
                if(ret <= 0)    {shift_flag[i]= 1,  path_scroll= 20;}
//                flip_screen();
            }
        }

/*
        for(i= 0; i < list_string_num; i++)
        {
            if(shift_flag[i])
            {
                ret= draw_hscroll(i, -2);
                if(ret <= 0) shift_flag[i]= 0;
            }
            else
            {
                ret= draw_hscroll(i, 1024);
                if(ret <= 0) shift_flag[i]= 1;
            }
        }
*/
    }

        flip_screen();
        OSTimeDly(5);     //about 50ms
//      clear_screen(COLOR_BLACK);
    }
 }

  if(screenp) free((int)screenp);

  clear_screen(COLOR_BG);
  flip_screen();
  OSTimeDly(10);     //about 100ms
  clear_gba_screen(COLOR_BLACK);
  flip_gba_screen();
  OSTimeDly(10);     //about 100ms
  clear_gba_screen(COLOR_BLACK);
  flip_gba_screen();
  return return_value;
}

//标准按键
const u32 gamepad_config_map_init[MAX_GAMEPAD_CONFIG_MAP] =
{
    BUTTON_ID_A,        /* 0    A */
    BUTTON_ID_B,        /* 1    B */
    BUTTON_ID_SELECT,   /* 2    [SELECT] */
    BUTTON_ID_START,    /* 3    [START] */
    BUTTON_ID_RIGHT,    /* 4    → */
    BUTTON_ID_LEFT,     /* 5    ← */
    BUTTON_ID_UP,       /* 6    ↑ */
    BUTTON_ID_DOWN,     /* 7    ↓ */
    BUTTON_ID_R,        /* 8    [R] */
    BUTTON_ID_L,        /* 9    [L] */
    BUTTON_ID_NONE,     /* 10   FA */
    BUTTON_ID_NONE,     /* 11   FB */
    BUTTON_ID_X,        /* 12   MENU */
    BUTTON_ID_NONE,     /* 13   NONE */
    BUTTON_ID_NONE,     /* 14   NONE */
    BUTTON_ID_NONE      /* 15   NONE */
};

#define BUTTON_MAP_A    gamepad_config_map[0]
#define BUTTON_MAP_B    gamepad_config_map[1]
#define BUTTON_MAP_FA   gamepad_config_map[10]
#define BUTTON_MAP_FB   gamepad_config_map[11]
#define BUTTON_MAP_MENU gamepad_config_map[12]

/* △ ○ × □ ↓ ← ↑ → */

/*--------------------------------------------------------
  game cfg的初始化
--------------------------------------------------------*/
void init_game_config()
{
  u32 i;
  game_config.frameskip_type = auto_frameskip;
  game_config.frameskip_value = 9;
  game_config.random_skip = NO;
  game_config.clock_speed_number = 9;
  game_config.audio_buffer_size_number = 15;
  game_config.update_backup_flag = NO;
  for(i = 0; i < MAX_CHEATS; i++)
  {
    game_config.cheats_flag[i].cheat_active = NO;
    game_config.cheats_flag[i].cheat_name[0] = '\0';
  }
  memcpy(game_config.gamepad_config_map, gamepad_config_map_init, sizeof(gamepad_config_map_init));
  game_config.use_default_gamepad_map = YES;
    game_config.gamepad_config_home = BUTTON_ID_X;
  game_config.allocate_sensor = NO;
    game_config.savestate_index= 0;
    game_config.savestate_map= 2;
}

/*--------------------------------------------------------
  gpSP cfg的初始化
--------------------------------------------------------*/
void init_default_gpsp_config()
{
//  int temp;
  gpsp_config.screen_scale = scaled_aspect;
  gpsp_config.screen_filter = filter_bilinear;
  gpsp_config.screen_ratio = R4_3;
  gpsp_config.screen_interlace = PROGRESSIVE;
  gpsp_config.enable_audio = YES;
  gpsp_config.enable_analog = YES;
  gpsp_config.analog_sensitivity_level = 4;
  gpsp_config.enable_home = NO;
  //keypad configure
  memcpy(gpsp_config.gamepad_config_map, gamepad_config_map_init, sizeof(gamepad_config_map_init));
  memcpy(gamepad_config_map, gpsp_config.gamepad_config_map, sizeof(gpsp_config.gamepad_config_map));
  gpsp_config.language = 0;     //defalut language= English
#if 1
  gpsp_config.emulate_core = ASM_CORE;
#else
  gpsp_config.emulate_core = C_CORE;
#endif
    gpsp_config.gamepad_config_home = BUTTON_ID_X;
  gpsp_config.debug_flag = NO;
  gpsp_config.fake_fat = NO;
  gpsp_config.rom_file[0]= 0;
  gpsp_config.rom_path[0]= 0;
  strcpy(gpsp_config.m_font, "knj10.fbm");
  strcpy(gpsp_config.s_font, "5x10rk.fbm");
}

/*--------------------------------------------------------
  game cfgファイルの読込み
  3/4修正
--------------------------------------------------------*/
void load_game_config_file(void)
{
  char game_config_filename[MAX_FILE];
  char game_config_path[MAX_PATH];
  FILE_ID game_config_file;
  u32 header, ver;

  // 设置初始值
  init_game_config();

  change_ext(gamepak_filename, game_config_filename, ".cfg");

  sprintf(game_config_path, "%s\\%s", DEFAULT_CFG_DIR, game_config_filename);
  FILE_OPEN(game_config_file, game_config_path, READ);

  if(FILE_CHECK_VALID(game_config_file))
  {
    // 头检查
    FILE_READ_VARIABLE(game_config_file, header);
    if(header == GAME_CONFIG_HEADER_U32)
    {
      // 版本检查
      FILE_READ_VARIABLE(game_config_file, ver);
      switch(ver)
      {
        case 0x10000: /* 1.0 */
          FILE_READ_VARIABLE(game_config_file, game_config);
          if(game_config.use_default_gamepad_map == YES)
            memcpy(gamepad_config_map, gpsp_config.gamepad_config_map, sizeof(gpsp_config.gamepad_config_map));
          else
            memcpy(gamepad_config_map, game_config.gamepad_config_map, sizeof(game_config.gamepad_config_map));
          break;
      }
    }
  }
  FILE_CLOSE(game_config_file);
}

/*--------------------------------------------------------
  gpSP cfg配置文件
--------------------------------------------------------*/
s32 load_config_file()
{
  char gpsp_config_path[MAX_PATH];
  FILE_ID gpsp_config_file;
  u32 header, ver;

  sprintf(gpsp_config_path, "%s\\%s", main_path, GPSP_CONFIG_FILENAME);
  FILE_OPEN(gpsp_config_file, gpsp_config_path, READ);

  if(FILE_CHECK_VALID(gpsp_config_file))
  {
    // 检查配置文件的头
    FILE_READ_VARIABLE(gpsp_config_file, header);
    if(header != GPSP_CONFIG_HEADER_U32)
    {
      FILE_CLOSE(gpsp_config_file);
      // 如果无法加载初始的一套值
      init_default_gpsp_config();
      return -1;
    }
    FILE_READ_VARIABLE(gpsp_config_file, ver);
    switch(ver)
    {
      case 0x10000: /* 1.0 */
        FILE_READ_VARIABLE(gpsp_config_file, gpsp_config);
        break;
    }
    FILE_CLOSE(gpsp_config_file);
    memcpy(gamepad_config_map, gpsp_config.gamepad_config_map, sizeof(gpsp_config.gamepad_config_map));

    return 0;
  }
  // 如果无法加载初始的一套值
  init_default_gpsp_config();
  return -1;
}

void initial_gpsp_config()
{
    //Initial directory path
    sprintf(g_default_rom_dir, "%s\\ROM", main_path);
    sprintf(DEFAULT_SAVE_DIR, "%s\\SAV", main_path);
    sprintf(DEFAULT_CFG_DIR, "%s\\CFG", main_path);
    sprintf(DEFAULT_SS_DIR, "%s\\SS", main_path);    
    sprintf(DEFAULT_CHEAT_DIR, "%s\\CHT", main_path);
    //Initial font path
    sprintf(m_font, "%s\\FONT\\%s", main_path, gpsp_config.m_font);
    sprintf(s_font, "%s\\FONT\\%s", main_path, gpsp_config.s_font);
}

/*--------------------------------------------------------
  メニューの表示
--------------------------------------------------------*/
u32 menu(u16 *original_screen)
{
    gui_action_type gui_action;
    u32 i;
    u32 repeat = 1;
    u32 return_value = 0;
    u32 first_load = 0;
    char current_savestate_filename[MAX_FILE];
    char line_buffer[256];
    char cheat_format_str[MAX_CHEATS][41*4];

    MENU_TYPE *current_menu;
    MENU_OPTION_TYPE *current_option;
    MENU_OPTION_TYPE *display_option;
    
    u32 current_option_num;
    u32 parent_option_num;
    u32 string_select;

  auto void choose_menu();
  auto void menu_return();
  auto void menu_exit();
  auto void menu_load();
  auto void menu_restart();
  auto void menu_save_ss();
  auto void menu_change_state();
  auto void menu_save_state();
    auto void menu_savestate_passive();
  auto void menu_load_state();
  auto void menu_load_state_file();
  auto void menu_load_cheat_file();
  auto void menu_fix_gamepad_help();
  auto void submenu_graphics();
  auto void submenu_audio_switch();
  auto void submenu_cheats();
    auto void submenu_tools();
    auto void submenu_screensnap();
    auto void submenu_others();
    auto void submenu_load_game();
  auto void submenu_gamepad();
    auto void keyremap_show();
  auto void submenu_analog();
  auto void submenu_game_state();
  auto void submenu_main();
    auto void main_menu_passive();
  auto void reload_cheats_page();
  auto void home_mode();
//  auto void set_gamepad();
    auto void delete_savestate();
    auto void screen_snap();
    auto void keyremap();
    auto void default_setting();
    auto void load_lastest_played();
    auto void language_set();

  //ローカル関数の定義
  void menu_return()
  {
    if(!first_load)
      repeat = 0;
  }

  void menu_exit()
  {
    quit(0);
  }

  void menu_load()
  {
    char *file_ext[] = { ".gba", ".bin", ".zip", NULL };
    char load_filename[MAX_FILE];

//    save_config_file();

//    if(!gpsp_config.update_backup_flag) // TODO タイミングを検討
//      update_backup_force();

    if(load_file(file_ext, load_filename, g_default_rom_dir) != -1)
    {
       if(load_gamepak(load_filename) == -1)
       {
         quit(0);
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

  void menu_save_ss()
  {
    if(!first_load)
      save_ss_bmp(original_screen);
  }

  void menu_change_state()
  {
    get_savestate_filename(SAVESTATE_SLOT, current_savestate_filename);
  }

  void menu_save_state()
  {
    if(gui_action == CURSOR_SELECT)
    {
printf("In save state\n");
        //The slot is empty
        if(!first_load)
        {
            u32 key;
            u16 *screenp;
            u32 color_bg;
            screenp = (u16*)malloc(256*192*2);
            if(screenp != NULL)
            {
                color_bg = COLOR16(32, 0, 8);
                memcpy(screenp, screen_address, 256*192*2);
            }
            else
                color_bg = COLOR_BG;

            if(game_config.savestate_map & (1<<game_config.savestate_index))
            {
                key= draw_message(screen_address, screenp, 28, 75, 227, 149, msg[MSG_SAVESTATE_FULL], 
                    color_bg, COLOR_WHITE, 2);
                if(key== 2)
                if(screenp != NULL)
                {
                    free((int)screenp);
                    return;
                }
            }

            get_savestate_filename(game_config.savestate_index, current_savestate_filename);
            draw_message(screen_address, screenp, 28, 22, 227, 171, msg[MSG_SAVESTATE_DOING], 
                color_bg, COLOR_WHITE, 0);
while(1);
            if(save_state(current_savestate_filename, original_screen) < 1)
                draw_message(screen_address, screenp, 80, 100, 180, 150, msg[MSG_SAVESTATE_FAILUER], 
                    color_bg, COLOR_WHITE, 0);
            else
                draw_message(screen_address, screenp, 80, 100, 180, 150, msg[MSG_SAVESTATE_SUCCESS], 
                    color_bg, COLOR_WHITE, 0);
        }
printf("save state over\n");
    }
  }

  void menu_load_state()
  {
    if(gui_action == CURSOR_SELECT)
    {
printf("In load state\n");
    menu_change_state();
    if(!first_load)
    {
      if (load_state(current_savestate_filename, SAVESTATE_SLOT) == 1)
      {
        return_value = 1;
        repeat = 0;
      }
      else
      {
        clear_screen(COLOR_BG);
        blit_to_screen(original_screen, 240, 160, 230, 40);
      }
    }
printf("load state over\n");
    }
  }

  void menu_load_state_file()
  {
    char *file_ext[] = { ".svs", NULL };
    char load_filename[512];
    if(load_file(file_ext, load_filename, DEFAULT_SAVE_DIR) != -1)
    {
      if (load_state(load_filename, SAVESTATE_SLOT) == 1)
      {
        return_value = 1;
        repeat = 0;
      }
      else
      {
        clear_screen(COLOR_BG);
        choose_menu(current_menu);
      }
    }
    else
    {
      choose_menu(current_menu);
    }
  }

    void menu_savestate_passive()
    {
        u32 k, j, x, y;
        u32 active, active0;

        active0= 0;
        if(current_option_num == 1 || current_option_num == 2) active0 = 1;

        x= 18;
        y= 100;
        for(k= 0, j= 0; k < 32; k++)
        {
            if(j== 16) {j= 0; y += 20;}
            if(game_config.savestate_map & (1<<k)) active= active0+1;
            else active = 0;

            if(game_config.savestate_index == k && active0 >0)
            {
                drawbox(screen_address, x+j*14-1, y-1, x+j*14+10, y+10, COLOR16(31, 31, 31));
                if(active== 2) active = 3;
            }
            draw_selitem(screen_address, x + j*14, y, 1, active);

            j++;
        }
    }

  // メニュー:チートファイルのロード
  void menu_load_cheat_file()
  {
#if 0
    char *file_ext[] = { ".cht", NULL };
    char load_filename[MAX_FILE];
    u32 i;

    if(load_file(file_ext, load_filename, DEFAULT_CHEAT_DIR) != -1)
    {
      add_cheats(load_filename);
      for(i = 0; i < MAX_CHEATS; i++)
      {
        if(i >= g_num_cheats)
        {
          sprintf(cheat_format_str[i], msg[MSG_CHEAT_MENU_NON_LOAD], i);
        }
        else
        {
          sprintf(cheat_format_str[i], msg[MSG_CHEAT_MENU_0], i, game_config.cheats_flag[i].cheat_name);
        }
      }
      choose_menu(current_menu);

    }
    else
    {
      choose_menu(current_menu);
    }
#endif
  }

  void menu_fix_gamepad_help()
  {
  }

  void submenu_graphics()
  {

  }

  void submenu_audio_switch()
  {
  }

  void submenu_cheats()
  {

  }

    void submenu_tools()
    {
    }

    void submenu_screensnap()
    {
    }

    void submenu_others()
    {
    }

    void submenu_load_game()
    {
    }

  void submenu_gamepad()
  {

  }

  void submenu_analog()
  {

  }

  void submenu_game_state()
  {
    get_savestate_filename_noshot(SAVESTATE_SLOT, current_savestate_filename);
//    PRINT_STRING_BG(msg[MSG_STATE_MENU_TITLE], COLOR_ACTIVE_ITEM, COLOR_BG, 10, 70);
    menu_change_state();
  }

    void submenu_main()
    {
    }

  void delete_savestate()
  {
  }

    void screen_snap()
    {
    }

    void keyremap()
    {
        if(gui_action== CURSOR_RIGHT || gui_action== CURSOR_LEFT)
        switch(current_option_num)
        {
            case 1: //GBA KEY A
                if(BUTTON_MAP_A == BUTTON_ID_A)
                    BUTTON_MAP_A = BUTTON_ID_Y;
                else if(BUTTON_MAP_A == BUTTON_ID_Y)
                    BUTTON_MAP_A = BUTTON_ID_A;

                BUTTON_MAP_FA = BUTTON_ID_NONE;
                BUTTON_MAP_FB = BUTTON_ID_NONE;
              break;
            case 2: //GBA KEY B(fixed B)
              break;
            case 3: //GBA KEY FA(fixed X)
                if(BUTTON_MAP_FA == BUTTON_ID_NONE)
                {
                   BUTTON_MAP_FA = BUTTON_ID_X;
                }
                else
                   BUTTON_MAP_FA = BUTTON_ID_NONE;

              break;
            case 4: //GBA KEY FB
                if(BUTTON_MAP_A == BUTTON_ID_A)
                {
                    if(BUTTON_MAP_FB == BUTTON_ID_Y)
                        BUTTON_MAP_FB = BUTTON_ID_NONE;
                    else
                        BUTTON_MAP_FB = BUTTON_ID_Y;
                }
                else if(BUTTON_MAP_A == BUTTON_ID_Y)
                {
                    if(BUTTON_MAP_FB == BUTTON_ID_NONE)
                        BUTTON_MAP_FB = BUTTON_ID_A;
                    else
                        BUTTON_MAP_FB = BUTTON_ID_NONE;
                }
              break;
            case 5: //Awaking menu
                if(BUTTON_MAP_MENU == BUTTON_ID_X)
                {
                    if(BUTTON_MAP_A != BUTTON_ID_Y && BUTTON_MAP_FB != BUTTON_ID_Y)
                        BUTTON_MAP_MENU = BUTTON_ID_Y;
                    else
                        BUTTON_MAP_MENU = BUTTON_ID_TOUCH;
                }
                else if(BUTTON_MAP_MENU == BUTTON_ID_TOUCH)
                {
                    if(BUTTON_MAP_FA != BUTTON_ID_X)
                        BUTTON_MAP_MENU = BUTTON_ID_X;
                    else if(BUTTON_MAP_A != BUTTON_ID_Y && BUTTON_MAP_FB != BUTTON_ID_Y)
                        BUTTON_MAP_MENU = BUTTON_ID_Y;
                }
                else
                    BUTTON_MAP_MENU = BUTTON_ID_TOUCH;
              break;
            default:
              break;
        }

        if(BUTTON_MAP_MENU == BUTTON_ID_X && BUTTON_MAP_FA == BUTTON_ID_X)
        {
            if(BUTTON_MAP_A == BUTTON_ID_Y || BUTTON_MAP_FB == BUTTON_ID_Y)
                BUTTON_MAP_MENU = BUTTON_ID_TOUCH;
            else
                BUTTON_MAP_MENU = BUTTON_ID_Y;
        }
        else if(BUTTON_MAP_MENU == BUTTON_ID_Y)
        {
            if(BUTTON_MAP_A == BUTTON_ID_Y || BUTTON_MAP_FB == BUTTON_ID_Y)
            {
                if(BUTTON_MAP_FA == BUTTON_ID_X)
                    BUTTON_MAP_MENU = BUTTON_ID_TOUCH;
                else
                    BUTTON_MAP_MENU = BUTTON_ID_X;
            }
        }

        gamepad_config_home = BUTTON_MAP_MENU;
    }

    void default_setting()
    {
    }

    void load_lastest_played()
    {
    }

    void language_set()
    {
printf("language %d\n", gpsp_config.language);
        load_language_msg("language.msg", gpsp_config.language);
printf("language over\n");
    }

    char *screen_ratio_options[] = { &msg[MSG_SCREEN_RATIO_0], &msg[MSG_SCREEN_RATIO_1] };
    
    char *frameskip_options[] = { &msg[MSG_FRAMESKIP_0], &msg[MSG_FRAMESKIP_1] };

    char *on_off_options[] = { &msg[MSG_ON_OFF_0], &msg[MSG_ON_OFF_1] };

    char *sound_seletion[] = { &msg[MSG_SOUND_SWITCH_0], &msg[MSG_SOUND_SWITCH_1] };

    char *snap_frame_options[] = { &msg[MSG_SNAP_FRAME_0], &msg[MSG_SNAP_FRAME_1] };

    char *enable_disable_options[] = { &msg[MSG_EN_DIS_ABLE_0], &msg[MSG_EN_DIS_ABLE_1] };

    char *language_options[] = { &lang[0], &lang[1] };

    char *keyremap_options[] = {&msg[MSG_KEY_MAP_NONE], &msg[MSG_KEY_MAP_A], &msg[MSG_KEY_MAP_B], 
                                &msg[MSG_KEY_MAP_SL], &msg[MSG_KEY_MAP_ST], &msg[MSG_KEY_MAP_RT], 
                                &msg[MSG_KEY_MAP_LF], &msg[MSG_KEY_MAP_UP], &msg[MSG_KEY_MAP_DW], 
                                &msg[MSG_KEY_MAP_R], &msg[MSG_KEY_MAP_L], &msg[MSG_KEY_MAP_X], 
                                &msg[MSG_KEY_MAP_Y], &msg[MSG_KEY_MAP_TOUCH]
                                };

  /*--------------------------------------------------------
    Video & Audio
  --------------------------------------------------------*/
  MENU_OPTION_TYPE graphics_options[] =
  {
    /* 00 */ SUBMENU_OPTION(NULL, &msg[MSG_SUB_MENU_05], NULL, 0),

    /* 01 */ STRING_SELECTION_OPTION(NULL, NULL, &msg[MSG_SUB_MENU_00], screen_ratio_options, 
        &gpsp_config.screen_ratio, 2, NULL, 1),

    /* 02 */ STRING_SELECTION_OPTION(NULL, NULL, &msg[MSG_SUB_MENU_01], frameskip_options,
        &game_config.frameskip_type, 2, NULL, 2),

    /* 03 */ NUMERIC_SELECTION_OPTION(NULL, &msg[MSG_SUB_MENU_02],
        &game_config.frameskip_value, 10, NULL, 3),

    /* 04 */ STRING_SELECTION_OPTION(NULL, NULL, &msg[MSG_SUB_MENU_04], on_off_options, 
        &gpsp_config.enable_audio, 2, NULL, 4)
  };

  MAKE_MENU(graphics, submenu_graphics, NULL);

  /*--------------------------------------------------------
     Game state
  --------------------------------------------------------*/
  MENU_OPTION_TYPE game_state_options[] =
  {
    /* 00 */ SUBMENU_OPTION(NULL, &msg[MSG_SUB_MENU_14], NULL, 0),

    /* 01 */ NUMERIC_SELECTION_ACTION_OPTION(menu_save_state, menu_savestate_passive,
        &msg[MSG_SUB_MENU_10], &game_config.savestate_index, 32, NULL, 1),

    /* 02 */ NUMERIC_SELECTION_ACTION_OPTION(menu_load_state, menu_savestate_passive,
        &msg[MSG_SUB_MENU_11], &game_config.savestate_index, 32, NULL, 2),

    /* 03 */ ACTION_OPTION(delete_savestate, NULL, &msg[MSG_SUB_MENU_13], NULL, 5)

  };

  MAKE_MENU(game_state, submenu_game_state, NULL);

  /*--------------------------------------------------------
     Cheat options
  --------------------------------------------------------*/
  MENU_OPTION_TYPE cheats_options[] =
  {    
    /* 00 */ SUBMENU_OPTION(NULL, &msg[MSG_SUB_MENU_24], NULL, 0),

    /* 01 */ CHEAT_OPTION(((10 * menu_cheat_page) + 0), 1),
    /* 02 */ CHEAT_OPTION(((10 * menu_cheat_page) + 1), 2),
    /* 03 */ CHEAT_OPTION(((10 * menu_cheat_page) + 2), 3),
    /* 04 */ CHEAT_OPTION(((10 * menu_cheat_page) + 3), 4),

    /* 11 */ NUMERIC_SELECTION_OPTION(reload_cheats_page, &msg[MSG_SUB_MENU_20],
        &menu_cheat_page, MAX_CHEATS_PAGE, NULL, 5),
    /* 12 */ ACTION_OPTION(menu_load_cheat_file, NULL, &msg[MSG_SUB_MENU_21], 
        NULL, 6)
  };

  MAKE_MENU(cheats, submenu_cheats, NULL);

  /*--------------------------------------------------------
     Tools-screensanp
  --------------------------------------------------------*/
    MENU_TYPE tools_menu;

    MENU_OPTION_TYPE tools_screensnap_options[] =
    {
        /* 00 */ SUBMENU_OPTION(&tools_menu, &msg[MSG_SUB_MENU_302], NULL, 0),

        /* 01 */ STRING_SELECTION_OPTION(NULL, NULL, &msg[MSG_SUB_MENU_300], 
            snap_frame_options, 0, 2, NULL, 1),

        /* 02 */ ACTION_OPTION(screen_snap, NULL, &msg[MSG_SUB_MENU_301], NULL, 2)
    };

    MAKE_MENU(tools_screensnap, submenu_screensnap, NULL);

  /*--------------------------------------------------------
     Tools-keyremap
  --------------------------------------------------------*/
    MENU_OPTION_TYPE tools_keyremap_options[] = 
    {
        /* 00 */ SUBMENU_OPTION(&tools_menu, &msg[MSG_SUB_MENU_315], NULL, 0),
        
        /* 01 */ STRING_SELECTION_HIDEN_OPTION(keyremap, keyremap_show, &msg[MSG_SUB_MENU_310], 
            NULL, &string_select, 2, NULL, 1),
        
        /* 02 */ STRING_SELECTION_HIDEN_OPTION(keyremap, keyremap_show, &msg[MSG_SUB_MENU_311], 
            NULL, &string_select, 1, NULL, 2),
        
        /* 03 */ STRING_SELECTION_HIDEN_OPTION(keyremap, keyremap_show, &msg[MSG_SUB_MENU_312], 
            NULL, &string_select, 2, NULL, 3),
        
        /* 04 */ STRING_SELECTION_HIDEN_OPTION(keyremap, keyremap_show, &msg[MSG_SUB_MENU_313], 
            NULL, &string_select, 3, NULL, 4),
        
        /* 05 */ STRING_SELECTION_HIDEN_OPTION(keyremap, keyremap_show, &msg[MSG_SUB_MENU_314], 
            NULL, &string_select, 3, NULL, 5)
    };

    MAKE_MENU(tools_keyremap, NULL, NULL);
  /*--------------------------------------------------------
     Tools
  --------------------------------------------------------*/
    MENU_OPTION_TYPE tools_options[] = 
    {
        /* 00 */ SUBMENU_OPTION(NULL, &msg[MSG_SUB_MENU_32], NULL, 0),

        /* 01 */ SUBMENU_OPTION(&tools_screensnap_menu, &msg[MSG_SUB_MENU_30], NULL, 1),

        /* 02 */ SUBMENU_OPTION(&tools_keyremap_menu, &msg[MSG_SUB_MENU_31], NULL, 2)
    };

//    MAKE_MENU(tools, submenu_tools, NULL);
    tools_menu.init_function = submenu_tools;
    tools_menu.passive_function = NULL;
    tools_menu.options = tools_options;
    tools_menu.num_options= sizeof(tools_options) / sizeof(MENU_OPTION_TYPE);

  /*--------------------------------------------------------
     Others
  --------------------------------------------------------*/
  MENU_OPTION_TYPE others_options[] = 
  {
    /* 00 */ SUBMENU_OPTION(NULL, &msg[MSG_SUB_MENU_45], NULL, 0),

    /* 01 */ STRING_SELECTION_OPTION(NULL, NULL, &msg[MSG_SUB_MENU_40], enable_disable_options, 
        &gpsp_config.auto_standby, 2, NULL, 1),

    /* 02 */ STRING_SELECTION_OPTION(language_set, NULL, &msg[MSG_SUB_MENU_41], language_options, 
        &gpsp_config.language, 2, NULL, 2),

    /* 03 */ STRING_SELECTION_OPTION(NULL, NULL, &msg[MSG_SUB_MENU_42], on_off_options, 
        &gpsp_config.auto_help, 2, NULL, 3),

    /* 04 */ STRING_SELECTION_OPTION(NULL, NULL, &msg[MSG_SUB_MENU_43], on_off_options, 0, 1, NULL, 4),

    /* 05 */ ACTION_OPTION(default_setting, NULL, &msg[MSG_SUB_MENU_44], NULL, 5)
  };

    MAKE_MENU(others, submenu_others, NULL);

  /*--------------------------------------------------------
     Load_game
  --------------------------------------------------------*/
    MENU_OPTION_TYPE load_game_options[] =
    {
        /* 00 */ SUBMENU_OPTION(NULL, &msg[MSG_SUB_MENU_62], NULL, 0),

        /* 01 */ ACTION_OPTION(load_lastest_played, NULL, &msg[MSG_SUB_MENU_60], NULL, 1),

        /* 02 */ ACTION_OPTION(menu_load, NULL, &msg[MSG_SUB_MENU_61], NULL, 2)
    };

    MAKE_MENU(load_game, submenu_load_game, NULL);

  /*--------------------------------------------------------
     MAIN MENU
  --------------------------------------------------------*/
  MENU_OPTION_TYPE main_options[] =
  {
    /* 00 */ SUBMENU_OPTION(&graphics_menu, &msg[MSG_MAIN_MENU_0], NULL, 0),

    /* 01 */ SUBMENU_OPTION(&game_state_menu, &msg[MSG_MAIN_MENU_1], NULL, 1),

    /* 02 */ SUBMENU_OPTION(&cheats_menu, &msg[MSG_MAIN_MENU_2], NULL, 2),

    /* 03 */ SUBMENU_OPTION(&tools_menu, &msg[MSG_MAIN_MENU_3], NULL, 3),

    /* 04 */ SUBMENU_OPTION(&others_menu, &msg[MSG_MAIN_MENU_4], NULL, 4),

    /* 05 */ ACTION_OPTION(menu_exit, NULL, &msg[MSG_MAIN_MENU_5], NULL, 5),

    /* 06 */ SUBMENU_OPTION(&load_game_menu, NULL, NULL, 6),

    /* 07 */ ACTION_OPTION(menu_restart, NULL, NULL, NULL, 7),
    
    /* 08 */ ACTION_OPTION(menu_return, NULL, NULL, NULL, 8)
  };

  MAKE_MENU(main, submenu_main, main_menu_passive);

    void main_menu_passive()
    {
        switch(gui_action)
        {
            case CURSOR_RIGHT:
                if(current_option_num >= (main_menu.num_options - 3))
                {
                    if(current_option_num < (main_menu.num_options - 1))
                        current_option_num++;
                    else
                        current_option_num = main_menu.num_options - 3;
                }
                current_option = main_menu.options + current_option_num;
              break;

            case CURSOR_LEFT:
                if(current_option_num >= (main_menu.num_options - 3))
                {
                    if(current_option_num == (main_menu.num_options - 3))
                        current_option_num = main_menu.num_options - 1;
                    else
                        current_option_num--;
                }
                current_option = main_menu.options + current_option_num;
              break;

            default:
              break;
        }// end swith
    }

  void choose_menu(MENU_TYPE *new_menu)
  {
    if(new_menu == NULL)
      new_menu = &main_menu;

//    clear_screen(COLOR_BG);
//    blit_to_screen(original_screen, 240, 160, 230, 40);

    current_menu = new_menu;
    current_option_num= 0;
    current_option = new_menu->options;
    if(current_menu->init_function)
        current_menu->init_function();
  }

  void reload_cheats_page()
  {
    for(i = 1; i<10; i++)
    {
      cheats_options[i].display_string = cheat_format_str[(10 * menu_cheat_page) + i];
      cheats_options[i].current_option = &(game_config.cheats_flag[(10 * menu_cheat_page) + i].cheat_active);
    }
  }

  void home_mode()
  {
//    sceImposeSetHomePopup(gpsp_config.enable_home);
  }

/*
  void set_gamepad()
  {
    if(game_config.use_default_gamepad_map == 1)
    {
      memcpy(game_config.gamepad_config_map, gamepad_config_map, sizeof(game_config.gamepad_config_map));
      memcpy(gamepad_config_map, gpsp_config.gamepad_config_map, sizeof(gpsp_config.gamepad_config_map));
    }
    else
    {
      memcpy(gpsp_config.gamepad_config_map, gamepad_config_map, sizeof(gpsp_config.gamepad_config_map));
      memcpy(gamepad_config_map, game_config.gamepad_config_map, sizeof(game_config.gamepad_config_map));
    }
  }
*/

    void keyremap_show()
    {
        unsigned short color;
        u32 line_num;
        u32 index;

        if(display_option->option_type & STRING_SELECTION_TYPE)
        {
            switch(i)
            {
                case 1: index= BUTTON_MAP_A; break;
                case 2: index= BUTTON_MAP_B; break;
                case 3: index= BUTTON_MAP_FA; break;
                case 4: index= BUTTON_MAP_FB; break;
                case 5: index= BUTTON_MAP_MENU; break;
                default: index= 0; break;
            }

            line_num= 0;
            while(index) {index >>= 1; line_num++;}
            sprintf(line_buffer, *(display_option->display_string), *((u32*)keyremap_options[line_num]));
        }
        else
            strcpy(line_buffer, *(display_option->display_string));
    
        line_num= display_option-> line_number;
        if(display_option == current_option)
            color= COLOR_ACTIVE_ITEM;
        else
            color= COLOR_INACTIVE_ITEM;
    
        PRINT_STRING_BG_UTF8(line_buffer, COLOR_INACTIVE_ITEM, COLOR_TRANS, 27,
            38 + line_num*20);
    
        if(display_option == current_option)
        {
            show_icon((char*)screen_address, ICON_SUBM_ISOLATOR, 7, 33 + line_num*20);
            show_icon((char*)screen_address, ICON_MENUINDEX, 12, 42 + line_num*20);
            show_icon((char*)screen_address, ICON_SUBM_ISOLATOR, 7, 53 + line_num*20);
        }
    }

  // ここからメニューの処理
  pause_sound(1);
  button_up_wait();
  video_resolution(FRAME_MENU);
  clear_screen(COLOR_BG);
  // MENU時は222MHz
  set_cpu_clock(10);

  if(gamepak_filename[0] == 0)
  {
    first_load = 1;
    memset(original_screen, 0x00, 240 * 160 * 2);
//    PRINT_STRING_EXT_BG(msg[MSG_NON_LOAD_GAME], 0xFFFF, 0x0000, 60, 75, original_screen, 240);
  }

    choose_menu(&main_menu);
    current_menu = &main_menu;

/*
  for(i = 0; i < MAX_CHEATS; i++)
  {
    if(i >= g_num_cheats)
    {
      sprintf(cheat_format_str[i], msg[MSG_CHEAT_MENU_NON_LOAD], i);
    }
    else
    {
      sprintf(cheat_format_str[i], msg[MSG_CHEAT_MENU_0], i, game_config.cheats_flag[i].cheat_name);
    }
  }
*/

//  reload_cheats_page();

    // Menu loop
  while(repeat)
  {

//    print_status(0);
    if(current_menu == &main_menu)
        show_background(screen_address, MENU_BACKGROUND);
    else
        show_background(screen_address, SUBM_BACKGROUND);
//    blit_to_screen(original_screen, 240, 160, 230, 40);

    display_option = current_menu->options;
    string_select= 0;

    if(current_menu == &main_menu)
    {
        for(i = 0; i < current_menu->num_options -3; i++, display_option++)
        {
          unsigned short color;
          u32 line_num;
          strcpy(line_buffer, *(display_option->display_string));

          line_num= display_option-> line_number;
          if(display_option == current_option)
            color= COLOR_ACTIVE_ITEM;
          else
            color= COLOR_INACTIVE_ITEM;

          PRINT_STRING_BG_UTF8(line_buffer, color, COLOR_TRANS, 160, 35 + line_num*16);

          if(display_option == current_option)
          {
            show_icon((char*)screen_address, ICON_MENU_ISOLATOR, 143, 33 + line_num*16);
            show_icon((char*)screen_address, ICON_MENUINDEX, 150, 39 + line_num*16);
            show_icon((char*)screen_address, ICON_MENU_ISOLATOR, 143, 49 + line_num*16);
          }
        }

        if(display_option++ == current_option)
          show_icon((char*)screen_address, ICON_NEWGAME_SLT, 0, 132);
        else
          show_icon((char*)screen_address, ICON_NEWGAME, 0, 132);

        if(display_option++ == current_option)
          show_icon((char*)screen_address, ICON_RESTGAME_SLT, 70, 132);
        else
          show_icon((char*)screen_address, ICON_RESTGAME, 70, 132);

        if(display_option++ == current_option)
          show_icon((char*)screen_address, ICON_RETGAME_SLT, 168, 132);
        else
          show_icon((char*)screen_address, ICON_RETGAME, 168, 132);
    }
    else
    {
        for(i = 0; i < current_menu->num_options; i++, display_option++)
        {
          unsigned short color;
          u32 line_num;

          if(!(display_option->option_type & HIDEN_TYPE))
          {
              if(display_option->option_type & NUMBER_SELECTION_TYPE)
              {
                sprintf(line_buffer, *(display_option->display_string),
                 *(display_option->current_option));
              }
              else if(display_option->option_type & STRING_SELECTION_TYPE)
              {
                sprintf(line_buffer, *(display_option->display_string),
                 *((u32*)(((u32 *)display_option->options)[*(display_option->current_option)])));
              }
              else
              {
                strcpy(line_buffer, *(display_option->display_string));
              }
    
              line_num= display_option-> line_number;
              if(display_option == current_option)
                color= COLOR_ACTIVE_ITEM;
              else
                color= COLOR_INACTIVE_ITEM;
    
              PRINT_STRING_BG_UTF8(line_buffer, COLOR_INACTIVE_ITEM, COLOR_TRANS, 27,
                38 + line_num*20);
    
              if(display_option == current_option)
              {
                show_icon((char*)screen_address, ICON_SUBM_ISOLATOR, 7, 33 + line_num*20);
                show_icon((char*)screen_address, ICON_MENUINDEX, 12, 42 + line_num*20);
                show_icon((char*)screen_address, ICON_SUBM_ISOLATOR, 7, 53 + line_num*20);
              }

          }

          if(display_option->passive_function)
            display_option->passive_function();
        }
    }

//    PRINT_STRING_BG(current_option->help_string, COLOR_HELP_TEXT, COLOR_BG, 30, 210);

    gui_action = get_gui_input();

//    if (quit_flag == 1)
//      menu_quit();

    switch(gui_action)
    {
      case CURSOR_DOWN:
        current_option_num = (current_option_num + 1) %
          current_menu->num_options;

        current_option = current_menu->options + current_option_num;
//        clear_help();
        break;

      case CURSOR_UP:
        if(current_option_num)
          current_option_num--;
        else
          current_option_num = current_menu->num_options - 1;

        current_option = current_menu->options + current_option_num;
//        clear_help();
        break;

      case CURSOR_RIGHT:
        if(current_option->option_type & (NUMBER_SELECTION_TYPE |
         STRING_SELECTION_TYPE))
        {
          u32 current_option_val = *(current_option->current_option);

          if(current_option_val <  current_option->num_options -1)
            current_option_val++;
          else
            current_option_val= 0;

          *(current_option->current_option) = current_option_val;

          if(current_option->action_function)
            current_option->action_function();
        }

            if(current_menu->passive_function)
                current_menu->passive_function();
        break;

      case CURSOR_LEFT:
        if(current_option->option_type & (NUMBER_SELECTION_TYPE |
         STRING_SELECTION_TYPE))
        {
          u32 current_option_val = *(current_option->current_option);

          if(current_option_val)
            current_option_val--;
          else
            current_option_val = current_option->num_options - 1;

          *(current_option->current_option) = current_option_val;

          if(current_option->action_function)
            current_option->action_function();
        }

            if(current_menu->passive_function)
                current_menu->passive_function();
        break;

      case CURSOR_EXIT:
        if(current_menu == &main_menu)
          menu_return();

        choose_menu(&main_menu);
        break;

      case CURSOR_SELECT:
        if(current_option->option_type & ACTION_TYPE)
            current_option->action_function();
        else if(current_option->option_type & SUBMENU_TYPE)
            choose_menu(current_option->sub_menu);
        break;

      case CURSOR_KEY_SELECT:
        break;

      default:
        ;
        break;
    }  // end swith

    flip_screen();
    clear_screen(COLOR_BLACK);
  }  // end while

  clear_screen(0);
  flip_screen();
  OSTimeDly(10);        //about 100ms
  clear_screen(0);
  flip_screen();

//  video_resolution(FRAME_GAME);

  set_cpu_clock(game_config.clock_speed_number);

  pause_sound(0);
//  real_frame_count = 0;
//  virtual_frame_count = 0;

// menu終了時の処理
  button_up_wait();

  return return_value;
}

u32 load_dircfg(char *file_name)  // TODO: Working directory configure
{
  int loop;
  int next_line;
  char current_line[256];
  char current_str[256];
  FILE *msg_file;
  char msg_path[MAX_PATH];

  sprintf(msg_path, "%s\\%s", main_path, file_name);
  msg_file = fopen(msg_path, "r");

  next_line = 0;
  if(msg_file)
  {
    loop = 0;
    while(fgets(current_line, 256, msg_file))
    {

      if(parse_line(current_line, current_str) != -1)
      {
        sprintf(current_line, "%s\\%s", main_path, current_str+2);

        switch(loop)
        {
          case 0:
            strcpy(g_default_rom_dir, current_line);
            if(opendir(current_line) == NULL)
            {
              printf("can't open rom dir : %s\n", g_default_rom_dir);
              strcpy(g_default_rom_dir, main_path);
//              *g_default_rom_dir = (char)NULL;
            }
            loop++;
            break;

          case 1:
            strcpy(DEFAULT_SAVE_DIR, current_line);
            if(opendir(current_line) == NULL)
            {
              printf("can't open save dir : %s\n", DEFAULT_SAVE_DIR);
              strcpy(DEFAULT_SAVE_DIR, main_path);
//              *DEFAULT_SAVE_DIR = (char)NULL;
            }
            loop++;
            break;

          case 2:
            strcpy(DEFAULT_CFG_DIR, current_line);
            if(opendir(current_line) == NULL)
            {
              printf("can't open cfg dir : %s\n", DEFAULT_CFG_DIR);
              strcpy(DEFAULT_CFG_DIR, main_path);
//              *DEFAULT_CFG_DIR = (char)NULL;
            }
            loop++;
            break;

          case 3:
            strcpy(DEFAULT_SS_DIR, current_line);
            if(opendir(current_line) == NULL)
            {
              printf("can't open screen shot dir : %s\n", current_line);
              strcpy(DEFAULT_SS_DIR, main_path);
//              *DEFAULT_SS_DIR = (char)NULL;
            }
            loop++;
            break;

          case 4:
            strcpy(DEFAULT_CHEAT_DIR, current_line);
            if(opendir(current_line) == NULL)
            {
              printf("can't open cheat dir : %s\n", current_line);
              strcpy(DEFAULT_CHEAT_DIR, main_path);
//              *DEFAULT_CHEAT_DIR = (char)NULL;
            }
            loop++;
            break;
        }
      }
    }

    fclose(msg_file);
    if (loop == 5)
    {
      return 0;
    }
    else
    {
      return -1;
    }
  }
  fclose(msg_file);
  return -1;
}

u32 load_fontcfg(char *file_name)  // TODO:スマートな実装に書き直す
{
  int loop;
  int next_line;
  char current_line[256];
  char current_str[256];
  FILE *msg_file;
  char msg_path[MAX_PATH];

  sprintf(msg_path, "%s\\%s", main_path, file_name);

  m_font[0] = '\0';
  s_font[0] = '\0';

  msg_file = fopen(msg_path, "rb");

  next_line = 0;
  if(msg_file)
  {
    loop = 0;
    while(fgets(current_line, 256, msg_file))
    {
      if(parse_line(current_line, current_str) != -1)
      {
        sprintf(current_line, "%s\\%s", main_path, current_str+2);
printf("main_path %s\n", main_path);
printf("current_str %s\n", current_str);
        switch(loop)
        {
          case 0:
            strcpy(m_font, current_line);
            loop++;
            break;
          case 1:
            strcpy(s_font, current_line);
            loop++;
            break;
        }
      }
    }

    fclose(msg_file);
    if (loop > 0)
      return 0;
    else
      return -1;
  }
  fclose(msg_file);
  return -1;
}
/*--------------------------------------------------------
--------------------------------------------------------*/
int load_language_msg(char *filename, u32 language)
{
    FILE *fp;
    char msg_path[MAX_PATH];
    char string[256];
    char start[32];
    char end[32];
    char *pt, *dst;
    u32 cmplen;
    u32 loop, offset, len;
    int ret;

    sprintf(msg_path, "%s\\%s", main_path, filename);
    fp = fopen(msg_path, "rb");
    if(fp == NULL)
        return -1;

    switch(language)
    {
    case ENGLISH:
        strcpy(start, "STARTENGLISH");
        strcpy(end, "ENDENGLISH");
        cmplen= 12;
        break;
    case CHINESE_SIMPLIFIED:
        strcpy(start, "STARTCHINESESIM");
        strcpy(end, "ENDCHINESESIM");
        cmplen= 15;
        break;
    default:
        strcpy(start, "STARTENGLISH");
        strcpy(end, "ENDENGLISH");
        cmplen= 12;
        break;
    }
    //find the start flag
    ret= 0;
    while(1)
    {
        pt= fgets(string, 256, fp);
        if(pt == NULL)
        {
            ret= -2;
            goto load_language_msg_error;
        }

        if(!strncmp(pt, start, cmplen))
            break;
    }

    loop= 0;
    offset= 0;
    dst= msg_data;
    msg[0]= dst;

    while(loop != MSG_END)
    {
        while(1)
        {
            pt = fgets(string, 256, fp);
            if(pt[0] == '#' || pt[0] == 0x0D || pt[0] == 0x0A)
                continue;
            if(pt != NULL)
                break;
            else
            {
                ret = -3;
                goto load_language_msg_error;
            }
        }

        if(!strncmp(pt, end, cmplen-2))
            break;

        len= strlen(pt);
        memcpy(dst, pt, len);

        dst += len;
        //at a line return, when "\n" paded, this message not end
        if(*(dst-1) == 0x0A)
        {
            pt = strrchr(pt, '\\');
            if((pt != NULL) && (*(pt+1) == 'n'))
            {
                if(*(dst-2) == 0x0D)
                {
                    *(dst-4)= '\n';
                    dst -= 3;
                }
                else
                {
                    *(dst-3)= '\n';
                    dst -= 2;
                }
            }
            else//a message end
            {
                if(*(dst-2) == 0x0D)
                    dst -= 1;
                *(dst-1) = '\0';
                msg[++loop] = dst;
            }
        }
    }

/*
unsigned char *ppt;
len= 0;
ppt= msg[0];
printf("------\n");
while(*ppt)
    printf("%02x ", *ppt++);
*/

load_language_msg_error:
  fclose(fp);
  return ret;
}
#if 0
u32 load_msgcfg(char *file_name)
{
  int loop;
  int next_line;
  char current_line[256];
  char current_str[256];
  FILE *msg_file;
  char msg_path[MAX_PATH];
  u32 offset;

  sprintf(msg_path, "%s\\%s", main_path, file_name);
  msg_file = fopen(msg_path, "rb");

  next_line = 0;
  offset = 0;
  if(msg_file)
  {
    loop = 0;
    while(fgets(current_line, 256, msg_file))
    {
      if(parse_line(current_line, current_str) != -1)
      {
        if (loop <= (MSG_END + 1 + next_line)) {
          if (next_line == 0)
          {
            // 新しい行の場合
            msg[loop] = &msg_data[offset];  // 新しい行
            next_line = 1;
            loop++;
          }
          strcpy(&msg_data[offset], current_str);
          offset = offset + strlen(current_str);  // offset はNULLの位置を示す
        }
      }
      else
      {
        next_line = 0;
        offset++;
      }
    }

    fclose(msg_file);
    if (loop == (MSG_END))
    {
      return 0;
    }
    else
    {
      return -1;
    }
  }
  fclose(msg_file);
  return -1;
}
#endif
/*--------------------------------------------------------
  加载字体库
--------------------------------------------------------*/
u32 load_font()
{
    return (u32)BDF_font_init();
}

void get_savestate_filename_noshot(u32 slot, char *name_buffer)
{
  char savestate_ext[16];

  sprintf(savestate_ext, "_%d.svs", (int)slot);
  change_ext(gamepak_filename, name_buffer, savestate_ext);
}

/*--------------------------------------------------------
  game cfgファイルの書込
--------------------------------------------------------*/
s32 save_game_config_file()
{
  char game_config_filename[MAX_FILE];
  char game_config_path[MAX_PATH];
  FILE_ID game_config_file;

  if(gamepak_filename[0] == 0) return -1;

  change_ext(gamepak_filename, game_config_filename, ".cfg");

  if (DEFAULT_CFG_DIR != NULL)
    sprintf(game_config_path, "%s\%s", DEFAULT_CFG_DIR, game_config_filename);
  else
    strcpy(game_config_path, game_config_filename);

  if(game_config.use_default_gamepad_map == 0)
    memcpy(game_config.gamepad_config_map, gamepad_config_map, sizeof(game_config.gamepad_config_map));

  FILE_OPEN(game_config_file, game_config_path, WRITE);
  if(FILE_CHECK_VALID(game_config_file))
  {
    FILE_WRITE(game_config_file, (int *)GAME_CONFIG_HEADER, sizeof(u32));
    FILE_WRITE_VARIABLE(game_config_file, game_config_ver);
    FILE_WRITE_VARIABLE(game_config_file, game_config);
    FILE_CLOSE(game_config_file);
    return 0;
  }
  FILE_CLOSE(game_config_file);
  return -1;
}

/*--------------------------------------------------------
  gpSP cfgファイルの書込
--------------------------------------------------------*/
s32 save_config_file()
{
  char gpsp_config_path[MAX_PATH];
  FILE_ID gpsp_config_file;
  u32 header;

  save_game_config_file();

  sprintf(gpsp_config_path, "%s\\%s", main_path, GPSP_CONFIG_FILENAME);
//  if(game_config.use_default_gamepad_map == 1)
//    memcpy(gpsp_config.gamepad_config_map, gamepad_config_map, sizeof(gpsp_config.gamepad_config_map));

  FILE_OPEN(gpsp_config_file, gpsp_config_path, WRITE);
  if(FILE_CHECK_VALID(gpsp_config_file))
  {
    header= GPSP_CONFIG_HEADER_U32;
    FILE_WRITE(gpsp_config_file, &header, sizeof(u32));
    FILE_WRITE_VARIABLE(gpsp_config_file, gpsp_config_ver);
    FILE_WRITE_VARIABLE(gpsp_config_file, gpsp_config);
    FILE_CLOSE(gpsp_config_file);
    return 0;
  }
  FILE_CLOSE(gpsp_config_file);
  return -1;
}

/******************************************************************************
 * local function definition
 ******************************************************************************/
static void get_savestate_snapshot(char *savestate_filename, u32 slot_num)
  {
    u16 snapshot_buffer[240 * 160];
    char savestate_timestamp_string[80];
    char savestate_path[1024];
    FILE_ID savestate_file;
    u64 savestate_time_flat;
    u64 local_time;
    int wday;
    struct rtc  current_time;
    u32 valid_flag = 0;

    sprintf(savestate_path, "%s\\%s", DEFAULT_SAVE_DIR, savestate_filename);

    if (slot_num != MEM_STATE_NUM)
      {
        FILE_OPEN(savestate_file, savestate_path, READ);
        if(FILE_CHECK_VALID(savestate_file))
          {
            FILE_READ_ARRAY(savestate_file, snapshot_buffer);
            FILE_READ_VARIABLE(savestate_file, savestate_time_flat);
            FILE_CLOSE(savestate_file);
            valid_flag = 1;
          }
      }
    else
      {
        if (mem_save_flag == 1)
          {
            g_state_buffer_ptr = savestate_write_buffer;
            FILE_READ_MEM_ARRAY(g_state_buffer_ptr, snapshot_buffer);
            FILE_READ_MEM_VARIABLE(g_state_buffer_ptr, savestate_time_flat);
            valid_flag = 1;
          }
      }

    if (valid_flag == 1)
      {
//        sceRtcConvertUtcToLocalTime(&savestate_time_flat, &local_time);

//        sceRtcSetTick(&current_time, &local_time);
//        wday = sceRtcGetDayOfWeek(current_time.year, current_time.month, current_time.day);
//        get_timestamp_string(savestate_timestamp_string, MSG_STATE_MENU_DATE_FMT_0, current_time.year,
//                             current_time.month, current_time.day,
//                             wday, current_time.hour, current_time.minutes, current_time.seconds, 0);

        savestate_timestamp_string[40] = 0;

        PRINT_STRING_BG(savestate_timestamp_string, COLOR_HELP_TEXT, COLOR_BG, 10, 40);
      }
    else
      {
//        memset(snapshot_buffer, 0, 240 * 160 * 2);
//        PRINT_STRING_EXT_BG(msg[MSG_STATE_MENU_STATE_NONE], 0xFFFF, 0x0000, 15, 75, snapshot_buffer, 240);
//        get_timestamp_string(savestate_timestamp_string, MSG_STATE_MENU_DATE_NONE_0, 0, 0, 0, 0, 0, 0, 0, 0);
//        PRINT_STRING_BG(savestate_timestamp_string, COLOR_HELP_TEXT, COLOR_BG, 10, 40);
      }
    blit_to_screen(snapshot_buffer, 240, 160, 230, 40);
  }

static void get_savestate_filename(u32 slot, char *name_buffer)
{
  char savestate_ext[16];

  sprintf(savestate_ext, "_%d.svs", slot);
  change_ext(gamepak_filename, name_buffer, savestate_ext);
}

static u32 parse_line(char *current_line, char *current_str)
{
  char *line_ptr;
  char *line_ptr_new;

  line_ptr = current_line;
  /* NULL or comment or other */
  if((current_line[0] == 0) || (current_line[0] == '#') || (current_line[0] != '!'))
    return -1;

  line_ptr++;

  line_ptr_new = strchr(line_ptr, '\r');
  while (line_ptr_new != NULL)
  {
    *line_ptr_new = '\n';
    line_ptr_new = strchr(line_ptr, '\r');
  }

  line_ptr_new = strchr(line_ptr, '\n');
  if (line_ptr_new == NULL)
    return -1;

  *line_ptr_new = 0;

  // "\n" to '\n'
  line_ptr_new = strstr(line_ptr, "\\n");
  while (line_ptr_new != NULL)
  {
    *line_ptr_new = '\n';
    memmove((line_ptr_new + 1), (line_ptr_new + 2), (strlen(line_ptr_new + 2) + 1));
    line_ptr_new = strstr(line_ptr_new, "\\n");
  }

  strcpy(current_str, line_ptr);
  return 0;
}

static void print_status(u32 mode)
{
//  char print_buffer_1[256];
//  char print_buffer_2[256];
//  char utf8[1024];
//  struct rtc  current_time;
//  pspTime current_time;

//    get_nds_time(&current_time);
//  sceRtcGetCurrentClockLocalTime(&current_time);
//  int wday = sceRtcGetDayOfWeek(current_time.year, current_time.month , current_time.day);

//    get_timestamp_string(print_buffer_1, MSG_MENU_DATE_FMT_0, current_time.year+2000, 
//        current_time.month , current_time.day, current_time.weekday, current_time.hours,
//        current_time.minutes, current_time.seconds, 0);
//    sprintf(print_buffer_2,"%s%s", msg[MSG_MENU_DATE], print_buffer_1);
//    PRINT_STRING_BG(print_buffer_1, COLOR_HELP_TEXT, COLOR_BG, 130, 0);

//    sprintf(print_buffer_1, "nGBA Ver:%d.%d", VERSION_MAJOR, VERSION_MINOR);
//    PRINT_STRING_BG(print_buffer_1, COLOR_HELP_TEXT, COLOR_BG, 0, 0);

//  sprintf(print_buffer_1, msg[MSG_MENU_BATTERY], scePowerGetBatteryLifePercent(), scePowerGetBatteryLifeTime());
//  PRINT_STRING_BG(print_buffer_1, COLOR_HELP_TEXT, COLOR_BG, 240, 0);

//    sprintf(print_buffer_1, "MAX ROM BUF: %02d MB Ver:%d.%d %s %d Build %d",
//        (int)(gamepak_ram_buffer_size/1024/1024), VERSION_MAJOR, VERSION_MINOR,
//        VER_RELEASE, VERSION_BUILD, BUILD_COUNT);
//    PRINT_STRING_BG(print_buffer_1, COLOR_HELP_TEXT, COLOR_BG, 10, 0);

//  if (mode == 0)
//  {
//    strncpy(print_buffer_1, gamepak_filename, 40);
//    PRINT_STRING_BG_SJIS(utf8, print_buffer_1, COLOR_ROM_INFO, COLOR_BG, 10, 10);
//    sprintf(print_buffer_1, "%s  %s  %s  %0X", gamepak_title, gamepak_code,
//        gamepak_maker, (unsigned int)gamepak_crc32);
//    PRINT_STRING_BG(print_buffer_1, COLOR_ROM_INFO, COLOR_BG, 10, 20);
//  }
}

#define PSP_SYSTEMPARAM_DATE_FORMAT_YYYYMMDD	0
#define PSP_SYSTEMPARAM_DATE_FORMAT_MMDDYYYY	1
#define PSP_SYSTEMPARAM_DATE_FORMAT_DDMMYYYY	2

static void get_timestamp_string(char *buffer, u16 msg_id, u16 year, u16 mon, 
    u16 day, u16 wday, u16 hour, u16 min, u16 sec, u32 msec)
{
/*
  char *weekday_strings[] =
  {
    msg[MSG_WDAY_0], msg[MSG_WDAY_1], msg[MSG_WDAY_2], msg[MSG_WDAY_3],
    msg[MSG_WDAY_4], msg[MSG_WDAY_5], msg[MSG_WDAY_6], ""
  };

  switch(date_format)
  {
    case PSP_SYSTEMPARAM_DATE_FORMAT_YYYYMMDD:
      sprintf(buffer, msg[msg_id    ], year, mon, day, weekday_strings[wday], hour, min, sec, msec / 1000);
      break;
    case PSP_SYSTEMPARAM_DATE_FORMAT_MMDDYYYY:
      sprintf(buffer, msg[msg_id + 1], weekday_strings[wday], mon, day, year, hour, min, sec, msec / 1000);
      break;
    case PSP_SYSTEMPARAM_DATE_FORMAT_DDMMYYYY:
      sprintf(buffer, msg[msg_id + 2], weekday_strings[wday], day, mon, year, hour, min, sec, msec / 1000);
      break;
  }
*/
    char *weekday_strings[] =
    {
        "SUN", "MON", "TUE", "WED", "TUR", "FRI", "SAT"
    };

    sprintf(buffer, "%s  %02d/%02d/%04d  %02d:%02d:%02d", weekday_strings[wday], 
        day, mon, year, hour, min, sec);
}

static void save_ss_bmp(u16 *image)
{
  static unsigned char header[] ={ 'B',  'M',  0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00,
                                    0x28, 0x00, 0x00, 0x00,  240, 0x00, 0x00,
                                    0x00,  160, 0x00, 0x00, 0x00, 0x01, 0x00,
                                    0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00};

  char ss_filename[MAX_FILE];
  char timestamp[MAX_FILE];
  char save_ss_path[MAX_PATH];
  struct rtc current_time;
  char rgb_data[240*160*3];
  u8 x,y;
  u16 col;
  u8 r,g,b;

  change_ext(gamepak_filename, ss_filename, "_");

  get_nds_time(&current_time);

  sprintf(save_ss_path, "%s\\%s%2d%2d%2d%2d%2d.bmp", DEFAULT_SS_DIR, ss_filename, 
    current_time.month, current_time.day, current_time.hours, current_time.minutes, current_time.seconds);

  for(y = 0; y < 160; y++)
  {
    for(x = 0; x < 240; x++)
    {
      col = image[x + y * 240];
      r = (col >> 10) & 0x1F;
      g = (col >> 5) & 0x1F;
      b = (col) & 0x1F;

      rgb_data[(159-y)*240*3+x*3+2] = b * 255 / 31;
      rgb_data[(159-y)*240*3+x*3+1] = g * 255 / 31;
      rgb_data[(159-y)*240*3+x*3+0] = r * 255 / 31;
    }
  }

    FILE *ss = fopen( save_ss_path, "wb" );
    if( ss == NULL ) return;

    fwrite( header, sizeof(header), 1, ss );
    fwrite( rgb_data, 240*160*3, 1, ss);
    fclose( ss );
}

void _flush_cache()
{
//    sceKernelDcacheWritebackAll();
    invalidate_all_cache();
}
