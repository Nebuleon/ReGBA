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

#include <ds2/ds.h>
#include <limits.h>  /* For PATH_MAX */
#include "bitmap.h"
#include "cheats.h"

// For general option text
#define OPTION_TEXT_X             10
#define OPTION_TEXT_SX            236

// For option rows
#define GUI_ROW1_Y                36
#define GUI_ROW_SY                19
// The following offset is added to the row's Y coordinate to provide
// the Y coordinate for its text.
#define TEXT_OFFSET_Y             2
// The following offset is added to the row's Y coordinate to provide
// the Y coordinate for its ICON_SUBSELA (sub-screen selection type A).
#define SUBSELA_OFFSET_Y          -2
#define SUBSELA_X                 ((DS_SCREEN_WIDTH - ICON_SUBSELA.x) / 2)

// For message boxes
#define MESSAGE_BOX_TEXT_X        ((DS_SCREEN_WIDTH - ICON_MSG.x) / 2 + 3)
#define MESSAGE_BOX_TEXT_SX       (ICON_MSG.x - 6)
// Y is brought down by the "window title" that's part of ICON_MSG
#define MESSAGE_BOX_TEXT_Y        ((DS_SCREEN_HEIGHT - ICON_MSG.y) / 2 + 24)

// For cheats [ NUM. DESC . . . . . +/- ]
#define CHEAT_NUMBER_X            10
#define CHEAT_DESC_X              34
#define CHEAT_DESC_SX             197
#define CHEAT_ACTIVE_X            241

// For the file selector
#define FILE_SELECTOR_ICON_X      10
#define FILE_SELECTOR_ICON_Y      (TEXT_OFFSET_Y - 1)
#define FILE_SELECTOR_NAME_X      32
#define FILE_SELECTOR_NAME_SX     214

// Back button
#define BACK_BUTTON_X             224
#define BACK_BUTTON_Y             10
#define BACK_BUTTON_TOUCH_X       216  /* ... and going right */
#define BACK_BUTTON_TOUCH_Y       33   /* ... and going up */
// Title icon
#define TITLE_ICON_X              12
#define TITLE_ICON_Y              9

#define BUTTON_REPEAT_START (CLOCKS_PER_SEC / 2) /* 1/2 of a second */
#define BUTTON_REPEAT_CONTINUE (CLOCKS_PER_SEC / 20) /* 1/20 of a second */

#define MAX_GAMEPAD_CONFIG_MAP 16

typedef enum
{
  BUTTON_NOT_HELD,
  BUTTON_HELD_INITIAL,
  BUTTON_HELD_REPEAT
} button_repeat_state_type;

// Runtime settings for the emulator. Not persistent.
typedef struct
{
  uint32_t screen_ratio;
  uint32_t enable_audio;
  uint32_t auto_standby;
  uint32_t auto_help;
  uint32_t analog_sensitivity_level;
  uint32_t enable_home;
  uint32_t emulate_core;
  uint32_t debug_flag;
  uint32_t fake_fat;
  char rom_file[256];
  char rom_path[256];
} GPSP_CONFIG;

// Persistent settings for the emulator.
typedef struct
{
  uint32_t language;
  char latest_file[5][512];
  uint32_t HotkeyRewind;
  uint32_t HotkeyReturnToMenu;
  uint32_t HotkeyToggleSound;
  uint32_t HotkeyTemporaryFastForward;
  uint32_t HotkeyQuickLoadState;
  uint32_t HotkeyQuickSaveState;
  /*
   * These contain DS button bitfields, each having 1 bit set,
   * corresponding to the 6 remappable GBA buttons and 2 specials:
   * [0] = A          [1] = B          [2] = SELECT
   * [3] = START      [4] = R          [5] = L
   * [6] = Rapid A    [7] = Rapid B    (6 and 7 can be unset)
   */
  uint32_t ButtonMappings[8];
  uint32_t DisplayFPS;
  uint32_t BottomScreenGame;
  uint32_t BootFromBIOS;
  uint32_t Reserved2[111];
} GPSP_CONFIG_FILE;

// Runtime settings for the current game. Not persistent and reset between
// games.
typedef struct
{
  uint32_t frameskip_type;
  uint32_t frameskip_value;
  uint32_t audio_buffer_size_number;
  CHEAT_TYPE cheats_flag[MAX_CHEATS];
  uint32_t gamepad_config_map[MAX_GAMEPAD_CONFIG_MAP];
  uint32_t backward;
  uint32_t backward_time;
} GAME_CONFIG;

// Persistent settings for the current game.
typedef struct
{
  /*
   * This value differs from the frameskip_type and frameskip_value in
   * GAME_CONFIG in that this one is just one value, for the GUI, and it's
   * split into two for the runtime settings in GAME_CONFIG.
   */
  uint32_t frameskip_value;
  uint32_t clock_speed_number;
  /*
   * This value differs from the backward and backward_time values in
   * GAME_CONFIG in that this one is just one value, for the GUI, and it's
   * split into two for the runtime settings in GAME_CONFIG.
   */
  uint32_t rewind_value;
  uint32_t HotkeyRewind;
  uint32_t HotkeyReturnToMenu;
  uint32_t HotkeyToggleSound;
  uint32_t HotkeyTemporaryFastForward;
  uint32_t HotkeyQuickLoadState;
  uint32_t HotkeyQuickSaveState;
  /*
   * These contain DS button bitfields, each having 1 or no bits set,
   * corresponding to the 6 remappable GBA buttons and 2 specials:
   * [0] = A          [1] = B          [2] = SELECT
   * [3] = START      [4] = R          [5] = L
   * [6] = Rapid A    [7] = Rapid B
   */
  uint32_t ButtonMappings[8];
  uint32_t Reserved1[113];
} GAME_CONFIG_FILE;

typedef enum
{
  CURSOR_UP,
  CURSOR_DOWN,
  CURSOR_LEFT,
  CURSOR_RIGHT,
  CURSOR_SELECT,
  CURSOR_BACK,
  CURSOR_EXIT,
  CURSOR_NONE,
  CURSOR_RTRIGGER,
  CURSOR_LTRIGGER,
  CURSOR_KEY_SELECT,
  CURSOR_TOUCH
} gui_action_type;

struct  FILE_LIST_INFO
{
    char current_path[PATH_MAX];
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
extern char g_default_rom_dir[PATH_MAX];
extern char DEFAULT_SAVE_DIR[PATH_MAX];
extern char DEFAULT_CFG_DIR[PATH_MAX];
extern char DEFAULT_SS_DIR[PATH_MAX];
extern char DEFAULT_CHEAT_DIR[PATH_MAX];

extern GPSP_CONFIG gpsp_config;
extern GAME_CONFIG game_config;

extern GPSP_CONFIG_FILE gpsp_persistent_config;
extern GAME_CONFIG_FILE game_persistent_config;

#define SKIP_RATE (game_config.frameskip_value)
#define AUTO_SKIP (game_config.frameskip_type)
/******************************************************************************
 * グローバル関数の宣言
 ******************************************************************************/
int32_t load_file(char **wildcards, char *result, char *default_dir_name);
void load_game_config_file(void);
int32_t load_config_file();
int32_t save_game_config_file();
int32_t save_config_file();

gui_action_type get_gui_input();

int load_game_stat_snapshot(const char* file);
uint32_t save_menu_ss_bmp(const uint16_t *image);

int gui_init(uint32_t lang_id);
uint32_t load_dircfg(char *file_name);
uint32_t load_fontcfg(char *file_name);
//uint32_t load_msgcfg(char *file_name);
extern int load_language_msg(const char *filename, uint32_t language);
int load_font();
void initial_gpsp_config();
void init_game_config();
extern void reorder_latest_file(const char* GamePath);

extern void game_set_frameskip(void);
extern void game_set_rewind(void);
extern void set_button_map(void);

extern void LowFrequencyCPU(void);
extern void HighFrequencyCPU(void);
extern void GameFrequencyCPU(void);

extern void QuickSaveState(void);
extern void QuickLoadState(void);

#endif

