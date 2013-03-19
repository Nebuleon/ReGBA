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
#include "cheats.h"

#define UP_SCREEN_UPDATE_METHOD   0
#define DOWN_SCREEN_UPDATE_METHOD 2

// For general option text
#define OPTION_TEXT_X             10
#define OPTION_TEXT_SX            236

// For message boxes
#define MESSAGE_BOX_TEXT_X        ((NDS_SCREEN_WIDTH - ICON_MSG.x) / 2 + 3)
#define MESSAGE_BOX_TEXT_SX       (ICON_MSG.x - 6)
// Y is brought down by the "window title" that's part of ICON_MSG
#define MESSAGE_BOX_TEXT_Y        ((NDS_SCREEN_HEIGHT - ICON_MSG.y) / 2 + 24)

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

// Runtime settings for the emulator. Not persistent.
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
  u32 emulate_core;
  u32 debug_flag;
  u32 fake_fat;
  char rom_file[256];
  char rom_path[256];
} GPSP_CONFIG;

// Persistent settings for the emulator.
typedef struct
{
  u32 language;
  char latest_file[5][512];
  u32 Reserved0[128];
} GPSP_CONFIG_FILE;

// Runtime settings for the current game. Not persistent and reset between
// games.
typedef struct
{
  u32 frameskip_type;
  u32 frameskip_value;
  u32 audio_buffer_size_number;
  u32 update_backup_flag;
  CHEAT_TYPE cheats_flag[MAX_CHEATS];
  u32 gamepad_config_map[MAX_GAMEPAD_CONFIG_MAP];
  u32 gamepad_config_home;
  u32 use_default_gamepad_map;
} GAME_CONFIG;

// Persistent settings for the current game.
typedef struct
{
  /*
   * This value differs from the frameskip_type and frameskip_value in
   * GAME_CONFIG in that this one is just one value, for the GUI, and it's
   * split into two for the runtime settings in GAME_CONFIG.
   */
  u32 frameskip_value;
  u32 clock_speed_number;
  u32 Reserved[128];
} GAME_CONFIG_FILE;

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

extern GPSP_CONFIG_FILE gpsp_persistent_config;
extern GAME_CONFIG_FILE game_persistent_config;

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

