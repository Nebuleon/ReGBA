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
#include "gui.h"

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

#define NDSGBA_VERSION "1.0"

#define FILE_BACKGROUND "SYSTEM\\GUI\\filebg.bmp"
#define MENU_BACKGROUND "SYSTEM\\GUI\\menubg.bmp"
#define SUBM_BACKGROUND "SYSTEM\\GUI\\submbg.bmp"

#define GPSP_CONFIG_FILENAME "SYSTEM\\ndsgba.cfg"

// dsGBA的一套头文件
// 标题 4字节
#define GPSP_CONFIG_HEADER  "NGBA1.0"
#define GPSP_CONFIG_HEADER_SIZE 7
const u32 gpsp_config_ver = 0x00010000;
GPSP_CONFIG gpsp_config;

// GAME的一套头文件
// 标题 4字节
#define GAME_CONFIG_HEADER     "GAME1.0"
#define GAME_CONFIG_HEADER_SIZE 7
#define GAME_CONFIG_HEADER_U32 0x67666367
const u32 game_config_ver = 0x00010000;
GAME_CONFIG game_config;

//save state file map
#define RTS_TIMESTAMP_POS   SVS_HEADER_SIZE
char savestate_map[32];
static unsigned int savestate_index;

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

#define INIT_MENU(name, init_functions, passive_functions)                    \
  {                                                                           \
    name##_menu.init_function = init_functions,                               \
    name##_menu.passive_function = passive_functions,                         \
    name##_menu.options = name##_options,                                     \
    name##_menu.num_options = sizeof(name##_options) / sizeof(MENU_OPTION_TYPE);\
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
  &cheat_format_ptr[number],                                                  \
  enable_disable_options,                                                     \
  &(game_config.cheats_flag[number].cheat_active),                            \
  2,                                                                          \
  NULL,                                                                       \
  line_number,                                                                \
  STRING_SELECTION_TYPE                                                       \
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
    display_string, options, option_ptr, num_options, help_string, action, line_number)\
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
  STRING_SELECTION_TYPE | action                                              \
}

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
  HIDEN_TYPE            = 0x10,
  PASSIVE_TYPE          = 0x00,
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
u32 game_fast_forward= 0;   //OFF
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
static void get_savestate_filelist(void);
static FILE* get_savestate_snapshot(char *savestate_filename);
static void get_savestate_filename(u32 slot, char *name_buffer);
static u32 get_savestate_slot(void);
static void reorder_savestate_slot(void);
void get_newest_savestate(char *name_buffer);
static int sort_function(const void *dest_str_ptr, const void *src_str_ptr);
static u32 parse_line(char *current_line, char *current_str);
static void print_status(u32 mode);
static void get_timestamp_string(char *buffer, u16 msg_id, u16 year, u16 mon, u16 day, u16 wday, u16 hour, u16 min, u16 sec, u32 msec);
static void get_time_string(char *buff, struct rtc *rtcp);
static u32 save_ss_bmp(u16 *image);
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

int sort_num= 0;

static int my_array_partion(void *array, int left, int right)
{
    unsigned int pivot= *((unsigned int*)array + left);

    while(left < right)
    {
        while(sort_function((void*)((unsigned int*)array+left), (void*)((unsigned int *)array+right)) < 0) {
            right--;
        }

        if(right== left) break;
        *((unsigned int*)array + left) = *((unsigned int*)array + right);
        *((unsigned int*)array + right) = pivot;

        if(left < right)
        {
            left++;
            if(right== left) break;
        }

        while(sort_function((void*)((unsigned int*)array+right), (void*)((unsigned int *)array+left)) > 0) {
            left++;
        }

        if(left== right) break;
        *((unsigned int*)array + right) = *((unsigned int*)array + left);
        *((unsigned int*)array + left) = pivot;
        right--;
    }

    return left;
}

void my_qsort(void *array, int left, int right)
{
    if(left < right)
    {
        int mid= my_array_partion(array, left, right);
        my_qsort(array, left, mid-1);
        my_qsort(array, mid+1, right);
    }
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
//#define FS_FAT_ATTR_DIRECTORY   0x10
#define FILE_LIST_MAX  1024
#define DIR_LIST_MAX   256
#define NAME_MEM_SIZE  256*256
/*--------------------------------------------------------
  读取文件列表
--------------------------------------------------------*/
static s32 load_file_list(struct FILE_LIST_INFO *filelist_infop)
{
    DIR *current_dir;
    char    current_dir_name[MAX_PATH];
    struct dirent *current_file;
    char *file_name;
    u32 file_name_length;
    char* name_mem_top;
    char* name_mem_base;
    char **file_list;
    char **dir_list;
    u32 num_files;                      // 当前目录的文件数量
    u32 num_dirs;                       // 当前目录的文件夹数量
    char **wildcards;
    s32 ret;

    if(filelist_infop -> current_path == NULL)
        return -1;

    name_mem_base = &(filelist_infop -> filename_mem[0]);
    name_mem_top = &(filelist_infop -> filename_mem[NAME_MEM_SIZE-1]);
    file_list = filelist_infop -> file_list;
    dir_list = filelist_infop -> dir_list;
    num_files = 0;
    num_dirs = 0;
    wildcards = filelist_infop -> wildcards;

    strcpy(current_dir_name, filelist_infop -> current_path);
    current_dir = opendir(current_dir_name);
    ret = 0;
    do
    {
        if(current_dir)
            current_file = readdir(current_dir);
        else
        {
            ret = -1;
            break;
        }

        if(current_file)
        {
            file_name = current_file->d_name;
            file_name_length = strlen(file_name);
        
            if((name_mem_base + file_name_length) >= name_mem_top)       //no enough memory
            {
                ret = 1;
                break;
            }

            if(ISDIR(current_file))                                     //Is directory
            {
                if(file_name[0] != '.')
                {
                    dir_list[num_dirs] = name_mem_base;
                    strcpy(dir_list[num_dirs], file_name);
                    num_dirs++;
                    name_mem_base += file_name_length + 1;
                }
            }
            else
            {
                char *ext_pos;
                u32 i;

                ext_pos = strrchr(file_name, '.');
                for(i = 0; wildcards[i] != NULL; i++)
                {
                    if(!strcasecmp(ext_pos, wildcards[i]))
                    {
                        file_list[num_files] = name_mem_base;
                        strcpy(file_list[num_files], file_name);
                        num_files++;
                        name_mem_base += file_name_length + 1;
                        break;
                    }
                }
            }

            if((num_dirs >= DIR_LIST_MAX) || (num_files >= FILE_LIST_MAX))
            {
                ret = 1;
                break;
            }

        }
    } while(current_file);

    if(ret >= 0)
    {
        filelist_infop -> file_num = num_files;
        filelist_infop -> dir_num = num_dirs;
    }

//printf("num_files: %d; num_dirs %d\n", num_files, num_dirs);
//    qsort((void *)file_list, num_files, sizeof(u8 *), sort_function);
    my_qsort((void *)file_list, 0, num_files-1);
//printf("pass 1\n");
//    qsort((void *)dir_list, num_dirs, sizeof(u8 *), sort_function);
    my_qsort((void *)dir_list, 0, num_dirs-1);
//printf("pass 2\n");
    closedir(current_dir);

    return ret;
}

s32 contsruct_filelist_info(struct FILE_LIST_INFO *filelist_infop, char **wildcards, char *dir_name)
{
    if(dir_name != NULL)
        strcpy(filelist_infop -> current_path, dir_name);
    else
        strcpy(filelist_infop -> current_path, main_path);

    filelist_infop -> wildcards = wildcards;
    filelist_infop -> file_num = 0;
    filelist_infop -> dir_num = 0;

    filelist_infop -> file_list= (char**)malloc(FILE_LIST_MAX * sizeof(char*));
    if(filelist_infop -> file_list == NULL)
        return -1;

    filelist_infop -> dir_list= (char**)malloc(DIR_LIST_MAX * sizeof(char*));
    if(filelist_infop -> dir_list == NULL)
        return -1;

    filelist_infop -> filename_mem= (char*)malloc(NAME_MEM_SIZE * sizeof(char));
    if(filelist_infop -> filename_mem == NULL)
        return -1;

    return 0;
}

void release_filelist_info(struct FILE_LIST_INFO *filelist_infop)
{
    if(filelist_infop -> file_list != NULL)
        free((int)filelist_infop -> file_list);

    if(filelist_infop -> dir_list != NULL)
        free((int)filelist_infop -> dir_list);

    if(filelist_infop -> filename_mem != NULL)
        free((int)filelist_infop -> filename_mem);
}

/*--------------------------------------------------------
  读取文件
--------------------------------------------------------*/
s32 load_file(char **wildcards, char *result, char *default_dir_name)
{
    struct FILE_LIST_INFO filelist_info;
    u32 repeat;
    s32 return_value;
    gui_action_type gui_action;
    u32 current_selection_item;
    u32 current_in_scroll_value;
    u32 last_in_scroll_value;
    u32 last_selection_item;
    u32 directory_changed;
    u32 shift_flag[FILE_LIST_ROWS +1];
    u32 path_scroll;
    char utf8[1024];
    u32 num_files;                      // 当前目录的文件数量
    u32 num_dirs;                       // 当前目录的文件夹数量
    char **file_list;
    char **dir_list;
    s32 flag;
    u32 to_update_filelist;
    u16 *screenp;
    u32 i, j, k;

    result[0] = '\0';
    if(default_dir_name != NULL)
        strcpy(filelist_info.current_path, default_dir_name);
    else
        strcpy(filelist_info.current_path, main_path);

    filelist_info.wildcards = wildcards;
    filelist_info.file_num = 0;
    filelist_info.dir_num = 0;

    filelist_info.file_list= (char**)malloc(FILE_LIST_MAX * sizeof(char*));
    if(filelist_info.file_list == NULL)
        return -1;

    filelist_info.dir_list= (char**)malloc(DIR_LIST_MAX * sizeof(char*));
    if(filelist_info.dir_list == NULL)
        return -1;

    filelist_info.filename_mem= (char*)malloc(NAME_MEM_SIZE * sizeof(char));
    if(filelist_info.filename_mem == NULL)
        return -1;

    screenp= (u16*)malloc(256*192*2);
    if(screenp == NULL)
        screenp = screen_address;

    flag = load_file_list(&filelist_info);
    num_files = filelist_info.file_num;
    num_dirs = filelist_info.dir_num;

    file_list = filelist_info.file_list;
    dir_list = filelist_info.dir_list;

    current_selection_item = 0;
    current_in_scroll_value = 0;
    last_in_scroll_value= -1;
    last_selection_item = -1;
    directory_changed = 1;
    path_scroll = 0;
    to_update_filelist = 0;             //to_load new file list

    repeat= 1;
    return_value = -1;
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
            //scroll page
            case CURSOR_RTRIGGER:
                if((current_in_scroll_value + FILE_LIST_ROWS) < (num_files + num_dirs))
                {
                    current_in_scroll_value += FILE_LIST_ROWS - 1;
                    current_selection_item += FILE_LIST_ROWS - 1;
                    if(current_selection_item > (num_files + num_dirs -1))
                        current_selection_item = (num_files + num_dirs -1);
                }
                break;
            //scroll page
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
            //scroll string
            case CURSOR_RIGHT:
                draw_hscroll(current_selection_item - current_in_scroll_value +1, -5);
                break;
            //scroll string
            case CURSOR_LEFT:
                draw_hscroll(current_selection_item - current_in_scroll_value +1, 5);
                break;

            case CURSOR_SELECT:
                //file selected
                if(current_selection_item + 1 <= num_files)
                {
                    if(num_files != 0)
                    {
                        repeat = 0;
                        return_value = 0;
                        strcpy(result, file_list[current_selection_item]);
                        strcpy(rom_path, filelist_info.current_path);
                        strcpy(default_dir_name, filelist_info.current_path);
                    }
                }
                //dir selected
                else if(num_dirs > 0)
                {
                    u32 current_dir_selection;

                    current_dir_selection= current_selection_item -num_files;
                    if(flag >= 0)//Only last open dir not failure
                    {
                        strcat(filelist_info.current_path, "\\");
                        strupr(dir_list[current_dir_selection]);
                        strcat(filelist_info.current_path, dir_list[current_dir_selection]);

                        to_update_filelist= 1;
                    }
                }
                break;

            case CURSOR_BACK:
                if(strcmp(filelist_info.current_path, "mmc:"))
                {
                    char *ext_pos;

                    ext_pos = strrchr(filelist_info.current_path, '\\');
                    *ext_pos = '\0';

                    to_update_filelist= 1;
                }
                break;

            case CURSOR_EXIT:
                return_value = -1;
                repeat = 0;
                break;

            default:
                break;
        }//end switch

        if(to_update_filelist)
        {
            flag = load_file_list(&filelist_info);
            num_files = filelist_info.file_num;
            num_dirs = filelist_info.dir_num;

            current_selection_item = 0;
            current_in_scroll_value = 0;
            last_in_scroll_value= -1;
            last_selection_item = -1;
            directory_changed = 1;
            path_scroll = 0;

            to_update_filelist = 0;
        }

        if(current_in_scroll_value != last_in_scroll_value || current_selection_item != last_selection_item)
        {
            for(i= 1; i < (FILE_LIST_ROWS +1); i++)
            {
                draw_hscroll_over(i);
                shift_flag[i]= 0;
            }

            char tmp_path[MAX_PATH];
            sprintf(tmp_path, "%s\\%s", main_path, FILE_BACKGROUND);
            show_background(screenp, tmp_path);
            memcpy(screen_address, screenp, 256*192*2);

            //file path
            show_icon(screen_address, ICON_FILEPATH, 14, 34);
            if(directory_changed)
            {
                draw_hscroll_over(0);
                draw_hscroll_init(screen_address, screenp, 26, 32, 160, COLOR_TRANS, 
                    COLOR_ACTIVE_ITEM, filelist_info.current_path);
                shift_flag[0]= 1;
                path_scroll= 20;
                directory_changed= 0;
            }
            else
                draw_hscroll(0, 0);

            //items
            sprintf(utf8, "%d/%d", current_selection_item +(num_files+num_dirs ? 1:0), num_files+num_dirs);
            j= BDF_cut_string(utf8, 0, 2);
            PRINT_STRING_BG(utf8, COLOR_ACTIVE_ITEM, COLOR_TRANS, (246-j), 37);

            //slider
            show_Vscrollbar((char*)screen_address, 0, 0, current_selection_item, num_files+num_dirs);

            //slected item
            if((num_files + num_dirs) > 0)
            {
                j= current_selection_item - current_in_scroll_value;
                show_icon(screenp, ICON_FILE_SELECT_BG, 29, 51 + j*24);
                show_icon(screen_address, ICON_FILE_SELECT_BG, 29, 51 + j*24);
                show_icon(screen_address, ICON_MENUINDEX, 17, 57 + j*24);
            }

            j= 0;
            k= current_in_scroll_value;
            i = num_files + num_dirs - k;
            if(i > FILE_LIST_ROWS) i= FILE_LIST_ROWS;

            for( ; i> 0; i--)
            {
                u16 color;

                if(k == current_selection_item)
                    color= COLOR_ACTIVE_ITEM;
                else
                    color= COLOR_INACTIVE_ITEM;

                if(k < num_files)
                {
                    char *ext_pos;

                    draw_hscroll_init(screen_address, screenp, FILE_LIST_POSITION, 56 + j*24, 180, 
                        COLOR_TRANS, color, file_list[k]);

                    ext_pos= strrchr(file_list[k], '.');
                    if(!strcasecmp(ext_pos, ".gba"))
                        show_icon(screen_address, ICON_GBAFILE, 29, 53 + j*24);
                    if(!strcasecmp(ext_pos, ".zip"))
                        show_icon(screen_address, ICON_ZIPFILE, 29, 53 + j*24);
                    if(!strcasecmp(ext_pos, ".bin"))
                        show_icon(screen_address, ICON_BINFILE, 29, 53 + j*24);
                }
                else
                {
                    draw_hscroll_init(screen_address, screenp, FILE_LIST_POSITION, 56 + j*24, 180, 
                        COLOR_TRANS, color, dir_list[k-num_files]);

                    show_icon(screen_address, ICON_DIRECTORY, 29, 53 + j*24);
                }

                show_icon(screen_address, ICON_FILE_ISOLATOR, 17, 72 + j*24);
                k++, j++;
                shift_flag[j]= 1;
            }

            last_in_scroll_value = current_in_scroll_value;
            last_selection_item = current_selection_item;
        }

        if(shift_flag[0])
        {
            if(path_scroll > 0) path_scroll -= 1;
            else if(draw_hscroll(0, -1) <= 0)
            {shift_flag[0]= 0,  path_scroll= 20;}
        }
        else
        {
            if(path_scroll > 0) path_scroll -= 1;
            else if(draw_hscroll(0, 1) <= 0)
            {shift_flag[0]= 1,  path_scroll= 20;}
        }

        flip_screen();
        OSTimeDly(5);     //about 50ms
    }

    for(i= 0; i < (FILE_LIST_ROWS +1); i++)
    {
        draw_hscroll_over(i);
    }

    free((int)filelist_info.file_list);
    free((int)filelist_info.dir_list);
    free((int)filelist_info.filename_mem);
    if(screenp) free((int)screenp);

    clear_screen(COLOR_BLACK);
    flip_screen();
    return return_value;
}

/*--------------------------------------------------------
  放映幻灯片
--------------------------------------------------------*/
u32 play_screen_snapshot()
{
    struct FILE_LIST_INFO filelist_info;
    char *file_ext[] = { ".bmp", NULL };
    u32 file_num;
    char **file_list;
    s32 flag;
    u32 repeat, i;
    u16 *screenp;
    u32 color_bg;

    screenp= (u16*)malloc(256*192*2);
    if(screenp == NULL)
    {
        screenp = screen_address;
        color_bg = COLOR_BG;
    }
    else
    {
        memcpy(screenp, screen_address, 256*192*2);
        color_bg = COLOR16(43, 11, 11);
    }

    if(contsruct_filelist_info(&filelist_info, file_ext, DEFAULT_SS_DIR) < 0)
        return 0;

    flag = load_file_list(&filelist_info);
    file_num = filelist_info.file_num;
    file_list = filelist_info.file_list;

    if(flag < 0 || file_num== 0)
    {
        draw_message(screen_address, screenp, 28, 31, 227, 165, color_bg);
        draw_string_vcenter(screen_address, 29, 55, 198, COLOR_WHITE, msg[MSG_NO_SLIDE]);
        flip_screen();

        if(screenp) free((int)screenp);
        release_filelist_info(&filelist_info);
        if(draw_yesno_dialog(screen_address, 115, "Yes", "No"))
            return 1;
        else 
            return 0;
    }

    char bmp_path[MAX_PATH];
    u32 w, h, x, y;
    u32 buff[256*192];
    u16 screen_buff[256*192];
    char *src;
    u16 *dst;
    u32 time0= 10;
    u32 pause= 0;

    draw_message(screen_address, screenp, 28, 31, 227, 165, color_bg);
    draw_string_vcenter(screen_address, 29, 55, 198, COLOR_WHITE, msg[MSG_PLAY_SLIDE1]);
    draw_string_vcenter(screen_address, 29, 70, 198, COLOR_WHITE, msg[MSG_PLAY_SLIDE2]);
    draw_string_vcenter(screen_address, 29, 85, 198, COLOR_WHITE, msg[MSG_PLAY_SLIDE3]);
    draw_string_vcenter(screen_address, 29, 100, 198, COLOR_WHITE, msg[MSG_PLAY_SLIDE4]);
    draw_string_vcenter(screen_address, 29, 115, 198, COLOR_WHITE, msg[MSG_PLAY_SLIDE5]);
    draw_string_vcenter(screen_address, 29, 130, 198, COLOR_WHITE, msg[MSG_PLAY_SLIDE6]);
    flip_screen();

    repeat= 1;
    i= 0;
    while(repeat)
    {
        sprintf(bmp_path, "%s\\%s", filelist_info.current_path, file_list[i]);

        w= 240, h= 160;
        dst= screen_buff;
        flag= BMP_read(bmp_path, (char*)buff, w, h);
        if(flag == BMP_OK)
        {
            for(y= 0; y< h; y++)
            {
                src= (char*)buff + (h-y-1)*w*4;
                for(x= 0; x < w; x++)
                {
                    *dst++ = RGB24_15(src);
                    src += 4;
                }
            }

            blit_to_screen(screen_buff, w, h, -1, -1);
            flip_gba_screen();
        }

        if(i+1 < file_num) i++;
        else i= 0;

        gui_action_type gui_action;
        u32 ticks= 0;
        u32 time1;

        time1= time0;
        while(ticks < time1)
        {
            gui_action = get_gui_input();

            switch(gui_action)
            {
                case CURSOR_UP:
                    if(!pause)
                    {
                        if(time0 > 1) time0 -= 1;
                        time1= time0;
                    }
                    break;
                    
                case CURSOR_DOWN:
                    if(!pause)
                    {
                        time0 += 1;
                        time1= time0;
                    }
                    break;
                    
                case CURSOR_LEFT:
                    time1 = ticks;
                    if(i > 1) i -= 2;
                    else if(i == 1) i= file_num -1;
                    else i= file_num -2;
                    break;
                
                case CURSOR_RIGHT:
                    time1 = ticks;
                    break;
                
                case CURSOR_SELECT:
                    if(!pause)
                    { 
                        time1 = -1;
                        pause= 1;
                    }
                    else
                    {
                        time1 = ticks;
                        pause= 0;
                    } 
                    break;
                    
                case CURSOR_BACK: 
                    if(screenp) free((int)screenp);
                    release_filelist_info(&filelist_info);
                    return 0;
                    
                default: gui_action= CURSOR_NONE;
                    break;
            }

            OSTimeDly(200/10);
            if(!pause)
                ticks ++;
        }
    }
}

/*--------------------------------------------------------
  搜索指定名称的目录
--------------------------------------------------------*/
int search_dir(char *directory, char* directory_path)
{
    DIR *current_dir;
    struct dirent *current_file;
    char *file_name;
    int directory_path_len;

    current_dir= opendir(directory_path);
    if(current_dir == NULL)
        return -1;

    directory_path_len= strlen(directory_path);
    while(1) {
        current_file = readdir(current_dir);
        if (current_file)
        {
            file_name = current_file->d_name;
            //Is directory 
            if (ISDIR(current_file))
            if (file_name[0] != '.')
            {
                sprintf(directory_path + directory_path_len, "\\%s", file_name);
                if(!strcasecmp(file_name, directory))
                {//dirctory find
                    closedir(current_dir);
                    return 0;
                }

                //dirctory find
                if(search_dir(directory, directory_path) == 0)
                {
                    closedir(current_dir);
                    return 0;
                }

                *(directory_path + directory_path_len)= '\0';
            }
        }
        else
            break;
    }

    closedir(current_dir);
    return -1;
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
    game_config.frameskip_type = 1;   //auto
    game_config.frameskip_value = 2;
    game_config.clock_speed_number = 3;
    game_config.audio_buffer_size_number = 15;
    game_config.update_backup_flag = 0;
    for(i = 0; i < MAX_CHEATS; i++)
    {
        game_config.cheats_flag[i].cheat_active = NO;
        game_config.cheats_flag[i].cheat_name[0] = '\0';
    }
    memcpy(game_config.gamepad_config_map, gamepad_config_map_init, sizeof(gamepad_config_map_init));
    game_config.use_default_gamepad_map = 1;
    game_config.gamepad_config_home = BUTTON_ID_X;
    memcpy(gamepad_config_map, game_config.gamepad_config_map, sizeof(game_config.gamepad_config_map));
    gamepad_config_home = game_config.gamepad_config_home;

    savestate_index= 0;
    for(i= 0; i < 32; i++)
    {
        savestate_map[i]= (char)(-(i+1));
    }
}

/*--------------------------------------------------------
  gpSP cfg的初始化
--------------------------------------------------------*/
void init_default_gpsp_config()
{
//  int temp;
  game_config.frameskip_type = 1;   //auto
  game_config.frameskip_value = 2;
  gpsp_config.screen_ratio = 1; //orignal
  gpsp_config.enable_audio = 1; //on
  //keypad configure
  memcpy(gpsp_config.gamepad_config_map, gamepad_config_map_init, sizeof(gamepad_config_map_init));
  memcpy(gamepad_config_map, gamepad_config_map_init, sizeof(gamepad_config_map_init));
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
  memset(gpsp_config.latest_file, 0, sizeof(gpsp_config.latest_file));

  strcpy(rom_path, main_path);
  gamepak_filename[0] = '\0';
}

/*--------------------------------------------------------
  game cfgファイルの読込み
  3/4修正
--------------------------------------------------------*/
void load_game_config_file(void)
{
    char game_config_filename[MAX_PATH];
    FILE_ID game_config_file;
    char *pt;

    // 设置初始值
    init_game_config();

    sprintf(game_config_filename, "%s\\%s", DEFAULT_CFG_DIR, gamepak_filename);
    pt= strrchr(game_config_filename, '.');
    *pt= 0;
    strcat(game_config_filename, "_0.rts");

    FILE_OPEN(game_config_file, game_config_filename, READ);
    if(FILE_CHECK_VALID(game_config_file))
    {
        //Check file header
        pt= game_config_filename;
        FILE_READ(game_config_file, pt, NGBARTS_HEADERA_SIZE);

        if (!strncmp(pt, NGBARTS_HEADERA, NGBARTS_HEADERA_SIZE))
        {
            FILE_READ_VARIABLE(game_config_file, game_config);
            memcpy(gamepad_config_map, game_config.gamepad_config_map, sizeof(game_config.gamepad_config_map));
            gamepad_config_home = game_config.gamepad_config_home;
        }
        FILE_CLOSE(game_config_file);
    }
}

/*--------------------------------------------------------
  gpSP cfg配置文件
--------------------------------------------------------*/
s32 load_config_file()
{
    char gpsp_config_path[MAX_PATH];
    FILE_ID gpsp_config_file;
    char *pt;

    sprintf(gpsp_config_path, "%s\\%s", main_path, GPSP_CONFIG_FILENAME);
    FILE_OPEN(gpsp_config_file, gpsp_config_path, READ);

    if(FILE_CHECK_VALID(gpsp_config_file))
    {
        // check the file header
        pt= gpsp_config_path;
        FILE_READ(gpsp_config_file, pt, GPSP_CONFIG_HEADER_SIZE);
        pt[GPSP_CONFIG_HEADER_SIZE]= 0;
        if(!strcmp(pt, GPSP_CONFIG_HEADER))
        {
            FILE_READ_VARIABLE(gpsp_config_file, gpsp_config);
            FILE_CLOSE(gpsp_config_file);
            memcpy(gamepad_config_map, gpsp_config.gamepad_config_map, sizeof(gpsp_config.gamepad_config_map));
            return 0;
        }
    }

    // 如果无法读取配置文件，设置缺省值
    init_default_gpsp_config();
    return -1;
}

void initial_gpsp_config()
{
    //Initial directory path
    sprintf(g_default_rom_dir, "%s\\GAMEPAK", main_path);
    sprintf(DEFAULT_SAVE_DIR, "%s\\GAMERTS", main_path);
    sprintf(DEFAULT_CFG_DIR, "%s\\GAMERTS", main_path);
    sprintf(DEFAULT_SS_DIR, "%s\\GAMEPIC", main_path);
    sprintf(DEFAULT_CHEAT_DIR, "%s\\GAMECHT", main_path);
}

/*--------------------------------------------------------
  メニューの表示
--------------------------------------------------------*/
u32 menu(u16 *original_screen)
{
    gui_action_type gui_action= CURSOR_NONE;
    u32 i;
    u32 repeat = 1;
    u32 return_value = 0;
    u32 first_load = 0;
    char current_savestate_filename[MAX_FILE];
    char line_buffer[512];
    char cheat_format_str[MAX_CHEATS][41*4];
    char *cheat_format_ptr[MAX_CHEATS];

    MENU_TYPE *current_menu;
    MENU_OPTION_TYPE *current_option;
    MENU_OPTION_TYPE *display_option;
    
    u32 current_option_num;
    u32 parent_option_num;
    u32 string_select;

    u16 *bg_screenp;
    u32 bg_screenp_color;

  auto void choose_menu();
  auto void menu_return();
  auto void menu_exit();
    auto void touch_menu();
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
  auto void submenu_gamestate_delette();
    auto void menu_delstate_passive();
  auto void submenu_main();
    auto void main_menu_passive();
  auto void reload_cheats_page();
//  auto void set_gamepad();
    auto void delette_savestate();
    auto void save_screen_snapshot();
    auto void browse_screen_snapshot();
    auto void keyremap();
    auto void load_default_setting();
    auto void check_gbaemu_version();
    auto void load_lastest_played();
    auto void load_lastest_played_passive();
    auto void submenu_latest_game();
    auto void language_set();
    auto void game_fastforward();
    auto void sound_switch();
    auto void check_card_space();
    auto void show_card_space();
    auto int plug_mode_load();

  //局部函数定义
    void touch_menu()
    {
        

    }

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

    if(gamepak_filename[0] != 0)
    {
//    if(!gpsp_config.update_backup_flag) // TODO タイミングを検討
        update_backup_force();
        save_game_config_file();
    }

    if(load_file(file_ext, load_filename, g_default_rom_dir) != -1)
    {
       if(load_gamepak(load_filename) == -1)
       {
         quit(0);
       }
       load_game_config_file();
       reset_gba();
       return_value = 1;
       repeat = 0;
       reg[CHANGED_PC_STATUS] = 1;

       reorder_latest_file(); 
       get_savestate_filelist();
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

  void submenu_game_state()
  {
  }

  void submenu_gamestate_delette()
  {
  }

  void clear_savestate_slot(u32 slot_index)
  {
    get_savestate_filename(slot_index, current_savestate_filename);
    sprintf(line_buffer, "%s\\%s", DEFAULT_SAVE_DIR, current_savestate_filename);
    unlink(line_buffer);
    if(savestate_map[slot_index] > 0)
        savestate_map[slot_index]= -savestate_map[slot_index];
    reorder_savestate_slot();
  }

  void menu_save_state()
  {
    if(gui_action == CURSOR_SELECT)
    {
        //The slot is empty
        if(!first_load)
        {
            u32 key;
            s32 slot_index;

            if(bg_screenp != NULL)
            {
                bg_screenp_color = COLOR16(43, 11, 11);
                memcpy(bg_screenp, screen_address, 256*192*2);
            }
            else
                bg_screenp_color = COLOR_BG;
    
            slot_index= get_savestate_slot();
            //the slot already have a savestate file
            if(slot_index >= 31)
            {
                draw_message(screen_address, bg_screenp, 28, 31, 227, 165, bg_screenp_color);
                draw_string_vcenter(screen_address, 29, 55, 198, COLOR_WHITE, msg[MSG_SAVESTATE_FULL]);
                if(draw_yesno_dialog(screen_address, 115, "Yes", "No") == 0)
                    return;

                clear_savestate_slot(0);
            }
            else
                slot_index += 1;


            savestate_map[slot_index]= -savestate_map[slot_index];
            get_savestate_filename(slot_index, current_savestate_filename);

            draw_message(screen_address, bg_screenp, 28, 31, 227, 165, bg_screenp_color);
            draw_string_vcenter(screen_address, 29, 75, 198, COLOR_WHITE, msg[MSG_SAVESTATE_DOING]);

            key= save_state(current_savestate_filename, original_screen);
            //clear message
            draw_message(screen_address, bg_screenp, 28, 31, 227, 96, bg_screenp_color);
            if(key < 1)
            {
                draw_string_vcenter(screen_address, 29, 65, 198, COLOR_WHITE, msg[MSG_SAVESTATE_FAILUER]);
                savestate_map[slot_index]= -savestate_map[slot_index];
            }
            else
            {
                draw_string_vcenter(screen_address, 29, 75, 198, COLOR_WHITE, msg[MSG_SAVESTATE_SUCCESS]);
                savestate_index = slot_index;
            }

            flip_screen();
            OSTimeDly(200/2);
        }
    }
  }
   
  void menu_load_state()
  {
    if(!first_load)
    {
        u32 len;
        FILE *fp= NULL;

        if(bg_screenp != NULL)
        {
            bg_screenp_color = COLOR16(43, 11, 11);
            memcpy(bg_screenp, screen_address, 256*192*2);
        }
        else
            bg_screenp_color = COLOR_BG;

        //Slot not emtpy
        if(savestate_map[savestate_index] > 0)
        {
            get_savestate_filename(savestate_index, current_savestate_filename);
            fp= get_savestate_snapshot(current_savestate_filename);

            //file error
            if(fp == NULL)
            {
                draw_message(screen_address, bg_screenp, 28, 31, 227, 165, bg_screenp_color);
                draw_string_vcenter(screen_address, 29, 80, 198, COLOR_WHITE, msg[MSG_SAVESTATE_FILE_BAD]);
                flip_screen();

                button_up_wait();
                if(gui_action == CURSOR_SELECT)
                {
                    gui_action_type gui_action = CURSOR_NONE;
                    while(gui_action == CURSOR_NONE)
                    {
                        gui_action = get_gui_input();
                        OSTimeDly(200/10);
                    }
                }
                else
                    OSTimeDly(200/2);
                return;
            }

            //right
            if(gui_action == CURSOR_SELECT)
            {
                draw_message(screen_address, bg_screenp, 28, 31, 227, 165, bg_screenp_color); 
                draw_string_vcenter(gba_screen_address, 29, 75, 198, COLOR_WHITE, msg[MSG_LOADSTATE_DOING]);

                len= load_state(current_savestate_filename, fp);
                if(len != 0)
                {
                    return_value = 1;
                    repeat = 0;
                    draw_string_vcenter(screen_address, 29, 75, 198, COLOR_WHITE, msg[MSG_LOADSTATE_SUCCESS]);
                }
                else
                    draw_string_vcenter(screen_address, 29, 75, 198, COLOR_WHITE, msg[MSG_LOADSTATE_FAILURE]);
            }

            fclose(fp);
        }
        else
        {
            clear_gba_screen(COLOR_BLACK);
            draw_string_vcenter(gba_screen_address, 29, 75, 198, COLOR_WHITE, msg[MSG_SAVESTATE_SLOT_EMPTY]);
            flip_gba_screen();
        }
    }
  }

  void menu_load_state_file()
  {
/*    char *file_ext[] = { ".svs", NULL };
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
    }*/
  }

    void savestate_selitem(u32 selected)
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
            if(savestate_map[k] > 0) active= active0+1;
            else active = 0;

            if(selected == k && active0 >0)
            {
                drawbox(screen_address, x+j*14-1, y-1, x+j*14+10, y+10, COLOR16(31, 31, 31));
                if(active== 2) active = 3;
            }
            draw_selitem(screen_address, x + j*14, y, 1, active);

            j++;
        }
    }

    void menu_savestate_passive()
    {
        s32 slot_index;
        u32 line_num;

        strcpy(line_buffer, *(display_option->display_string));
        line_num= display_option-> line_number;
        if(display_option == current_option)
            PRINT_STRING_BG_UTF8(line_buffer, COLOR_ACTIVE_ITEM, COLOR_TRANS, 27, 38 + line_num*20);
        else
            PRINT_STRING_BG_UTF8(line_buffer, COLOR_INACTIVE_ITEM, COLOR_TRANS, 27, 38 + line_num*20);

        slot_index= get_savestate_slot();
        sprintf(line_buffer, "%d", (slot_index+2) > 32 ? 32 : (slot_index+2));
        PRINT_STRING_BG_UTF8(line_buffer, COLOR_INACTIVE_ITEM, COLOR_TRANS, 151,
            38 + 1*20);
        
        if(display_option == current_option)
        {
            show_icon(screen_address, ICON_SUBM_ISOLATOR, 7, 33 + line_num*20);
            show_icon(screen_address, ICON_MENUINDEX, 12, 42 + line_num*20);
            show_icon(screen_address, ICON_SUBM_ISOLATOR, 7, 53 + line_num*20);
            savestate_selitem(slot_index+1);
        }
    }

    void menu_readstate_passive()
    {
        u32 line_num;

        sprintf(line_buffer, *(display_option->display_string),
            *(display_option->current_option)+1);

        line_num= display_option-> line_number;
        if(display_option == current_option)
            PRINT_STRING_BG_UTF8(line_buffer, COLOR_ACTIVE_ITEM, COLOR_TRANS, 27, 38 + line_num*20);
        else
            PRINT_STRING_BG_UTF8(line_buffer, COLOR_INACTIVE_ITEM, COLOR_TRANS, 27, 38 + line_num*20);

        if(display_option == current_option)
        {
            show_icon(screen_address, ICON_SUBM_ISOLATOR, 7, 33 + line_num*20);
            show_icon(screen_address, ICON_MENUINDEX, 12, 42 + line_num*20);
            show_icon(screen_address, ICON_SUBM_ISOLATOR, 7, 53 + line_num*20);
            savestate_selitem(savestate_index);
        }
        else if(current_option_num != 1)
            savestate_selitem(-1);
    }

    u32 delette_savestate_num= 0;
    void menu_delstate_passive()
    {
        u32 line_num;

        sprintf(line_buffer, *(display_option->display_string),
            *(display_option->current_option));

        line_num= display_option-> line_number;
        if(display_option == current_option)
            PRINT_STRING_BG_UTF8(line_buffer, COLOR_ACTIVE_ITEM, COLOR_TRANS, 27, 38 + line_num*20);
        else
            PRINT_STRING_BG_UTF8(line_buffer, COLOR_INACTIVE_ITEM, COLOR_TRANS, 27, 38 + line_num*20);

        if(display_option == current_option)
        {
            show_icon(screen_address, ICON_SUBM_ISOLATOR, 7, 33 + line_num*20);
            show_icon(screen_address, ICON_MENUINDEX, 12, 42 + line_num*20);
            show_icon(screen_address, ICON_SUBM_ISOLATOR, 7, 53 + line_num*20);
            savestate_selitem(delette_savestate_num);
        }
        else
            savestate_selitem(-1);
    }

    void menu_load_cheat_file()
    {
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
                    sprintf(cheat_format_str[i], msg[MSG_CHEAT_MENU_LOADED], i, 
                        game_config.cheats_flag[i].cheat_name);
                }
            }
            choose_menu(current_menu);
        }
        else
        {
            choose_menu(current_menu);
        }
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
        check_card_space();
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

    void submenu_main()
    {
    }

  void delette_savestate()
  {
    if(!first_load && (gui_action == CURSOR_SELECT))
    {
        if(bg_screenp != NULL)
        {
            bg_screenp_color = COLOR16(43, 11, 11);
            memcpy(bg_screenp, screen_address, 256*192*2);
        }
        else
            bg_screenp_color = COLOR_BG;

        button_up_wait();

        if(current_option_num == 1)         //delette all
        {
            u32 i, flag;

            draw_message(screen_address, bg_screenp, 28, 31, 227, 165, bg_screenp_color);
            draw_string_vcenter(screen_address, 29, 75, 198, COLOR_WHITE, msg[MSG_DELETTE_ALL_SAVESTATE_WARING]);

            flag= 0;
            for(i= 0; i < 32; i++)
                if(savestate_map[i] > 0)
                    {flag= 1; break;}

            if(flag)
            {
                if(draw_yesno_dialog(screen_address, 115, "Yes", "No"))
                {
                    for(i= 0; i < 32; i++)
                    {
                        get_savestate_filename(i, current_savestate_filename);
                        sprintf(line_buffer, "%s\\%s", DEFAULT_SAVE_DIR, current_savestate_filename);
                        unlink(line_buffer);
                        if(savestate_map[i] > 0)
                            savestate_map[i]= -savestate_map[i];
                    }
                    savestate_index= 0;
                }
            }
            else
            {
                draw_message(screen_address, bg_screenp, 28, 31, 227, 165, bg_screenp_color);
                draw_string_vcenter(screen_address, 29, 90, 198, COLOR_WHITE, msg[MSG_DELETTE_SAVESTATE_NOTHING]);
                flip_screen();
                OSTimeDly(200/2);
            }
        }
        else if(current_option_num == 2)    //delette single
        {
            draw_message(screen_address, bg_screenp, 28, 31, 227, 165, bg_screenp_color); 

            if(savestate_map[delette_savestate_num] > 0)
            {
                sprintf(line_buffer, msg[MSG_DELETTE_SINGLE_SAVESTATE_WARING], delette_savestate_num);
                draw_string_vcenter(screen_address, 29, 70, 198, COLOR_WHITE, line_buffer);

                if(draw_yesno_dialog(screen_address, 115, "Yes", "No"))
                    clear_savestate_slot(delette_savestate_num);
            }
            else
            {
                draw_string_vcenter(screen_address, 29, 90, 198, COLOR_WHITE, msg[MSG_DELETTE_SAVESTATE_NOTHING]);
                flip_screen();
                OSTimeDly(200/2);
            }
        }
    }
  }

    void save_screen_snapshot()
    {
        if((gui_action == CURSOR_SELECT))
        {
            if(bg_screenp != NULL)
            {
                bg_screenp_color = COLOR16(43, 11, 11);
                memcpy(bg_screenp, screen_address, 256*192*2);
            }
            else
                bg_screenp_color = COLOR_BG;

            draw_message(screen_address, bg_screenp, 28, 31, 227, 165, bg_screenp_color);
            if(!first_load)
            {
                draw_string_vcenter(screen_address, 29, 70, 198, COLOR_WHITE, msg[MSG_SAVE_SNAPSHOT]);
                flip_screen();
                if(save_ss_bmp(original_screen))
                    draw_string_vcenter(screen_address, 29, 90, 198, COLOR_WHITE, msg[MSG_SAVE_SNAPSHOT_COMPLETE]);
                else
                    draw_string_vcenter(screen_address, 29, 90, 198, COLOR_WHITE, msg[MSG_SAVE_SNAPSHOT_FAILURE]);
                flip_screen();
                OSTimeDly(200/2);
            }
            else
            {
                draw_string_vcenter(screen_address, 29, 90, 198, COLOR_WHITE, msg[MSG_SAVESTATE_SLOT_EMPTY]);
                flip_screen();
                OSTimeDly(200/2);
            }
        }
    }

    void browse_screen_snapshot()
    {
        if(current_option_num == 2)
            play_screen_snapshot();
    }

    void keyremap()
    {
        if(gui_action== CURSOR_RIGHT || gui_action== CURSOR_LEFT)
        {
//          if(current_option_num != 0)
//              game_config.use_default_gamepad_map = 0;
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

    void load_default_setting()
    {
        if(bg_screenp != NULL)
        {
            bg_screenp_color = COLOR16(43, 11, 11);
            memcpy(bg_screenp, screen_address, 256*192*2);
        }
        else
            bg_screenp_color = COLOR_BG;

        draw_message(screen_address, bg_screenp, 28, 31, 227, 165, bg_screenp_color);
        draw_string_vcenter(screen_address, 35, 70, 186, COLOR_WHITE, msg[MSG_LOAD_DEFAULT_WARING]);

        if(draw_yesno_dialog(screen_address, 115, "Yes", "No"))
        {
            draw_message(screen_address, bg_screenp, 28, 31, 227, 165, bg_screenp_color);
            draw_string_vcenter(screen_address, 29, 80, 198, COLOR_WHITE, msg[MSG_DEFAULT_LOADING]);
            flip_screen();

            sprintf(line_buffer, "%s\\%s", main_path, GPSP_CONFIG_FILENAME);
            unlink(line_buffer);

            first_load= 1;
            init_default_gpsp_config();
            init_game_config();

            clear_gba_screen(0);
            draw_string_vcenter(gba_screen_address, 0, 80, 256, COLOR_WHITE, msg[MSG_NON_LOAD_GAME]);
            flip_gba_screen();

            OSTimeDly(200/2);
        }
    }

    void check_gbaemu_version()
    {
        if(bg_screenp != NULL)
        {
            bg_screenp_color = COLOR16(43, 11, 11);
            memcpy(bg_screenp, screen_address, 256*192*2);
        }
        else
            bg_screenp_color = COLOR_BG;

        draw_message(screen_address, bg_screenp, 28, 31, 227, 165, bg_screenp_color);
        draw_string_vcenter(screen_address, 29, 80, 198, COLOR_WHITE, msg[MSG_GBA_VERSION0]);
        sprintf(line_buffer, "%s  %s", msg[MSG_GBA_VERSION1], NDSGBA_VERSION);
        draw_string_vcenter(screen_address, 29, 95, 198, COLOR_WHITE, line_buffer);
        flip_screen();

        gui_action_type gui_action = CURSOR_NONE;
        while(gui_action == CURSOR_NONE)
        {
            gui_action = get_gui_input();
            OSTimeDly(200/10);
        }
    }

    void load_lastest_played()
    {
        if(gui_action == CURSOR_LEFT)
        {
            draw_hscroll(current_option_num -1, 5);
        }
        else if(gui_action == CURSOR_RIGHT)
        {
            draw_hscroll(current_option_num -1, -5);
        }
        else if(gui_action == CURSOR_SELECT)
        {
            char *ext_pos;

            if(gamepak_filename[0] != 0)
            {
//              if(!gpsp_config.update_backup_flag) // TODO タイミングを検討
                update_backup_force();

                save_game_config_file();
            }

            if(bg_screenp != NULL)
            {
                bg_screenp_color = COLOR16(43, 11, 11);
                memcpy(bg_screenp, screen_address, 256*192*2);
            }
            else
                bg_screenp_color = COLOR_BG;

            draw_message(screen_address, bg_screenp, 28, 31, 227, 165, bg_screenp_color);
            draw_string_vcenter(screen_address, 30, 75, 196, COLOR_WHITE, msg[MSG_LOADING_GAME]);

            ext_pos= strrchr(gpsp_config.latest_file[current_option_num -1], '\\');
            *ext_pos= '\0';
            strcpy(rom_path, gpsp_config.latest_file[current_option_num -1]);
            *ext_pos= '\\';
            if(load_gamepak(ext_pos+1) == -1)
                return;
            load_game_config_file();
            reset_gba();

            reorder_latest_file();
            get_savestate_filelist();

            get_newest_savestate(current_savestate_filename);
            if(current_savestate_filename[0] != '\0')
            {
                FILE *fp;
                fp= get_savestate_snapshot(current_savestate_filename);
                if(fp != NULL)
                    load_state(current_savestate_filename, fp);
                else
                {
                    reset_gba();
                    reg[CHANGED_PC_STATUS] = 1;
                }
            }
            else
            {
                reset_gba();
                reg[CHANGED_PC_STATUS] = 1;
            }

            return_value = 1;
            repeat = 0;
        }
    }

    void release_lastest_played_scroll()
    {
        u32 k;
        
        for(k= 0; k < 5; k++)
        {
            if(gpsp_config.latest_file[k][0] != '\0')
                draw_hscroll_over(k);
        }
    }

    void load_lastest_played_passive()
    {
        u32 k;

        if(gpsp_config.latest_file[i-1][0] != '\0')
        {
//???????
//            if(current_option_num != i)
//                draw_hscroll(i-1, 1000);
//            else
                draw_hscroll(i-1, 0);
        }

        k= display_option-> line_number;
        if(display_option == current_option)
        {
            show_icon(screen_address, ICON_SUBM_ISOLATOR, 7, 33 + k*20);
            show_icon(screen_address, ICON_MENUINDEX, 12, 42 + k*20);
            show_icon(screen_address, ICON_SUBM_ISOLATOR, 7, 53 + k*20);
        }
    }

    void language_set()
    {
        if(gui_action == CURSOR_LEFT || gui_action == CURSOR_RIGHT)
        {
            if(bg_screenp != NULL)
            {
                bg_screenp_color = COLOR16(43, 11, 11);
                memcpy(bg_screenp, screen_address, 256*192*2);
            }
            else
                bg_screenp_color = COLOR_BG;

            draw_message(screen_address, bg_screenp, 28, 31, 227, 165, bg_screenp_color);
            draw_string_vcenter(screen_address, 30, 75, 196, COLOR_WHITE, msg[MSG_CHANGE_LANGUAGE]);
            draw_string_vcenter(screen_address, 30, 95, 196, COLOR_WHITE, msg[MSG_CHANGE_LANGUAGE_WAITING]);
            flip_screen();

            load_language_msg(LANGUAGE_PACK, gpsp_config.language);    
            gui_change_icon(gpsp_config.language);

            if(first_load)
            {
                clear_gba_screen(0);
                draw_string_vcenter(gba_screen_address, 0, 80, 256, COLOR_WHITE, msg[MSG_NON_LOAD_GAME]);
                flip_gba_screen();
            }

            OSTimeDly(200/2);
        }
    }

    void sound_switch()
    {
        sound_on = gpsp_config.enable_audio & 0x1;
    }

    //typedef struct {
    //  FS_u32 total_clusters;
    //  FS_u32 avail_clusters;
    //  FS_u16 sectors_per_cluster;
    //  FS_u16 bytes_per_sector;
    //} FS_DISKFREE_T;

    FS_DISKFREE_T disk_info;
    void check_card_space()
    {
        fioctl("mmc:", GET_DISK_FREE_SPACE, 0, &disk_info);
    }

    void show_card_space ()
    {
        u32 line_num;
        u32 num_byte;

        strcpy(line_buffer, *(display_option->display_string));
        line_num= display_option-> line_number;        
        PRINT_STRING_BG_UTF8(line_buffer, COLOR_INACTIVE_ITEM, COLOR_TRANS, 27,
            38 + line_num*20);

        num_byte = disk_info.avail_clusters * disk_info.sectors_per_cluster;
        if(num_byte <= 9999*2)
        { /* < 9999KB */
            sprintf(line_buffer, "%d", num_byte/2);
            if(num_byte & 1)
                strcat(line_buffer, ".5 KB");
            else
                strcat(line_buffer, ".0 KB");
        }
        else if(num_byte <= 9999*1024*2)
        { /* < 9999MB */
            num_byte /= 1024;
            sprintf(line_buffer, "%d", num_byte/2);
            if(num_byte & 1)
                strcat(line_buffer, ".5 MB");
            else
                strcat(line_buffer, ".0 MB");
        }
        else
        {
            num_byte /= 1024*1024;
            sprintf(line_buffer, "%d", num_byte/2);
            if(num_byte & 1)
                strcat(line_buffer, ".5 GB");
            else
                strcat(line_buffer, ".0 GB");
        }

        PRINT_STRING_BG_UTF8(line_buffer, COLOR_INACTIVE_ITEM, COLOR_TRANS, 147,
            38 + line_num*20);
    }

    char *screen_ratio_options[] = { &msg[MSG_SCREEN_RATIO_0], &msg[MSG_SCREEN_RATIO_1] };
    
    char *frameskip_options[] = { &msg[MSG_FRAMESKIP_0], &msg[MSG_FRAMESKIP_1] };

    char *on_off_options[] = { &msg[MSG_ON_OFF_0], &msg[MSG_ON_OFF_1] };

    char *sound_seletion[] = { &msg[MSG_SOUND_SWITCH_0], &msg[MSG_SOUND_SWITCH_1] };

    char *snap_frame_options[] = { &msg[MSG_SNAP_FRAME_0], &msg[MSG_SNAP_FRAME_1] };

    char *enable_disable_options[] = { &msg[MSG_EN_DIS_ABLE_0], &msg[MSG_EN_DIS_ABLE_1] };

    char *language_options[] = { &lang[1], &lang[0] };

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
        &gpsp_config.screen_ratio, 2, NULL, PASSIVE_TYPE | HIDEN_TYPE, 1),

    /* 02 */ STRING_SELECTION_OPTION(game_fastforward, NULL, &msg[MSG_SUB_MENU_01], on_off_options, 
        &game_fast_forward, 2, NULL, ACTION_TYPE, 2),

    /* 03 */ STRING_SELECTION_OPTION(NULL, NULL, &msg[MSG_SUB_MENU_02], frameskip_options,
        &game_config.frameskip_type, 2, NULL, PASSIVE_TYPE, 3),

    /* 04 */ NUMERIC_SELECTION_OPTION(NULL, &msg[MSG_SUB_MENU_03], 
        &game_config.frameskip_value, 10, NULL, 4),

    /* 05 */ STRING_SELECTION_OPTION(sound_switch, NULL, &msg[MSG_SUB_MENU_04], on_off_options, 
        &gpsp_config.enable_audio, 2, NULL, ACTION_TYPE ,5)
  };

  MAKE_MENU(graphics, submenu_graphics, NULL);

  /*--------------------------------------------------------
     Game state -- delette
  --------------------------------------------------------*/
  MENU_TYPE game_state_menu;

  MENU_OPTION_TYPE gamestate_delette_options[] =
  {
    /* 00 */ SUBMENU_OPTION(&game_state_menu, &msg[MSG_SUB_MENU_14], NULL, 0),

    /* 01 */ ACTION_OPTION(delette_savestate, NULL, &msg[MSG_SUB_MENU_130], NULL, 1),

    /* 02 */ NUMERIC_SELECTION_ACTION_OPTION(delette_savestate, menu_delstate_passive,
        &msg[MSG_SUB_MENU_131], &delette_savestate_num, 32, NULL, 2)
  };

  MAKE_MENU(gamestate_delette, submenu_gamestate_delette, NULL);

  /*--------------------------------------------------------
     Game state
  --------------------------------------------------------*/
  MENU_OPTION_TYPE game_state_options[] =
  {
    /* 00 */ SUBMENU_OPTION(NULL, &msg[MSG_SUB_MENU_14], NULL, 0),

    /* 01 */ ACTION_OPTION(menu_save_state, menu_savestate_passive, &msg[MSG_SUB_MENU_10], NULL, 1),

    /* 02 */ NUMERIC_SELECTION_ACTION_OPTION(menu_load_state, menu_readstate_passive,
        &msg[MSG_SUB_MENU_11], &savestate_index, 32, NULL, 2),

    /* 03 */ SUBMENU_OPTION(&gamestate_delette_menu, &msg[MSG_SUB_MENU_13], NULL, 5),
  };

  INIT_MENU(game_state, submenu_game_state, NULL);

  /*--------------------------------------------------------
     Cheat options
  --------------------------------------------------------*/
  MENU_OPTION_TYPE cheats_options[] =
  {
    /* 00 */ SUBMENU_OPTION(NULL, &msg[MSG_SUB_MENU_24], NULL,0),

    /* 01 */ CHEAT_OPTION(((CHEATS_PER_PAGE * menu_cheat_page) + 0), 1),
    /* 02 */ CHEAT_OPTION(((CHEATS_PER_PAGE * menu_cheat_page) + 1), 2),
    /* 03 */ CHEAT_OPTION(((CHEATS_PER_PAGE * menu_cheat_page) + 2), 3),

    /* 04 */ NUMERIC_SELECTION_ACTION_OPTION(reload_cheats_page, NULL, &msg[MSG_SUB_MENU_20],
        &menu_cheat_page, MAX_CHEATS_PAGE, NULL, 4),

    /* 05 */ ACTION_OPTION(menu_load_cheat_file, NULL, &msg[MSG_SUB_MENU_21],
        NULL, 5)
  };

  MAKE_MENU(cheats, submenu_cheats, NULL);

  /*--------------------------------------------------------
     Tools-screensanp
  --------------------------------------------------------*/
    MENU_TYPE tools_menu;

    MENU_OPTION_TYPE tools_screensnap_options[] =
    {
        /* 00 */ SUBMENU_OPTION(&tools_menu, &msg[MSG_SUB_MENU_302], NULL, 0),

        /* 01 */ ACTION_OPTION(save_screen_snapshot, NULL, &msg[MSG_SUB_MENU_300], NULL, 1),

        /* 02 */ ACTION_OPTION(browse_screen_snapshot, NULL, &msg[MSG_SUB_MENU_301], NULL, 2)
    };

    MAKE_MENU(tools_screensnap, submenu_screensnap, NULL);

  /*--------------------------------------------------------
     Tools-keyremap
  --------------------------------------------------------*/
    MENU_OPTION_TYPE tools_keyremap_options[] = 
    {
        /* 00 */ SUBMENU_OPTION(&tools_menu, &msg[MSG_SUB_MENU_315], NULL, 0),
        
        /* 01 */ STRING_SELECTION_OPTION(keyremap, keyremap_show, &msg[MSG_SUB_MENU_310], 
            NULL, &string_select, 2, NULL, ACTION_TYPE, 1),
        
        /* 02 */ STRING_SELECTION_OPTION(keyremap, keyremap_show, &msg[MSG_SUB_MENU_311], 
            NULL, &string_select, 1, NULL, ACTION_TYPE, 2),
        
        /* 03 */ STRING_SELECTION_OPTION(keyremap, keyremap_show, &msg[MSG_SUB_MENU_312], 
            NULL, &string_select, 2, NULL, ACTION_TYPE, 3),
        
        /* 04 */ STRING_SELECTION_OPTION(keyremap, keyremap_show, &msg[MSG_SUB_MENU_313], 
            NULL, &string_select, 3, NULL, ACTION_TYPE, 4),
        
        /* 05 */ STRING_SELECTION_OPTION(keyremap, keyremap_show, &msg[MSG_SUB_MENU_314], 
            NULL, &string_select, 3, NULL, ACTION_TYPE, 5)
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

    INIT_MENU(tools, submenu_tools, NULL);
//    tools_menu.init_function = submenu_tools;
//    tools_menu.passive_function = NULL;
//    tools_menu.options = tools_options;
//    tools_menu.num_options= sizeof(tools_options) / sizeof(MENU_OPTION_TYPE);

  /*--------------------------------------------------------
     Others
  --------------------------------------------------------*/
    u32 desert= 0;
  MENU_OPTION_TYPE others_options[] = 
  {
    /* 00 */ SUBMENU_OPTION(NULL, &msg[MSG_BACK], NULL, 0),

    /* 01 */ STRING_SELECTION_OPTION(NULL, NULL, &msg[MSG_SUB_MENU_40], enable_disable_options, 
        &gpsp_config.auto_standby, 2, NULL, PASSIVE_TYPE | HIDEN_TYPE, 1),

    /* 02 */ STRING_SELECTION_OPTION(language_set, NULL, &msg[MSG_SUB_MENU_41], language_options, 
        &gpsp_config.language, 2, NULL, ACTION_TYPE, 2),

    /* 03 */ STRING_SELECTION_OPTION(NULL, show_card_space, &msg[MSG_SUB_MENU_43], NULL, 
        &desert, 2, NULL, PASSIVE_TYPE | HIDEN_TYPE, 3),

//    /* 03 */ STRING_SELECTION_OPTION(NULL, NULL, &msg[MSG_SUB_MENU_42], on_off_options, 
//        &gpsp_config.auto_help, 2, NULL, PASSIVE_TYPE | HIDEN_TYPE, 3),

    /* 04 */ ACTION_OPTION(load_default_setting, NULL, &msg[MSG_SUB_MENU_44], NULL, 4),

    /* 05 */ ACTION_OPTION(check_gbaemu_version, NULL, &msg[MSG_SUB_MENU_45], NULL, 5),
  };

    MAKE_MENU(others, submenu_others, NULL);

  /*--------------------------------------------------------
     Load_game
  --------------------------------------------------------*/
    MENU_TYPE latest_game_menu;
  
    MENU_OPTION_TYPE load_game_options[] =
    {
        /* 00 */ ACTION_OPTION(menu_load, NULL, &msg[MSG_SUB_MENU_61], NULL, 0),

        /* 01 */ SUBMENU_OPTION(&latest_game_menu, &msg[MSG_SUB_MENU_60], NULL, 1),

        /* 02 */ SUBMENU_OPTION(NULL, &msg[MSG_SUB_MENU_62], NULL, 2)
    };

    MAKE_MENU(load_game, submenu_load_game, NULL);

  /*--------------------------------------------------------
     Latest game
  --------------------------------------------------------*/
    MENU_OPTION_TYPE latest_game_options[] =
    {
        /* 00 */ SUBMENU_OPTION(&load_game_menu, &msg[MSG_SUB_MENU_62], NULL, 0),

        /* 01 */ STRING_SELECTION_OPTION(load_lastest_played, load_lastest_played_passive, NULL, NULL,
            &desert, 2, NULL, ACTION_TYPE, 1),

        /* 02 */ STRING_SELECTION_OPTION(load_lastest_played, load_lastest_played_passive, NULL, NULL,
            &desert, 2, NULL, ACTION_TYPE, 2),

        /* 03 */ STRING_SELECTION_OPTION(load_lastest_played, load_lastest_played_passive, NULL, NULL,
            &desert, 2, NULL, ACTION_TYPE, 3),

        /* 04 */ STRING_SELECTION_OPTION(load_lastest_played, load_lastest_played_passive, NULL, NULL,
            &desert, 2, NULL, ACTION_TYPE, 4),

        /* 05 */ STRING_SELECTION_OPTION(load_lastest_played, load_lastest_played_passive, NULL, NULL,
            &desert, 2, NULL, ACTION_TYPE, 5)
    };
    
    INIT_MENU(latest_game, submenu_latest_game, NULL);

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
            case CURSOR_DOWN:
                if(current_option_num >= (main_menu.num_options - 3))
                    current_option_num = 0;
                else
                    current_option_num += 1;

                current_option = current_menu->options + current_option_num;
              break;

            case CURSOR_UP:
                if(current_option_num >= (main_menu.num_options - 3))
                    current_option_num = current_menu->num_options - 4;
                else
                {
                    if(current_option_num > 0)
                        current_option_num -= 1;
                    else
                        current_option_num = current_menu->num_options - 3;
                }

                current_option = current_menu->options + current_option_num;
              break;

            case CURSOR_RIGHT:
                if(current_option_num >= (main_menu.num_options - 3))
                {
                    if(current_option_num < (main_menu.num_options - 1))
                        current_option_num++;
                    else
                        current_option_num = main_menu.num_options - 3;
                }
                else
                    current_option_num = main_menu.num_options - 1;
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
                else
                    current_option_num = main_menu.num_options - 3;
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

    if(current_menu == &latest_game_menu)
        release_lastest_played_scroll();

    current_menu = new_menu;
    current_option_num= 0;
    current_option = new_menu->options;
    if(current_menu->init_function)
        current_menu->init_function();
  }

  void reload_cheats_page()
  {
    for(i = 0; i < CHEATS_PER_PAGE; i++)
    {
      cheats_options[i+1].display_string = &cheat_format_ptr[(CHEATS_PER_PAGE * menu_cheat_page) + i];
      cheats_options[i+1].current_option = &(game_config.cheats_flag[(CHEATS_PER_PAGE * menu_cheat_page) + i].cheat_active);
    }
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
            show_icon(screen_address, ICON_SUBM_ISOLATOR, 7, 33 + line_num*20);
            show_icon(screen_address, ICON_MENUINDEX, 12, 42 + line_num*20);
            show_icon(screen_address, ICON_SUBM_ISOLATOR, 7, 53 + line_num*20);
        }
    }

    void game_fastforward()
    {
        if(game_fast_forward)
        {
            game_config.frameskip_value= 9;
            sound_on= 0;
            graphics_options[3].option_type |= HIDEN_TYPE;
            graphics_options[4].option_type |= HIDEN_TYPE;
            graphics_options[5].option_type |= HIDEN_TYPE;
        }
        else
        {
            game_config.frameskip_value= 2;
            sound_on= gpsp_config.enable_audio & 0x1;
            graphics_options[3].option_type &= ~HIDEN_TYPE;
            graphics_options[4].option_type &= ~HIDEN_TYPE;
            graphics_options[5].option_type &= ~HIDEN_TYPE;
        }
    }

    void submenu_latest_game()
    {
        u32 k;
        char *ext_pos;
        char tmp_path[MAX_PATH];

        sprintf(tmp_path, "%s\\%s", main_path, SUBM_BACKGROUND);
        show_background(screen_address, tmp_path);
        if(bg_screenp != NULL)
            memcpy(bg_screenp, screen_address, 256*192*2);
        print_status(0);

        for(k= 0; k < 5; k++)
        {
            ext_pos= strrchr(gpsp_config.latest_file[k], '\\');
            if(ext_pos != NULL)
                draw_hscroll_init(screen_address, bg_screenp, 27, 38 + (k+1)*20, 200, 
                    COLOR_TRANS, COLOR_INACTIVE_ITEM, ext_pos+1);
            else
                break;
        }

        for(; k < 5; k++)
        {
            latest_game_options[k+1].option_type |= HIDEN_TYPE;
        }
    }

    int plug_mode_load()
    {
        char load_filename[MAX_FILE];

        strcpy(load_filename, gamepak_filename);
        if(load_gamepak(load_filename) == -1)
            return -1;

        load_game_config_file();
        reset_gba();
        return_value = 1;
        repeat = 0;
        reg[CHANGED_PC_STATUS] = 1;

        reorder_latest_file(); 
        get_savestate_filelist();
        return 0;
    }

//----------------------------------------------------------------------------//
  // Menu 处理
//    set_cpu_clock(1);

    button_up_wait();

//    game_fastforward();
    bg_screenp= (u16*)malloc(256*192*2);

    //In plug mode
    if(plug_valid)
    {
        clear_screen(0);
        flip_screen();

        plug_valid= 0;
        if(plug_mode_load() == 0)
        {
            if(bg_screenp != NULL) free((int)bg_screenp);

            clear_screen(0);
            flip_screen();
            return 0;
        }
    }

    for(i = 0; i < MAX_CHEATS; i++)
    {
        if(i >= g_num_cheats)
        {
            sprintf(cheat_format_str[i], msg[MSG_CHEAT_MENU_NON_LOAD], i);
        }
        else
        {
            sprintf(cheat_format_str[i], msg[MSG_CHEAT_MENU_LOADED], i, game_config.cheats_flag[i].cheat_name);
        }
        cheat_format_ptr[i]= cheat_format_str[i];
    }
    reload_cheats_page();

    if(gamepak_filename[0] == 0)
    {
        first_load = 1;
        memset(original_screen, 0x00, 240 * 160 * 2);
        draw_string_vcenter(gba_screen_address, 0, 80, 256, COLOR_WHITE, msg[MSG_NON_LOAD_GAME]);
        flip_gba_screen();
//    PRINT_STRING_EXT_BG(msg[MSG_NON_LOAD_GAME], 0xFFFF, 0x0000, 60, 75, original_screen, 240);
    }

    choose_menu(&main_menu);
    current_menu = &main_menu;

    current_option_num= 6;
    current_option = current_menu->options + current_option_num;     //Default at "NEW"

    // Menu loop
  while(repeat)
  {

    char tmp_path[MAX_PATH];
    if(current_menu == &main_menu)
    {
        sprintf(tmp_path, "%s\\%s", main_path, MENU_BACKGROUND);
        show_background(screen_address, tmp_path);
    }
    else
    {
        sprintf(tmp_path, "%s\\%s", main_path, SUBM_BACKGROUND);
        show_background(screen_address, tmp_path);
    }
    print_status(0);

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
            show_icon(screen_address, ICON_MENU_ISOLATOR, 143, 33 + line_num*16);
            show_icon(screen_address, ICON_MENUINDEX, 150, 39 + line_num*16);
            show_icon(screen_address, ICON_MENU_ISOLATOR, 143, 49 + line_num*16);
          }
        }

        if(display_option++ == current_option)
          show_icon(screen_address, ICON_NEWGAME_SLT, 0, 132);
        else
          show_icon(screen_address, ICON_NEWGAME, 0, 132);

        if(display_option++ == current_option)
          show_icon(screen_address, ICON_RESTGAME_SLT, 70, 132);
        else
          show_icon(screen_address, ICON_RESTGAME, 70, 132);

        if(display_option++ == current_option)
          show_icon(screen_address, ICON_RETGAME_SLT, 168, 132);
        else
          show_icon(screen_address, ICON_RETGAME, 168, 132);
    }
    else
    {
        for(i = 0; i < current_menu->num_options; i++, display_option++)
        {
            unsigned short color;
            u32 line_num;

            if(display_option->passive_function)
                display_option->passive_function();
            else
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
        
                PRINT_STRING_BG_UTF8(line_buffer, color, COLOR_TRANS, 27,
                    38 + line_num*20);
        
                if(display_option == current_option)
                {
                    show_icon(screen_address, ICON_SUBM_ISOLATOR, 7, 33 + line_num*20);
                    show_icon(screen_address, ICON_MENUINDEX, 12, 42 + line_num*20);
                    show_icon(screen_address, ICON_SUBM_ISOLATOR, 7, 53 + line_num*20);
                }
            }
        }
    }

//    PRINT_STRING_BG(current_option->help_string, COLOR_HELP_TEXT, COLOR_BG, 30, 210);

    gui_action = get_gui_input();
    if(gui_action == CURSOR_TOUCH)
    {
    }
    else
    {
    }

    switch(gui_action)
    {
      case CURSOR_DOWN:
        if(current_menu->passive_function)
            current_menu->passive_function();
        else
        {
            current_option_num = (current_option_num + 1) % current_menu->num_options;
            current_option = current_menu->options + current_option_num;

            while(current_option -> option_type & HIDEN_TYPE)
            {
                current_option_num = (current_option_num + 1) % current_menu->num_options;
                current_option = current_menu->options + current_option_num;
            }
        }

        break;

      case CURSOR_UP:
        if(current_menu->passive_function)
            current_menu->passive_function();
        else
        {
            if(current_option_num)
                current_option_num--;
            else
                current_option_num = current_menu->num_options - 1;
            current_option = current_menu->options + current_option_num;

            while(current_option -> option_type & HIDEN_TYPE)
            {
                if(current_option_num)
                    current_option_num--;
                else
                    current_option_num = current_menu->num_options - 1;
                current_option = current_menu->options + current_option_num;
            }
        }

        break;

      case CURSOR_RIGHT:
        if(current_menu->passive_function)
            current_menu->passive_function();
        {
            if(current_option->option_type & (NUMBER_SELECTION_TYPE | STRING_SELECTION_TYPE))
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
        }

        break;

      case CURSOR_LEFT:
        if(current_menu->passive_function)
            current_menu->passive_function();
        else
        {
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
        }

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

      default:
        ;
        break;
    }  // end swith

    flip_screen();
    clear_screen(COLOR_BLACK);
  }  // end while

  clear_screen(0);
  flip_screen();
  clear_gba_screen(0);
  flip_gba_screen();  
  OSTimeDly(200/10);
  clear_screen(0);
  flip_screen();
  clear_gba_screen(0);
  flip_gba_screen();  
  
  if(bg_screenp != NULL) free((int)bg_screenp);

// menu終了時の処理
  button_up_wait();
//  set_cpu_clock(2);

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

  sprintf(savestate_ext, "_%d.rts", (int)slot);
  change_ext(gamepak_filename, name_buffer, savestate_ext);
}

/*--------------------------------------------------------
  游戏的设置文件
--------------------------------------------------------*/
s32 save_game_config_file()
{
    char game_config_filename[MAX_PATH];
    FILE_ID game_config_file;
    char *pt;

    if(gamepak_filename[0] == 0) return -1;

    memcpy(game_config.gamepad_config_map, gamepad_config_map, sizeof(game_config.gamepad_config_map));
    game_config.gamepad_config_home = gamepad_config_home;

    sprintf(game_config_filename, "%s\\%s", DEFAULT_CFG_DIR, gamepak_filename);
    pt = strrchr(game_config_filename, '.');
    *pt = '\0';
    strcat(pt, "_0.rts");

    FILE_OPEN(game_config_file, game_config_filename, WRITE);
    if(FILE_CHECK_VALID(game_config_file))
    {
        FILE_WRITE(game_config_file, NGBARTS_HEADERA, NGBARTS_HEADERA_SIZE);
        FILE_WRITE_VARIABLE(game_config_file, game_config);
        FILE_CLOSE(game_config_file);
        return 0;
    }

    return -1;
}

/*--------------------------------------------------------
  dsGBA 的配置文件
--------------------------------------------------------*/
s32 save_config_file()
{
    char gpsp_config_path[MAX_PATH];
    FILE_ID gpsp_config_file;

    sprintf(gpsp_config_path, "%s\\%s", main_path, GPSP_CONFIG_FILENAME);

    if(game_config.use_default_gamepad_map == 1)
        memcpy(gpsp_config.gamepad_config_map, gamepad_config_map, sizeof(gpsp_config.gamepad_config_map));

    FILE_OPEN(gpsp_config_file, gpsp_config_path, WRITE);
    if(FILE_CHECK_VALID(gpsp_config_file))
    {
        FILE_WRITE(gpsp_config_file, GPSP_CONFIG_HEADER, GPSP_CONFIG_HEADER_SIZE);
        FILE_WRITE_VARIABLE(gpsp_config_file, gpsp_config);
        FILE_CLOSE(gpsp_config_file);
        return 0;
    }

    return -1;
}

/******************************************************************************
 * local function definition
 ******************************************************************************/
void reorder_latest_file(void)
{
    char *pt, *ext_pos, *ext_pos1;
    u32 i, len;
    char full_file[512];
    
    if(gamepak_filename[0] == '\0')
        return;

    for(i= 0; i < 5; i++)
    {
        ext_pos= strrchr(gpsp_config.latest_file[i], '\\');
        if(ext_pos != NULL)
        {
            ext_pos1= strrchr(ext_pos + 1, '.');
            len= ext_pos1 - ext_pos -1;
            if(!strncasecmp(ext_pos + 1, gamepak_filename, len))
                break;
        }
    }
    
    //some one match, ly to last
    if(i < 5)
    {
        if(i < 4)
        {
            strcpy(full_file, gpsp_config.latest_file[i]);
            for(i+=1; i < 5; i++)
            {
                if(gpsp_config.latest_file[i][0] != '\0')
                    strcpy(gpsp_config.latest_file[i-1], gpsp_config.latest_file[i]);
                else
                    break;
            }
            
            strcpy(gpsp_config.latest_file[i-1], full_file);
        }
        return ;
    }
     
    //none match
    for(i= 0; i < 5; i++)
    {
        if(gpsp_config.latest_file[i][0] == '\0')
        {
            sprintf(gpsp_config.latest_file[i], "%s\\%s", rom_path, gamepak_filename);
            break;
        }
    }
    
    if(i < 5) return;
    
    //queue full
    for(i=1; i < 5; i++)
        strcpy(gpsp_config.latest_file[i-1], gpsp_config.latest_file[i]);
        
    sprintf(gpsp_config.latest_file[i-1], "%s\\%s", rom_path, gamepak_filename);
}


static int rtc_time_cmp(struct rtc *t1, struct rtc *t2)
{
    int result;

    result= (int)((unsigned int)(t1 -> year) - (unsigned int)(t2 -> year));
    if(result != 0)
        return result;
    result= (int)((unsigned int)(t1 -> month) - (unsigned int)(t2 -> month));
    if(result != 0)
        return result;
    result= (int)((unsigned int)(t1 -> day) - (unsigned int)(t2 -> day));
    if(result != 0)
        return result;
    result= (int)((unsigned int)(t1 -> weekday) - (unsigned int)(t2 -> weekday));
    if(result != 0)
        return result;
    result= (int)((unsigned int)(t1 -> hours) - (unsigned int)(t2 -> hours));
    if(result != 0)
        return result;
    result= (int)((unsigned int)(t1 -> minutes) - (unsigned int)(t2 -> minutes));
    if(result != 0)
        return result;
    result= (int)((unsigned int)(t1 -> seconds) - (unsigned int)(t2 -> seconds));
    if(result != 0)
        return result;    
}

static void get_savestate_filelist(void)
{
    struct rtc  current_time[32];
    int i;
    char savestate_path[MAX_PATH];
    char postdrix[8];
    char *pt;
    FILE *fp;

//    char time_st[32];

    memset((char*)current_time, 0xFF, sizeof(current_time));
    for(i= 0; i < 32; i++)
    {
        savestate_map[i]= (char)(-(i+1));
    }
/*
    for(i= 0; i < 32; i++)
    {
        printf("%d  %d\n", i, savestate_map[i]);
    }
*/
    sprintf(savestate_path, "%s\\%s", DEFAULT_SAVE_DIR, gamepak_filename);
    pt= strrchr(savestate_path, '.');
    for(i= 0; i < 32; i++)
    {
        sprintf(postdrix, "_%d.rts", i+1);
        strcpy(pt, postdrix);
        
        fp= fopen(savestate_path, "r");
        if (fp != NULL)
        {
            if(fseek(fp, RTS_TIMESTAMP_POS, SEEK_SET))
            { /* Read back the time stamp */
                fread((char*)&current_time[i], 1, sizeof(struct rtc), fp);
                savestate_map[i] = (char)(i+1);

//                get_time_string(time_st, &current_time[i]);
//                printf("%d  %s\n", i, time_st);
            }
            fclose(fp);
        }
    }

    int k, reslut;
    struct rtc current_time_tmp;
    for(i= 0; i < 32-1; i++)
    {
        for(k=0; k < 32-1-i; k++)
        {
            reslut= rtc_time_cmp(&current_time[k], &current_time[k+1]);
            if(reslut > 0)
            {
               current_time_tmp = current_time[k];
               current_time[k] = current_time[k+1];
               current_time[k+1] = current_time_tmp;
               reslut = savestate_map[k];
               savestate_map[k] = savestate_map[k+1];
               savestate_map[k+1] = reslut;
            }
        }
    }
/*
    for(i= 0; i < 32; i++)
    {
        printf("%d  %d  ", i, savestate_map[i]);
        get_time_string(time_st, &current_time[i]);
        printf("%s\n", time_st);
    }
*/
    savestate_index= get_savestate_slot();
    if(savestate_index < 0) savestate_index = 0;
}

static FILE* get_savestate_snapshot(char *savestate_filename)
{
    char savestate_path[MAX_PATH];
    struct rtc  current_time;
    u16 snapshot_buffer[240 * 160];
    FILE *fp;
    char *pt;

    clear_gba_screen(COLOR_BLACK);

    sprintf(savestate_path, "%s\\%s", DEFAULT_SAVE_DIR, savestate_filename);
    fp= fopen(savestate_path, "r");
    if(fp == NULL)
    {
printf("Open %s failure\n", savestate_path);
        goto get_savestate_snapshot_error;
    }

    if(SVS_FILE_SIZE != file_length(fp)) goto get_savestate_snapshot_error;

    pt= savestate_path;
    fread(pt, 1, SVS_HEADER_SIZE, fp);

    pt[SVS_HEADER_SIZE]= 0;
    if(strcmp(pt, SVS_HEADER)) goto get_savestate_snapshot_error;

    fread((char*)&current_time, 1, sizeof(struct rtc), fp);
    fread((char*)snapshot_buffer, 1, 240*160*2, fp);

    pt[0]= 0, pt[1]= 0;
    fread(pt, 1, 2, fp);

    if(pt[0] != 0x5A || pt[1] != 0x3C) goto get_savestate_snapshot_error;

    get_time_string(pt, &current_time);
    PRINT_STRING_GBA_UTF8(pt, COLOR_WHITE, COLOR_TRANS, 5, 0);

    blit_to_screen(snapshot_buffer, 240, 160, -1, -1);
    flip_gba_screen();
    return fp;

get_savestate_snapshot_error:
    if(fp != NULL)  fclose(fp);
    draw_string_vcenter(gba_screen_address, 29, 75, 198, COLOR_WHITE, msg[MSG_SAVESTATE_SLOT_EMPTY]);
    flip_gba_screen();
printf("Read %s failure\n", savestate_path);
    return NULL;
}

static void get_savestate_filename(u32 slot, char *name_buffer)
{
  char savestate_ext[16];

  sprintf(savestate_ext, "_%d.rts", savestate_map[slot]);
  change_ext(gamepak_filename, name_buffer, savestate_ext);
}

static u32 get_savestate_slot(void)
{
    s32 i;
    char *ptr;

    i= 31;
    ptr= savestate_map;
    while(i >= 0)
    {
        if(ptr[i] > 0) break;
        i--;
    }

    return i;
}

static void reorder_savestate_slot(void)
{
    u32 x, y;
    char *ptr;
    s32 tmp;
    
    ptr= savestate_map;
    for(x= 0; x < 32; x++)
    {
        tmp= ptr[x];
        if(tmp< 0)
        {
            for(y= x+1; y< 32; y++)
                ptr[y-1]= ptr[y];
            ptr[31]= tmp;
        }
    }
}

void get_newest_savestate(char *name_buffer)
{
    int i;

    i= get_savestate_slot();
    if (i < 0)
    {
        name_buffer[0]= '\0';
        return;
    }

    get_savestate_filename(i, name_buffer);
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
    char print_buffer_1[256];
//  char print_buffer_2[256];
//  char utf8[1024];
    struct rtc  current_time;
//  pspTime current_time;

    get_nds_time(&current_time);
//  sceRtcGetCurrentClockLocalTime(&current_time);
//  int wday = sceRtcGetDayOfWeek(current_time.year, current_time.month , current_time.day);

//    get_timestamp_string(print_buffer_1, MSG_MENU_DATE_FMT_0, current_time.year+2000, 
//        current_time.month , current_time.day, current_time.weekday, current_time.hours,
//        current_time.minutes, current_time.seconds, 0);
//    sprintf(print_buffer_2,"%s%s", msg[MSG_MENU_DATE], print_buffer_1);
    get_time_string(print_buffer_1, &current_time);
    PRINT_STRING_BG(print_buffer_1, COLOR_BLACK, COLOR_TRANS, 44, 176);
    PRINT_STRING_BG(print_buffer_1, COLOR_HELP_TEXT, COLOR_TRANS, 43, 175);

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

    sprintf(buffer, "%s %02d/%02d/%04d %02d:%02d:%02d", weekday_strings[wday], 
        day, mon, year, hour, min, sec);
}

static void get_time_string(char *buff, struct rtc *rtcp)
{
    get_timestamp_string(buff, NULL,
                            rtcp -> year +2000,
                            rtcp -> month,
                            rtcp -> day,
                            rtcp -> weekday,
                            rtcp -> hours,
                            rtcp -> minutes,
                            rtcp -> seconds,
                            0);
}

static u32 save_ss_bmp(u16 *image)
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
    sprintf(save_ss_path, "%s\\%s%02d%02d%02d%02d%02d.bmp", DEFAULT_SS_DIR, ss_filename, 
    current_time.month, current_time.day, current_time.hours, current_time.minutes, current_time.seconds);

    for(y = 0; y < 160; y++)
    {
        for(x = 0; x < 240; x++)
        {
            col = image[x + y * 240];
            r = (col >> 10) & 0x1F;
            g = (col >> 5) & 0x1F;
            b = (col) & 0x1F;

            rgb_data[(159-y)*240*3+x*3+2] = b << 3;
            rgb_data[(159-y)*240*3+x*3+1] = g << 3;
            rgb_data[(159-y)*240*3+x*3+0] = r << 3;
        }
    }

    FILE *ss = fopen( save_ss_path, "wb" );
    if( ss == NULL ) return 0;

    fwrite( header, sizeof(header), 1, ss );
    fwrite( rgb_data, 240*160*3, 1, ss);
    fclose( ss );

    return 1;
}

void _flush_cache()
{
//    sceKernelDcacheWritebackAll();
    invalidate_all_cache();
}
