/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
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
 * gui.h
 * gui周りの処理
 ******************************************************************************/
#ifndef GUI_H
#define GUI_H

#include "ds2io.h"
#include "bitmap.h"

#define UP_SCREEN_UPDATE_METHOD   0
#define DOWN_SCREEN_UPDATE_METHOD 2

// For general option text
#define OPTION_TEXT_X             10
#define OPTION_TEXT_SX            236

// For cheats [ NUM. DESC . . . . . +/- ]
#define CHEAT_NUMBER_X            10
#define CHEAT_DESC_X              34
#define CHEAT_DESC_SX             197
#define CHEAT_ACTIVE_X            241

// For the file selector
#define FILE_SELECTOR_ICON_X      10
#define FILE_SELECTOR_NAME_X      32
#define FILE_SELECTOR_NAME_SX     214

#define MAX_GAMEPAD_CONFIG_MAP 16


//
typedef struct
{
  u32 screen_ratio;
  u32 enable_audio;
  u32 auto_standby;
  u32 auto_help;
  u32 analog_sensitivity_level;
  u32 enable_home;
  u32 gamepad_config_map[MAX_GAMEPAD_CONFIG_MAP];
  u32 gamepad_config_home;
  u32 language;
  u32 emulate_core;
  u32 debug_flag;
  u32 fake_fat;
  char rom_file[256];
  char rom_path[256];
  char latest_file[5][512];
} GPSP_CONFIG;

typedef struct
{
  u32 frameskip_type;
  u32 frameskip_value;
  u32 clock_speed_number;
  u32 audio_buffer_size_number;
  u32 update_backup_flag;
	// Disabled [Neb]
  //CHEAT_TYPE cheats_flag[MAX_CHEATS];
  u32 gamepad_config_map[MAX_GAMEPAD_CONFIG_MAP];
  u32 gamepad_config_home;
  u32 use_default_gamepad_map;
} GAME_CONFIG;

struct  FILE_LIST_INFO
{
    char current_path[MAX_PATH];
    char **wildcards;
    unsigned int file_num;
    unsigned int dir_num;
	unsigned int mem_size;
	unsigned int file_size;
	unsigned int dir_size;
    char **file_list;
    char **dir_list;
    char *filename_mem;
};

#define ASM_CORE 0
#define C_CORE   1

/******************************************************************************
 * グローバル変数の宣言
 ******************************************************************************/
extern char g_default_rom_dir[MAX_PATH];
extern char DEFAULT_SAVE_DIR[MAX_PATH];
extern char DEFAULT_CFG_DIR[MAX_PATH];
extern char DEFAULT_SS_DIR[MAX_PATH];
extern char DEFAULT_CHEAT_DIR[MAX_PATH];

extern char main_path[MAX_PATH];

extern GPSP_CONFIG gpsp_config;
extern GAME_CONFIG game_config;

#define SKIP_RATE (game_config.frameskip_value)
#define AUTO_SKIP (game_config.frameskip_type)
/******************************************************************************
 * グローバル関数の宣言
 ******************************************************************************/
s32 load_file(char **wildcards, char *result, char *default_dir_name);
int search_dir(char *directory, char* directory_path);
void load_game_config_file(void);
s32 load_config_file();
s32 save_game_config_file();
s32 save_config_file();

u32 menu(u16 *original_screen, int FirstInvocation);

u32 load_dircfg(char *file_name);
u32 load_fontcfg(char *file_name);
//u32 load_msgcfg(char *file_name);
extern int load_language_msg(char *filename, u32 language);
u32 load_font();
void get_savestate_filename_noshot(u32 slot, char *name_buffer);
extern  void get_newest_savestate(char *name_buffer);
void initial_gpsp_config();
void init_game_config();
extern void reorder_latest_file(void);

extern void LowFrequencyCPU();
extern void HighFrequencyCPU();
extern void GameFrequencyCPU();

#endif

