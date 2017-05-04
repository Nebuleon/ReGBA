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
#include <ds2/ioext.h>
#include <ds2/pm.h>

#include "common.h"
#include "gui.h"
#include "input.h"
#include "draw.h"
#include "cheats.h"

// Program arguments.
char argv[2][PATH_MAX];

// If adding a language, make sure you update the size of the array in
// message.h too.
const char *lang[LANG_END] = {
	"English",          // 0
	"简体中文",          // 1
	"Français",         // 2
	"Deutsch",          // 3
	"Nederlands",       // 4
	"Español",          // 5
	"Italiano",         // 6
	"Português (Br.)",  // 7
	"繁體中文",          // 8
};

const char *msg[MSG_END + 1];
char msg_data[32 * 1024] __attribute__((section(".noinit")));

/******************************************************************************
 * 宏定义
 ******************************************************************************/
#define STATUS_ROWS 0
#define CURRENT_DIR_ROWS 2
//#define FILE_LIST_ROWS 25
#define FILE_LIST_ROWS SUBMENU_ROW_NUM
#define FILE_LIST_ROWS_CENTER   (FILE_LIST_ROWS/2)
#define FILE_LIST_POSITION 50
//#define DIR_LIST_POSITION 360
#define DIR_LIST_POSITION 130
#define PAGE_SCROLL_NUM SUBMENU_ROW_NUM

#define SUBMENU_ROW_NUM	8

#define SAVE_STATE_SLOT_NUM 16

#define NDSGBA_VERSION "1.45"

#define GPSP_CONFIG_FILENAME "SYSTEM/ndsgba.cfg"

// dsGBA的一套头文件
// 标题 4字节
#define GPSP_CONFIG_HEADER  "NGBA1.1"
#define GPSP_CONFIG_HEADER_SIZE 7
const uint32_t gpsp_config_ver = 0x00010001;
GPSP_CONFIG gpsp_config;
GPSP_CONFIG_FILE gpsp_persistent_config;

// GAME的一套头文件
// 标题 4字节
#define GAME_CONFIG_HEADER     "GAME1.1"
#define GAME_CONFIG_HEADER_SIZE 7
// #define GAME_CONFIG_HEADER_U32 0x67666367
const uint32_t game_config_ver = 0x00010001;
GAME_CONFIG game_config __attribute__((section(".noinit")));
GAME_CONFIG_FILE game_persistent_config __attribute__((section(".noinit")));

//save state file map
#define RTS_TIMESTAMP_POS   SVS_HEADER_SIZE
static uint32_t savestate_index; // current selection in the saved states menu
static int32_t latest_save; // Slot number of the latest (in time) save for this game, or -1 if none
static bool SavedStateExistenceCached[SAVE_STATE_SLOT_NUM]; // [I] == TRUE if Cache[I] is meaningful
static bool SavedStateExistenceCache[SAVE_STATE_SLOT_NUM] __attribute__((section(".noinit")));

// These are U+05C8 and subsequent codepoints encoded in UTF-8.
// They are null-terminated.
const char HOTKEY_A_DISPLAY[] = "\xD7\x88";
const char HOTKEY_B_DISPLAY[] = "\xD7\x89";
const char HOTKEY_X_DISPLAY[] = "\xD7\x8A";
const char HOTKEY_Y_DISPLAY[] = "\xD7\x8B";
const char HOTKEY_L_DISPLAY[] = "\xD7\x8C";
const char HOTKEY_R_DISPLAY[] = "\xD7\x8D";
const char HOTKEY_START_DISPLAY[] = "\xD7\x8E";
const char HOTKEY_SELECT_DISPLAY[] = "\xD7\x8F";
// These are U+2190 and subsequent codepoints encoded in UTF-8.
const char HOTKEY_LEFT_DISPLAY[] = "\xE2\x86\x90";
const char HOTKEY_UP_DISPLAY[] = "\xE2\x86\x91";
const char HOTKEY_RIGHT_DISPLAY[] = "\xE2\x86\x92";
const char HOTKEY_DOWN_DISPLAY[] = "\xE2\x86\x93";

#ifdef TEST_MODE
#define VER_RELEASE "test"
#else
#define VER_RELEASE "release"
#endif

/******************************************************************************
 * 定义全局变量
 ******************************************************************************/
char g_default_rom_dir[PATH_MAX] __attribute__((section(".noinit")));
char DEFAULT_SAVE_DIR[PATH_MAX] __attribute__((section(".noinit")));
char DEFAULT_CFG_DIR[PATH_MAX] __attribute__((section(".noinit")));
char DEFAULT_SS_DIR[PATH_MAX] __attribute__((section(".noinit")));
char DEFAULT_CHEAT_DIR[PATH_MAX] __attribute__((section(".noinit")));
uint32_t game_fast_forward = 0;  // off
uint32_t temporary_fast_forward = 0;  // also off, controlled by the hotkey

/******************************************************************************
 * 本地函数的声明
 ******************************************************************************/
static void get_savestate_filelist(const char* GamePath);
static uint8_t SavedStateSquareX(uint32_t slot);
static bool SavedStateFileExists(uint32_t slot);
static void SavedStateCacheInvalidate (void);
void get_newest_savestate(char *name_buffer);
static uint32_t save_ss_bmp(const uint16_t *image);
static void init_default_gpsp_config();

void game_set_frameskip();

/*--------------------------------------------------------
	GBA key input
--------------------------------------------------------*/

bool SoundHotkeyWasHeld = false;
bool LoadStateWasHeld = false;
bool SaveStateWasHeld = false;

uint32_t fast_backward = 0;
uint32_t last_buttons;

enum ReGBA_Buttons ReGBA_GetPressedButtons()
{
	struct DS_InputState inputdata;
	uint32_t i;
	DS2_GetInputState(&inputdata);

	uint32_t buttons = inputdata.buttons;
	uint32_t non_repeat_buttons = (last_buttons ^ buttons) & buttons;
	last_buttons = buttons;

	enum ReGBA_Buttons Result = 0;

	// Lid closed? (DS)
	if (buttons & DS_BUTTON_LID)
	{
		LowFrequencyCPU();
		DS2_SystemSleep();
		GameFrequencyCPU();
	}

	// Update sound toggle.
	uint32_t HotkeyToggleSound = game_persistent_config.HotkeyToggleSound != 0 ? game_persistent_config.HotkeyToggleSound : gpsp_persistent_config.HotkeyToggleSound;

	bool SoundHotkeyIsHeld = HotkeyToggleSound && (buttons & HotkeyToggleSound) == HotkeyToggleSound;
	if (!SoundHotkeyWasHeld && SoundHotkeyIsHeld)
	{
		sound_on ^= 1;
	}
	SoundHotkeyWasHeld = SoundHotkeyIsHeld;

	// Update fast-forwarding.
	uint32_t HotkeyTemporaryFastForward = game_persistent_config.HotkeyTemporaryFastForward != 0 ? game_persistent_config.HotkeyTemporaryFastForward : gpsp_persistent_config.HotkeyTemporaryFastForward;

	uint32_t WasFastForwarding = temporary_fast_forward;
	temporary_fast_forward = HotkeyTemporaryFastForward && (buttons & HotkeyTemporaryFastForward) == HotkeyTemporaryFastForward;
	if (WasFastForwarding != temporary_fast_forward)
		// update the frameskip value only if we were fast-forwarding
		// but now are not, or if we were not and now are
		game_set_frameskip();

	// Go to the menu if the globally configured key is pressed
	// (on the DS, that's the Touch Screen) or the hotkey.
	uint32_t HotkeyReturnToMenu = game_persistent_config.HotkeyReturnToMenu != 0 ? game_persistent_config.HotkeyReturnToMenu : gpsp_persistent_config.HotkeyReturnToMenu;

	if (non_repeat_buttons & game_config.gamepad_config_map[12] /* MENU */
	 || (HotkeyReturnToMenu && (buttons & HotkeyReturnToMenu) == HotkeyReturnToMenu))
	{
		Result |= REGBA_BUTTON_MENU;
	}

	// Request rewinding in gpsp_main.c if the hotkey is pressed.
	uint32_t HotkeyRewind = game_persistent_config.HotkeyRewind != 0 ? game_persistent_config.HotkeyRewind : gpsp_persistent_config.HotkeyRewind;

	if (HotkeyRewind && (buttons & HotkeyRewind) == HotkeyRewind) // Rewind requested
    {
		fast_backward = 1;
		return 0;
    }

	// Process saved state requests.
	uint32_t HotkeyQuickLoadState = game_persistent_config.HotkeyQuickLoadState != 0 ? game_persistent_config.HotkeyQuickLoadState : gpsp_persistent_config.HotkeyQuickLoadState;

	bool LoadStateIsHeld = HotkeyQuickLoadState && (buttons & HotkeyQuickLoadState) == HotkeyQuickLoadState;
	if (!LoadStateWasHeld && LoadStateIsHeld)
	{
		QuickLoadState();
	}
	LoadStateWasHeld = LoadStateIsHeld;

	uint32_t HotkeyQuickSaveState = game_persistent_config.HotkeyQuickSaveState != 0 ? game_persistent_config.HotkeyQuickSaveState : gpsp_persistent_config.HotkeyQuickSaveState;

	bool SaveStateIsHeld = HotkeyQuickSaveState && (buttons & HotkeyQuickSaveState) == HotkeyQuickSaveState;
	if (!SaveStateWasHeld && SaveStateIsHeld)
	{
		QuickSaveState();
	}
	SaveStateWasHeld = SaveStateIsHeld;

	// Now update GBA keys.
	for (i = 0; i < 12; i++)
	{
		if (buttons & game_config.gamepad_config_map[i]) // DS side
			Result |= 1 << i; // GBA side
	}

    return Result;
}

// GUI输入处理

button_repeat_state_type button_repeat_state = BUTTON_NOT_HELD;
clock_t button_repeat_timestamp;
uint16_t gui_button_repeat = 0;

gui_action_type key_to_cursor(uint16_t key)
{
	switch (key)
	{
		case DS_BUTTON_UP:	return CURSOR_UP;
		case DS_BUTTON_DOWN:	return CURSOR_DOWN;
		case DS_BUTTON_LEFT:	return CURSOR_LEFT;
		case DS_BUTTON_RIGHT:	return CURSOR_RIGHT;
		case DS_BUTTON_L:	return CURSOR_LTRIGGER;
		case DS_BUTTON_R:	return CURSOR_RTRIGGER;
		case DS_BUTTON_A:	return CURSOR_SELECT;
		case DS_BUTTON_B:	return CURSOR_BACK;
		case DS_BUTTON_X:	return CURSOR_EXIT;
		case DS_BUTTON_TOUCH:	return CURSOR_TOUCH;
		default:	return CURSOR_NONE;
	}
}

static uint16_t gui_keys[] = {
	DS_BUTTON_A, DS_BUTTON_B, DS_BUTTON_X, DS_BUTTON_L, DS_BUTTON_R, DS_BUTTON_TOUCH, DS_BUTTON_UP, DS_BUTTON_DOWN, DS_BUTTON_LEFT, DS_BUTTON_RIGHT
};

gui_action_type get_gui_input(void)
{
	size_t i;
	struct DS_InputState inputdata;
	DS2_GetInputState(&inputdata);

	if (inputdata.buttons & DS_BUTTON_LID) {
		DS2_SystemSleep();
	}

	if ((inputdata.buttons & (DS_BUTTON_L | DS_BUTTON_R)) == (DS_BUTTON_L | DS_BUTTON_R)) {
		save_menu_ss_bmp(DS2_GetSubScreen());
		DS2_AwaitNotAllButtonsIn(DS_BUTTON_L | DS_BUTTON_R);
	}

	while (1) {
		switch (button_repeat_state) {
		case BUTTON_NOT_HELD:
			// Pick the first pressed button out of the gui_keys array.
			for (i = 0; i < sizeof(gui_keys) / sizeof(gui_keys[0]); i++) {
				if (inputdata.buttons & gui_keys[i]) {
					button_repeat_state = BUTTON_HELD_INITIAL;
					button_repeat_timestamp = clock();
					gui_button_repeat = gui_keys[i];
					return key_to_cursor(gui_keys[i]);
				}
			}
			return CURSOR_NONE;
		case BUTTON_HELD_INITIAL:
		case BUTTON_HELD_REPEAT:
			// If the key that was being held isn't anymore...
			if (!(inputdata.buttons & gui_button_repeat)) {
				button_repeat_state = BUTTON_NOT_HELD;
				// Go see if another key is held (try #2)
				break;
			}
			else
			{
				bool IsRepeatReady = clock() - button_repeat_timestamp >= (button_repeat_state == BUTTON_HELD_INITIAL ? BUTTON_REPEAT_START : BUTTON_REPEAT_CONTINUE);
				if (!IsRepeatReady) {
					// Temporarily turn off the key.
					// It's not its turn to be repeated.
					inputdata.buttons &= ~gui_button_repeat;
				}
				
				// Pick the first pressed button out of the gui_keys array.
				for (i = 0; i < sizeof(gui_keys) / sizeof(gui_keys[0]); i++) {
					if (inputdata.buttons & gui_keys[i]) {
						// If it's the held key,
						// it's now repeating quickly.
						button_repeat_state = gui_keys[i] == gui_button_repeat ? BUTTON_HELD_REPEAT : BUTTON_HELD_INITIAL;
						button_repeat_timestamp = clock();
						gui_button_repeat = gui_keys[i];
						return key_to_cursor(gui_keys[i]);
					}
				}
				// If it was time for the repeat but it
				// didn't occur, stop repeating.
				if (IsRepeatReady) button_repeat_state = BUTTON_NOT_HELD;
				return CURSOR_NONE;
			}
		}
	}
}

struct selector_entry {
	char* name;
	bool  is_dir;
};

/*--------------------------------------------------------
	Sort function
--------------------------------------------------------*/
static int name_sort(const void* a, const void* b)
{
	return strcasecmp(((const struct selector_entry*) a)->name,
	                  ((const struct selector_entry*) b)->name);
}

/*
 * Shows a file selector interface.
 *
 * Input:
 *   exts: An array of const char* specifying the file extension filter.
 *     If the array is NULL, or the first entry is NULL, all files are shown
 *     regardless of extension.
 *     Otherwise, only files whose extensions match any of the entries, which
 *     must start with '.', are shown.
 * Input/output:
 *   dir: On entry to the function, the initial directory to be used.
 *     On exit, if a file was selected, the directory containing the selected
 *     file; otherwise, unchanged.
 * Output:
 *   result_name: If a file was selected, this is updated with the name of the
 *     selected file without its path; otherwise, unchanged.
 * Returns:
 *   0: a file was selected.
 *   -1: the user exited the selector without selecting a file.
 *   < -1: an error occurred.
 */
int32_t load_file(const char **exts, char *result_name, char *dir)
{
	if (dir == NULL || *dir == '\0')
		return -4;

	char cur_dir[PATH_MAX];
	bool continue_dir = true;
	int32_t ret;
	size_t i;
	void* scrollers[FILE_LIST_ROWS + 1 /* for the directory name */];

	strcpy(cur_dir, dir);

	for (i = 0; i < FILE_LIST_ROWS + 1; i++) {
		scrollers[i] = NULL;
	}

	while (continue_dir) {
		HighFrequencyCPU();
		// Read the current directory. This loop is continued every time the
		// current directory changes.

		show_icon(DS2_GetSubScreen(), &ICON_SUBBG, 0, 0);
		show_icon(DS2_GetSubScreen(), &ICON_TITLE, 0, 0);
		show_icon(DS2_GetSubScreen(), &ICON_TITLEICON, TITLE_ICON_X, TITLE_ICON_Y);
		PRINT_STRING_BG(DS2_GetSubScreen(), msg[MSG_FILE_MENU_LOADING_LIST], COLOR_WHITE, COLOR_TRANS, 49, 10);
		DS2_UpdateScreen(DS_ENGINE_SUB);

		clock_t last_display = clock();

		struct selector_entry* entries;
		DIR* cur_dir_handle = NULL;
		size_t count = 1, capacity = 4 /* initially */;

		entries = malloc(capacity * sizeof(struct selector_entry));
		if (entries == NULL) {
			ret = -2;
			continue_dir = false;
			goto cleanup;
		}

		entries[0].name = strdup("..");
		if (entries[0].name == NULL) {
			ret = -1;
			continue_dir = 0;
			goto cleanup;
		}
		entries[0].is_dir = true;

		cur_dir_handle = opendir(cur_dir);
		if (cur_dir_handle == NULL) {
			ret = -1;
			continue_dir = 0;
			goto cleanup;
		}

		struct dirent* cur_entry_handle;
		struct stat    st;

#ifdef SCDS2
		while ((cur_entry_handle = readdir_stat(cur_dir_handle, &st)) != NULL)
#else
		while ((cur_entry_handle = readdir(cur_dir_handle)) != NULL)
#endif
		{
			clock_t now = clock();
			bool add = false;
			char* name = cur_entry_handle->d_name;
#ifndef SCDS2
			char path[PATH_MAX];

			snprintf(path, PATH_MAX, "%s/%s", cur_dir, name);
			stat(path, &st);
#endif

			if (now >= last_display + CLOCKS_PER_SEC / 20) {
				last_display = now;

				DS2_AwaitScreenUpdate(DS_ENGINE_SUB);
				show_icon(DS2_GetSubScreen(), &ICON_TITLE, 0, 0);
				show_icon(DS2_GetSubScreen(), &ICON_TITLEICON, TITLE_ICON_X, TITLE_ICON_Y);
				char line[384];
				sprintf(line, "%s (%" PRIu32 ")", msg[MSG_FILE_MENU_LOADING_LIST], count);
				PRINT_STRING_BG(DS2_GetSubScreen(), line, COLOR_WHITE, COLOR_TRANS, 49, 10);
				DS2_UpdateScreenPart(DS_ENGINE_SUB, 10, 10 + BDF_GetFontHeight());
			}

			if (S_ISDIR(st.st_mode)) {
				// Add directories no matter what, except for the special
				// ones, "." and "..".
				if (!(name[0] == '.' &&
				    (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))) {
					add = true;
				}
			} else {
				if (exts == NULL || exts[0] == NULL) // Show every file
					add = true;
				else {
					// Add files only if their extension is in the list.
					char* ext = strrchr(name, '.');
					if (ext != NULL) {
						for (i = 0; exts[i] != NULL; i++) {
							if (strcasecmp(ext, exts[i]) == 0) {
								add = true;
								break;
							}
						}
					}
				}
			}

			if (add) {
				// Ensure we have enough capacity in the selector_entry array.
				if (count == capacity) {
					struct selector_entry* new_entries = realloc(entries, capacity * 2 * sizeof(struct selector_entry));
					if (new_entries == NULL) {
						ret = -2;
						continue_dir = false;
						goto cleanup;
					} else {
						entries = new_entries;
						capacity *= 2;
					}
				}

				// Then add the entry.
				entries[count].name = strdup(name);
				if (entries[count].name == NULL) {
					ret = -2;
					continue_dir = 0;
					goto cleanup;
				}

				entries[count].is_dir = S_ISDIR(st.st_mode) ? true : false;

				count++;
			}
		}

		DS2_AwaitScreenUpdate(DS_ENGINE_SUB);
		show_icon(DS2_GetSubScreen(), &ICON_TITLE, 0, 0);
		show_icon(DS2_GetSubScreen(), &ICON_TITLEICON, TITLE_ICON_X, TITLE_ICON_Y);
		PRINT_STRING_BG(DS2_GetSubScreen(), msg[MSG_FILE_MENU_SORTING_LIST], COLOR_WHITE, COLOR_TRANS, 49, 10);
		DS2_UpdateScreenPart(DS_ENGINE_SUB, 10, 10 + BDF_GetFontHeight());

		/* skip the first entry when sorting, which is always ".." */
		qsort(&entries[1], count - 1, sizeof(struct selector_entry), name_sort);
		DS2_AwaitScreenUpdate(DS_ENGINE_SUB);
		LowFrequencyCPU();

		bool continue_input = true;
		size_t sel_entry = 0, prev_sel_entry = 1 /* different to show scrollers to start */;
		uint32_t dir_scroll = 0x8000; // First scroll to the left
		int32_t entry_scroll = 0;

		scrollers[0] = draw_hscroll_init(DS2_GetSubScreen(), 49, 10, 199, COLOR_TRANS,
			COLOR_WHITE, cur_dir);

		if (scrollers[0] == NULL) {
			ret = -2;
			continue_dir = false;
			goto cleanupScrollers;
		}

		// Show the current directory and get input. This loop is continued
		// every frame, because the current directory scrolls atop the screen.

		while (continue_dir && continue_input) {
			// Try to use a row set such that the selected entry is in the
			// middle of the screen.
			size_t last_entry = sel_entry + FILE_LIST_ROWS / 2, first_entry;

			// If the last row is out of bounds, put it back in bounds.
			// (In this case, the user has selected an entry in the last
			// FILE_LIST_ROWS / 2.)
			if (last_entry >= count)
				last_entry = count - 1;

			if (last_entry < FILE_LIST_ROWS - 1) {
				/* Move to the first entry unconditionally. */
				first_entry = 0;
				// If there are more than FILE_LIST_ROWS / 2 files,
				// we need to enlarge the first page.
				last_entry = FILE_LIST_ROWS - 1;
				if (last_entry >= count) // No...
					last_entry = count - 1;
			} else
				first_entry = last_entry - (FILE_LIST_ROWS - 1);

			// Update scrollers.
			// a) If a different item has been selected, remake entry
			//    scrollers, resetting the formerly selected entry to the
			//    start and updating the selection color.
			if (sel_entry != prev_sel_entry) {
				// Preserve the directory scroller.
				for (i = 1; i < FILE_LIST_ROWS + 1; i++) {
					draw_hscroll_over(scrollers[i]);
					scrollers[i] = NULL;
				}
				for (i = first_entry; i <= last_entry; i++) {
					uint16_t color = (i == sel_entry)
						? COLOR_ACTIVE_ITEM
						: COLOR_INACTIVE_ITEM;
					scrollers[i - first_entry + 1] = hscroll_init(DS2_GetSubScreen(),
						FILE_SELECTOR_NAME_X,
						GUI_ROW1_Y + (i - first_entry) * GUI_ROW_SY + TEXT_OFFSET_Y,
						FILE_SELECTOR_NAME_SX,
						COLOR_TRANS,
						color,
						entries[i].name);
					if (scrollers[i - first_entry + 1] == NULL) {
						ret = -2;
						continue_dir = 0;
						goto cleanupScrollers;
					}
				}

				prev_sel_entry = sel_entry;
			}

			// b) Must we update the directory scroller?
			if ((dir_scroll & 0xFF) >= 0x20) {
				if (dir_scroll & 0x8000) {  /* scrolling to the left */
					if (draw_hscroll(scrollers[0], -1) == 0) dir_scroll = 0;
				} else {  /* scrolling to the right */
					if (draw_hscroll(scrollers[0], 1) == 0) dir_scroll = 0x8000;
				}
			} else {
				// Wait one less frame before scrolling the directory again.
				dir_scroll++;
			}

			// c) Must we scroll the current file as a result of user input?
			if (entry_scroll != 0) {
				draw_hscroll(scrollers[sel_entry - first_entry + 1], entry_scroll);
				entry_scroll = 0;
			}

			// Draw.
			// a) The background.
			show_icon(DS2_GetSubScreen(), &ICON_SUBBG, 0, 0);
			show_icon(DS2_GetSubScreen(), &ICON_TITLE, 0, 0);
			show_icon(DS2_GetSubScreen(), &ICON_TITLEICON, TITLE_ICON_X, TITLE_ICON_Y);

			// b) The selection background.
			show_icon(DS2_GetSubScreen(), &ICON_SUBSELA, SUBSELA_X, GUI_ROW1_Y + (sel_entry - first_entry) * GUI_ROW_SY + SUBSELA_OFFSET_Y);

			// c) The scrollers.
			for (i = 0; i < FILE_LIST_ROWS + 1; i++)
				draw_hscroll(scrollers[i], 0);

			// d) The icons.
			for (i = first_entry; i <= last_entry; i++)
			{
				struct gui_icon* icon;
				if (i == 0)
					icon = &ICON_DOTDIR;
				else if (entries[i].is_dir)
					icon = &ICON_DIRECTORY;
				else {
					char* ext = strrchr(entries[i].name, '.');
					if (ext != NULL) {
						if (strcasecmp(ext, ".gba") == 0)
							icon = &ICON_GBAFILE;
						else if (strcasecmp(ext, ".zip") == 0)
							icon = &ICON_ZIPFILE;
						else if (strcasecmp(ext, ".cht") == 0)
							icon = &ICON_CHTFILE;
						else
							icon = &ICON_UNKNOW;
					} else
						icon = &ICON_UNKNOW;
				}

				show_icon(DS2_GetSubScreen(), icon, FILE_SELECTOR_ICON_X, GUI_ROW1_Y + (i - first_entry) * GUI_ROW_SY + FILE_SELECTOR_ICON_Y);
			}

			DS2_UpdateScreen(DS_ENGINE_SUB);
			DS2_AwaitScreenUpdate(DS_ENGINE_SUB);

			struct DS_InputState inputdata;
			gui_action_type gui_action = get_gui_input();
			DS2_GetInputState(&inputdata);

			// Get DS_BUTTON_RIGHT and DS_BUTTON_LEFT separately to allow scrolling
			// the selected file name faster.
			if (inputdata.buttons & DS_BUTTON_RIGHT)
				entry_scroll = -3;
			else if (inputdata.buttons & DS_BUTTON_LEFT)
				entry_scroll = 3;

			switch (gui_action) {
				case CURSOR_TOUCH:
				{
					DS2_AwaitNoButtons();
					// ___ 33        This screen has 6 possible rows. Touches
					// ___ 60        above or below these are ignored.
					// . . . (+27)
					// ___ 192
					if (inputdata.touch_y <= GUI_ROW1_Y || inputdata.touch_y > DS_SCREEN_HEIGHT)
						break;

					size_t row = (inputdata.touch_y - GUI_ROW1_Y) / GUI_ROW_SY;

					if (row >= last_entry - first_entry + 1)
						break;

					sel_entry = first_entry + row;
					/* fall through */
				}

				case CURSOR_SELECT:
					DS2_AwaitNoButtons();
					if (sel_entry == 0) {  /* the parent directory */
						char* slash = strrchr(cur_dir, '/');
						if (slash != NULL) {  /* there's a parent */
							*slash = '\0';
							continue_input = false;
						} else {  /* we're at the root */
							ret = -1;
							continue_dir = false;
						}
					} else if (entries[sel_entry].is_dir) {
						strcat(cur_dir, "/");
						strcat(cur_dir, entries[sel_entry].name);
						continue_input = false;
					} else {
						strcpy(dir, cur_dir);
						strcpy(result_name, entries[sel_entry].name);
						ret = 0;
						continue_dir = false;
					}
					break;

				case CURSOR_UP:
					if (sel_entry > 0)
						sel_entry--;
					break;

				case CURSOR_DOWN:
					sel_entry++;
					if (sel_entry >= count)
						sel_entry--;
					break;

				//scroll page down
				case CURSOR_RTRIGGER:
					sel_entry += FILE_LIST_ROWS;
					if (sel_entry >= count)
						sel_entry = count - 1;
					break;

				//scroll page up
				case CURSOR_LTRIGGER:
					if (sel_entry >= FILE_LIST_ROWS)
						sel_entry -= FILE_LIST_ROWS;
					else
						sel_entry = 0;
					break;

				case CURSOR_BACK:
				{
					DS2_AwaitNoButtons();
					char* slash = strrchr(cur_dir, '/');
					if (slash != NULL) {  /* there's a parent */
						*slash = '\0';
						continue_input = false;
					} else {  /* we're at the root */
						ret = -1;
						continue_dir = false;
					}
					break;
				}

				case CURSOR_EXIT:
					DS2_AwaitNoButtons();
					ret = -1;
					continue_dir = false;
					break;

				default:
					break;
			} // end switch
		} // end while

cleanupScrollers:
		for (i = 0; i < FILE_LIST_ROWS + 1; i++) {
			draw_hscroll_over(scrollers[i]);
			scrollers[i] = NULL;
		}

cleanup:
		if (cur_dir_handle != NULL)
			closedir(cur_dir_handle);

		if (entries != NULL) {
			for (; count > 0; count--)
				free(entries[count - 1].name);
			free(entries);
		}
	} // end while

	return ret;
}

/*--------------------------------------------------------
  放映幻灯片
--------------------------------------------------------*/

void play_screen_snapshot(void)
{
    uint32_t repeat, i;
    uint16_t *screenp;

	char** EntryNames = NULL;
	DIR*   CurrentDirHandle = NULL;
	uint32_t    EntryCount = 0, EntryCapacity = 4 /* initial */;
	uint32_t    Failed = 0;

	EntryNames = (char**) malloc(EntryCapacity * sizeof(char*));
	if (EntryNames == NULL)
	{
		Failed = 1;
		goto cleanup;
	}

	CurrentDirHandle = opendir(DEFAULT_SS_DIR);
	if (CurrentDirHandle == NULL) {
		Failed = 1;
		goto cleanup;
	}

	struct dirent* CurrentEntryHandle;
	struct stat    Stat;

	while ((CurrentEntryHandle = readdir(CurrentDirHandle)) != NULL)
	{
		char* Name = CurrentEntryHandle->d_name;
		char FullPath[PATH_MAX];

		snprintf(FullPath, PATH_MAX, "%s/%s", DEFAULT_SS_DIR, Name);
		stat(FullPath, &Stat);

		if (!S_ISDIR(Stat.st_mode))
		{
			// Add files only if their extension is in the list.
			char* Extension = strrchr(Name, '.');
			if (Extension != NULL)
			{
				if (strcasecmp(Extension, ".bmp") == 0)
				{
					// Ensure we have enough capacity in the char* array first.
					if (EntryCount == EntryCapacity)
					{
						void* NewEntryNames = realloc(EntryNames, EntryCapacity * 2 * sizeof(char*));
						if (NewEntryNames == NULL)
						{
							Failed = 1;
							goto cleanup;
						}
						else
							EntryNames = NewEntryNames;

						EntryCapacity *= 2;
					}

					// Then add the entry.
					EntryNames[EntryCount] = malloc(strlen(Name) + 1);
					if (EntryNames[EntryCount] == NULL)
					{
						Failed = 1;
						goto cleanup;
					}

					strcpy(EntryNames[EntryCount], Name);

					EntryCount++;
				}
			}
		}
	}

cleanup:
	if (CurrentDirHandle != NULL)
		closedir(CurrentDirHandle);

	screenp = malloc(DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT * sizeof(uint16_t));
	if (screenp == NULL) {
		screenp = DS2_GetSubScreen();
	} else {
		memcpy(screenp, DS2_GetSubScreen(), DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT * sizeof(uint16_t));
	}

    if (Failed || EntryCount == 0)
    {
        draw_message_box(DS2_GetSubScreen());
        draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_NO_SLIDE]);
		DS2_UpdateScreen(DS_ENGINE_SUB);

        if (screenp) free((void*)screenp);

		if (EntryNames != NULL)
		{
			for (; EntryCount > 0; EntryCount--)
				free(EntryNames[EntryCount - 1]);
			free(EntryNames);
		}

		DS2_AwaitNoButtons();
		DS2_AwaitAnyButtons();
		DS2_AwaitNoButtons();

		return;
    }

    char bmp_path[PATH_MAX];

    uint32_t time0= 10;
    uint32_t pause= 0;

    draw_message_box(DS2_GetSubScreen());
    draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_SCREENSHOT_SLIDESHOW_KEYS]);
	DS2_UpdateScreen(DS_ENGINE_SUB);

    repeat= 1;
    i= 0;
    while (repeat)
    {
        sprintf(bmp_path, "%s/%s", DEFAULT_SS_DIR, EntryNames[i]);

		BMP_Read(bmp_path, DS2_GetMainScreen(), DS_SCREEN_WIDTH, DS_SCREEN_HEIGHT);
		DS2_FlipMainScreen();

        if (i+1 < EntryCount) i++;
        else i= 0;

        gui_action_type gui_action;
        uint32_t ticks= 0;
        uint32_t time1;

        time1= time0;
        while (ticks < time1)
        {
            gui_action = get_gui_input();

            switch(gui_action)
            {
                case CURSOR_UP:
                    if (!pause)
                    {
                        if (time0 > 1) time0 -= 1;
                        time1= time0;
                    }
                    break;

                case CURSOR_DOWN:
                    if (!pause)
                    {
                        time0 += 1;
                        time1= time0;
                    }
                    break;

                case CURSOR_LEFT:
                    time1 = ticks;
                    if (i > 1) i -= 2;
                    else if (i == 1) i= EntryCount -1;
                    else i= EntryCount -2;
                    break;

                case CURSOR_RIGHT:
                    time1 = ticks;
                    break;

                case CURSOR_SELECT:
                    if (!pause)
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
					repeat = 0;
					break;

                default: gui_action= CURSOR_NONE;
                    break;
            }

			if (gui_action != CURSOR_BACK)
				usleep(100000);
            if (!pause)
                ticks ++;
        }
    }

	if (screenp) free(screenp);

	if (EntryNames != NULL)
	{
		for (; EntryCount > 0; EntryCount--)
			free(EntryNames[EntryCount - 1]);
		free(EntryNames);
	}
}

/*--------------------------------------------------------
  搜索指定名称的目录
--------------------------------------------------------*/

//标准按键
const uint32_t gamepad_config_map_init[MAX_GAMEPAD_CONFIG_MAP] =
{
	/* DS               -> GBA    */
	DS_BUTTON_A,        /* 0    A */
	DS_BUTTON_B,        /* 1    B */
	DS_BUTTON_SELECT,   /* 2    [SELECT] */
	DS_BUTTON_START,    /* 3    [START] */
	DS_BUTTON_RIGHT,    /* 4    → */
	DS_BUTTON_LEFT,     /* 5    ← */
	DS_BUTTON_UP,       /* 6    ↑ */
	DS_BUTTON_DOWN,     /* 7    ↓ */
	DS_BUTTON_R,        /* 8    [R] */
	DS_BUTTON_L,        /* 9    [L] */
	0,                  /* 10   FA */
	0,                  /* 11   FB */
	DS_BUTTON_TOUCH,    /* 12   MENU */
	0,                  /* 13   NONE */
	0,                  /* 14   NONE */
	0                   /* 15   NONE */
};

/*
 * After loading a new game or resetting its configuration through the
 * Options menu, calling this function is needed. It applies settings that
 * aren't automatically tracked by gpSP variables.
 * This is called by init_game_config and load_game_config, below. That's all
 * that should be needed.
 */
void FixUpSettings()
{
	game_set_frameskip();
	game_set_rewind();
	set_button_map();
}

/*--------------------------------------------------------
  game cfg的初始化
--------------------------------------------------------*/
void init_game_config()
{
	memset(&game_config, 0, sizeof(game_config));
	memset(&game_persistent_config, 0, sizeof(game_persistent_config));

	uint32_t i;
	game_fast_forward = 0;
	game_persistent_config.frameskip_value = 0; // default: keep up/automatic
	game_persistent_config.rewind_value = 6; // default: 10 seconds
	game_config.audio_buffer_size_number = 15;
	for (i = 0; i < MAX_CHEATS; i++) {
		game_config.cheats_flag[i].cheat_active = NO;
		game_config.cheats_flag[i].cheat_name[0] = '\0';
	}

	FixUpSettings();
}

/*--------------------------------------------------------
  gpSP cfg的初始化
--------------------------------------------------------*/
void init_default_gpsp_config()
{
	memset(&gpsp_config, 0, sizeof(gpsp_config));
	memset(&gpsp_persistent_config, 0, sizeof(gpsp_persistent_config));

	game_config.frameskip_type = 1;   //auto
	game_config.frameskip_value = 2;
	game_config.backward = 0;       //time backward disable
	game_config.backward_time = 5;  //time backward granularity 10s
	gpsp_config.screen_ratio = 1;  //orignal
	gpsp_config.enable_audio = 1;  //on
	gpsp_persistent_config.language = 0;     //default language= English
	// By default, allow L+Y (DS side) to rewind in all games.
	gpsp_persistent_config.HotkeyRewind = DS_BUTTON_L | DS_BUTTON_Y;
	// Default button mapping.
	gpsp_persistent_config.ButtonMappings[0] /* GBA A */ = DS_BUTTON_A;
	gpsp_persistent_config.ButtonMappings[1] /* GBA B */ = DS_BUTTON_B;
	gpsp_persistent_config.ButtonMappings[2] /* GBA SELECT */ = DS_BUTTON_SELECT;
	gpsp_persistent_config.ButtonMappings[3] /* GBA START */ = DS_BUTTON_START;
	gpsp_persistent_config.ButtonMappings[4] /* GBA R */ = DS_BUTTON_R;
	gpsp_persistent_config.ButtonMappings[5] /* GBA L */ = DS_BUTTON_L;
#if USE_C_CORE
	gpsp_config.emulate_core = C_CORE;
#else
	gpsp_config.emulate_core = ASM_CORE;
#endif
	gpsp_config.debug_flag = NO;
	gpsp_config.fake_fat = NO;
	gpsp_config.rom_file[0] = 0;
	gpsp_config.rom_path[0] = 0;
	memset(gpsp_persistent_config.latest_file, 0, sizeof(gpsp_persistent_config.latest_file));
	gpsp_persistent_config.cpu_hz = UINT32_C(360000000);
	gpsp_persistent_config.mem_hz = UINT32_C(120000000);
}

/*--------------------------------------------------------
  game cfgファイルの読込み
  3/4修正
--------------------------------------------------------*/
void load_game_config_file(void)
{
	char game_config_filename[PATH_MAX];
	FILE_TAG_TYPE game_config_file;
	char FileNameNoExt[PATH_MAX];

	// 设置初始值
	init_game_config();

	GetFileNameNoExtension(FileNameNoExt, CurrentGamePath);

	sprintf(game_config_filename, "%s/%s_0.rts", DEFAULT_CFG_DIR, FileNameNoExt);

	FILE_OPEN(game_config_file, game_config_filename, READ);
	if (FILE_CHECK_VALID(game_config_file)) {
		//Check file header
		char* pt= game_config_filename;
		FILE_READ(game_config_file, pt, GAME_CONFIG_HEADER_SIZE);

		if (strncmp(pt, GAME_CONFIG_HEADER, GAME_CONFIG_HEADER_SIZE) == 0) {
			FILE_READ_VARIABLE(game_config_file, game_persistent_config);
		}
		FILE_CLOSE(game_config_file);
	}
	FixUpSettings();
}

/*--------------------------------------------------------
  gpSP cfg配置文件
--------------------------------------------------------*/
int32_t load_config_file()
{
	char gpsp_config_path[PATH_MAX];
	FILE_TAG_TYPE gpsp_config_file;
	char *pt;

	init_default_gpsp_config();

	sprintf(gpsp_config_path, "%s/%s", main_path, GPSP_CONFIG_FILENAME);
	FILE_OPEN(gpsp_config_file, gpsp_config_path, READ);

	if (FILE_CHECK_VALID(gpsp_config_file))
	{
		// check the file header
		pt = gpsp_config_path;
		FILE_READ(gpsp_config_file, pt, GPSP_CONFIG_HEADER_SIZE);
		pt[GPSP_CONFIG_HEADER_SIZE] = '\0';
		if (strcmp(pt, GPSP_CONFIG_HEADER) == 0) {
			FILE_READ_VARIABLE(gpsp_config_file, gpsp_persistent_config);
			FILE_CLOSE(gpsp_config_file);

			if (gpsp_persistent_config.cpu_hz == 0)
				gpsp_persistent_config.cpu_hz = UINT32_C(360000000);
			if (gpsp_persistent_config.mem_hz == 0)
				gpsp_persistent_config.mem_hz = UINT32_C(120000000);

			return 0;
		} else
			FILE_CLOSE(gpsp_config_file);
	}

	return -1;
}

void initial_gpsp_config()
{
	sprintf(g_default_rom_dir, "%s/GAMES", main_path);
	sprintf(DEFAULT_SAVE_DIR, "%s/SAVES", main_path);
	sprintf(DEFAULT_CFG_DIR, "%s/SAVES", main_path);
	sprintf(DEFAULT_SS_DIR, "%s/PICS", main_path);
	sprintf(DEFAULT_CHEAT_DIR, "%s/CHEATS", main_path);
}

bool LoadGameAndItsData(const char* filename)
{
	draw_message_box(DS2_GetSubScreen());
	draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_LOADING_GAME]);
	DS2_UpdateScreen(DS_ENGINE_SUB);

	HighFrequencyCPU();
	int load_result = load_gamepak(filename);
	LowFrequencyCPU();

	if (load_result == -1) {
		return false;
	}

	load_game_config_file();

	reorder_latest_file(filename);
	get_savestate_filelist(filename);

	game_fast_forward = 0;

	return true;
}

/* --- THE MENU --- */

enum EntryKind {
	KIND_ACTION,
	KIND_OPTION,
	KIND_SUBMENU,
	KIND_DISPLAY,
	KIND_CUSTOM,
};

enum DataType {
	TYPE_STRING,
	TYPE_INT32,
	TYPE_UINT32,
	TYPE_INT64,
	TYPE_UINT64,
};

struct TouchBounds {
	uint32_t X1; /* The left bound. */
	uint32_t Y1; /* The top bound. */
	uint32_t X2; /* The right bound, exclusive. */
	uint32_t Y2; /* The bottom bound, exclusive. */
};

struct Entry;

struct Menu;

/*
 * ModifyFunction is the type of a function that acts on an input in the
 * menu. The function is assigned this input via the Entry struct's
 * various button functions, Init, End, etc.
 * Input:
 *   1: On entry into the function, points to a memory location containing
 *     a pointer to the active menu. On exit from the function, the menu may
 *     be modified to a new one, in which case the function has chosen to
 *     activate that new menu; the End of the old menu is called,
 *     then the Init function of the new menu is called.
 *
 *     The exception to this rule is the NULL menu. If NULL is chosen to be
 *     activated, then no Init function is called; additionally, the menu is
 *     exited.
 *   2: On entry into the function, points to a memory location containing the
 *     index among the active menu's Entries array corresponding to the active
 *     menu entry. On exit from the function, the menu entry index may be
 *     modified to a new one, in which case the function has chosen to
 *     activate that new menu entry.
 */
typedef void (*ModifyFunction) (struct Menu**, uint32_t*);

/*
 * EntryDisplayFunction is the type of a function that displays an element
 * (the name or the value, depending on which member receives a function of
 * this type) of a single menu entry.
 * Input:
 *   1: A pointer to the data for the menu entry whose part is being drawn.
 *   2: A pointer to the data for the active menu entry.
 *   3: The position, expressed as a line number starting at 0, of the entry
 *     part to be drawn.
 */
typedef void (*EntryDisplayFunction) (struct Entry*, struct Entry*, uint32_t);

/*
 * EntryCanFocusFunction is the type of a function that determines whether
 * a menu entry can receive the focus.
 * Input:
 *   1: The menu containing the entry that is being tested.
 *   2: The menu entry that is being tested.
 *   3: The index of the entry within its containing menu.
 */
typedef bool (*EntryCanFocusFunction) (struct Menu*, struct Entry*, uint32_t);

/*
 * EntryFunction is the type of a function that displays all data related
 * to a menu or that modifies the Target variable of the active menu entry.
 * Input:
 *   1: The menu containing the entries that are being drawn, or the entry
 *     whose Target variable is being modified.
 *   2: The active menu entry.
 */
typedef void (*EntryFunction) (struct Menu*, struct Entry*);

/*
 * EntryTouchBoundsFunction is the type of a function that determines the
 * bounds of a menu entry.
 * Input:
 *   1: A pointer to the data for the menu containing the entry whose touch
 *     bounds are being queried.
 *   2: A pointer to the data for the menu entry whose touch bounds are being
 *     queried.
 *   3: The position, expressed as a line number starting at 0, of the entry.
 * Output:
 *   4: The bounds of the entry's touch rectangle.
 */
typedef void (*EntryTouchBoundsFunction) (struct Menu*, struct Entry*, uint32_t, struct TouchBounds*);

/*
 * EntryTouchFunction is the type of a function that acts on touches in the
 * menu.
 * Input:
 *   1: On entry into the function, points to a memory location containing
 *     a pointer to the active menu. On exit from the function, the menu may
 *     be modified to a new one, in which case the function has chosen to
 *     activate that new menu; the End of the old menu is called,
 *     then the Init function of the new menu is called.
 *
 *     The exception to this rule is the NULL menu. If NULL is chosen to be
 *     activated, then no Init function is called; additionally, the menu is
 *     exited.
 *   2: On entry into the function, points to a memory location containing the
 *     index among the active menu's Entries array that has been touched. It
 *     is updated by the input dispatch function. On exit from the function,
 *     the menu entry index may be modified to a new one, in which case the
 *     function has chosen to activate that new menu entry.
 *   3: The X coordinate of the touch.
 *   4: The Y coordinate of the touch.
 */
typedef void (*EntryTouchFunction) (struct Menu**, uint32_t*, uint32_t X, uint32_t Y);

/*
 * InitFunction is the type of a function that runs when a menu is being
 * initialised.
 * Variables:
 *   1: A pointer to a variable holding the menu that is being initialised.
 *   On exit from the function, the menu may be modified to a new one, in
 *   which case the function has chosen to activate that new menu. This can
 *   be used when the initialisation has failed for some reason.
 */
typedef void (*InitFunction) (struct Menu**);

/*
 * MenuFunction is the type of a function that runs when a menu is being
 * finalised or drawn.
 * Input:
 *   1: The menu that is being finalised or drawn.
 */
typedef void (*MenuFunction) (struct Menu*);

/*
 * ActionFunction is the type of a function that runs after changes to the
 * value of an option (if the entry kind is KIND_OPTION), or to respond to
 * pressing the action button (if the entry kind is KIND_ACTION).
 */
typedef void (*ActionFunction) (void);

struct Entry {
	enum EntryKind Kind;
	const char** Name;
	enum DataType DisplayType;
	void* Target;           // With KIND_OPTION, must point to uint32_t.
	                        // With KIND_DISPLAY, must point to the data type
	                        // specified by DisplayType.
	                        // With KIND_SUBMENU, this is struct Menu*.
	// Choices and ChoiceCount are only used with KIND_OPTION.
	ModifyFunction Enter;
	EntryFunction Left;
	EntryFunction Right;
	EntryTouchBoundsFunction TouchBounds;
	EntryTouchFunction Touch;
	EntryDisplayFunction DisplayName;
	EntryDisplayFunction DisplayValue;
	EntryCanFocusFunction CanFocus;
	ActionFunction Action;
	void* UserData;
	uint32_t ChoiceCount;
	const char** Choices[];
};

struct Menu {
	struct Menu* Parent;
	const char** Title;
	MenuFunction DisplayBackground;
	MenuFunction DisplayTitle;
	EntryFunction DisplayData;
	ModifyFunction InputDispatch;
	ModifyFunction Up;
	ModifyFunction Down;
	ModifyFunction Leave;
	InitFunction Init;
	MenuFunction End;
	void* UserData;
	uint32_t ActiveEntryIndex;
	struct Entry* Entries[]; // Entries are ended by a NULL pointer value.
};

static bool DefaultCanFocus(struct Menu* ActiveMenu, struct Entry* ActiveEntry, uint32_t Position)
{
	if (ActiveEntry->Kind == KIND_DISPLAY)
		return false;
	return true;
}

static uint32_t FindNullEntry(struct Menu* ActiveMenu)
{
	uint32_t Result = 0;
	while (ActiveMenu->Entries[Result] != NULL)
		Result++;
	return Result;
}

static bool MoveUp(struct Menu* ActiveMenu, uint32_t* ActiveEntryIndex)
{
	if (*ActiveEntryIndex == 0) {  // Going over the top?
		// Go back to the bottom.
		uint32_t NullEntry = FindNullEntry(ActiveMenu);
		if (NullEntry == 0)
			return false;
		*ActiveEntryIndex = NullEntry - 1;
		return true;
	}
	(*ActiveEntryIndex)--;
	return true;
}

static bool MoveDown(struct Menu* ActiveMenu, uint32_t* ActiveEntryIndex)
{
	if (*ActiveEntryIndex == 0 && ActiveMenu->Entries[*ActiveEntryIndex] == NULL)
		return false;
	if (ActiveMenu->Entries[*ActiveEntryIndex] == NULL)  // Is the sentinel "active"?
		*ActiveEntryIndex = 0;  // Go back to the top.
	(*ActiveEntryIndex)++;
	if (ActiveMenu->Entries[*ActiveEntryIndex] == NULL)  // Going below the bottom?
		*ActiveEntryIndex = 0;  // Go back to the top.
	return true;
}

static void DefaultUp(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	if (MoveUp(*ActiveMenu, ActiveEntryIndex)) {
		// Keep moving up until a menu entry can be focused.
		uint32_t Sentinel = *ActiveEntryIndex;
		EntryCanFocusFunction CanFocus = (*ActiveMenu)->Entries[*ActiveEntryIndex]->CanFocus;
		if (CanFocus == NULL) CanFocus = DefaultCanFocus;
		while (!CanFocus(*ActiveMenu, (*ActiveMenu)->Entries[*ActiveEntryIndex], *ActiveEntryIndex)) {
			MoveUp(*ActiveMenu, ActiveEntryIndex);
			if (*ActiveEntryIndex == Sentinel) {
				// If we went through all of them, we cannot focus anything.
				// Place the focus on the NULL entry.
				*ActiveEntryIndex = FindNullEntry(*ActiveMenu);
				return;
			}
			CanFocus = (*ActiveMenu)->Entries[*ActiveEntryIndex]->CanFocus;
			if (CanFocus == NULL) CanFocus = DefaultCanFocus;
		}
	}
}

static void DefaultDown(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	if (MoveDown(*ActiveMenu, ActiveEntryIndex)) {
		// Keep moving up until a menu entry can be focused.
		uint32_t Sentinel = *ActiveEntryIndex;
		EntryCanFocusFunction CanFocus = (*ActiveMenu)->Entries[*ActiveEntryIndex]->CanFocus;
		if (CanFocus == NULL) CanFocus = DefaultCanFocus;
		while (!CanFocus(*ActiveMenu, (*ActiveMenu)->Entries[*ActiveEntryIndex], *ActiveEntryIndex)) {
			MoveDown(*ActiveMenu, ActiveEntryIndex);
			if (*ActiveEntryIndex == Sentinel) {
				// If we went through all of them, we cannot focus anything.
				// Place the focus on the NULL entry.
				*ActiveEntryIndex = FindNullEntry(*ActiveMenu);
				return;
			}
			CanFocus = (*ActiveMenu)->Entries[*ActiveEntryIndex]->CanFocus;
			if (CanFocus == NULL) CanFocus = DefaultCanFocus;
		}
	}
}

static void DefaultRight(struct Menu* ActiveMenu, struct Entry* ActiveEntry)
{
	if (ActiveEntry->Kind == KIND_OPTION
	|| (ActiveEntry->Kind == KIND_CUSTOM && ActiveEntry->Right == &DefaultRight /* chose to use this function */
	 && ActiveEntry->Target != NULL)) {
		uint32_t* Target = (uint32_t*) ActiveEntry->Target;
		(*Target)++;
		if (*Target >= ActiveEntry->ChoiceCount)
			*Target = 0;

		ActionFunction Action = ActiveEntry->Action;
		if (Action != NULL)
			Action();
	}
}

static void DefaultLeft(struct Menu* ActiveMenu, struct Entry* ActiveEntry)
{
	if (ActiveEntry->Kind == KIND_OPTION
	|| (ActiveEntry->Kind == KIND_CUSTOM && ActiveEntry->Left == &DefaultLeft /* chose to use this function */
	 && ActiveEntry->Target != NULL)) {
		uint32_t* Target = (uint32_t*) ActiveEntry->Target;
		if (*Target == 0)
			*Target = ActiveEntry->ChoiceCount;
		(*Target)--;

		ActionFunction Action = ActiveEntry->Action;
		if (Action != NULL)
			Action();
	}
}

static void DefaultEnter(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	if ((*ActiveMenu)->Entries[*ActiveEntryIndex]->Kind == KIND_SUBMENU) {
		*ActiveMenu = (struct Menu*) (*ActiveMenu)->Entries[*ActiveEntryIndex]->Target;
	} else if ((*ActiveMenu)->Entries[*ActiveEntryIndex]->Kind == KIND_ACTION) {
		ActionFunction Action = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Action;
		if (Action != NULL) Action();
	}
}

static void DefaultLeave(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	*ActiveMenu = (*ActiveMenu)->Parent;
}

static void DefaultTouchBounds(struct Menu* ActiveMenu, struct Entry* ActiveEntry, uint32_t Position, struct TouchBounds* Bounds)
{
	*Bounds = (struct TouchBounds) {
		.X1 = 0, .Y1 = GUI_ROW1_Y + (Position - 1) * GUI_ROW_SY,
		.X2 = 256, .Y2 = GUI_ROW1_Y + Position * GUI_ROW_SY
	};
}

static void DefaultTouch(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex, uint32_t X, uint32_t Y)
{
	if ((*ActiveMenu)->Entries[*ActiveEntryIndex]->Kind == KIND_SUBMENU) {
		ModifyFunction Enter = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Enter;
		if (Enter == NULL) Enter = DefaultEnter;
		Enter(ActiveMenu, ActiveEntryIndex);
	} else if ((*ActiveMenu)->Entries[*ActiveEntryIndex]->Kind == KIND_ACTION) {
		ActionFunction Action = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Action;
		if (Action != NULL)
			Action();
	} else if ((*ActiveMenu)->Entries[*ActiveEntryIndex]->Kind == KIND_OPTION) {
		EntryFunction Right = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Right;
		if (Right == NULL) Right = DefaultRight;
		Right(*ActiveMenu, (*ActiveMenu)->Entries[*ActiveEntryIndex]);
	}
}

static void DefaultInputDispatch(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	gui_action_type gui_action = get_gui_input();
	
	switch (gui_action) {
	case CURSOR_SELECT:
		if ((*ActiveMenu)->Entries[*ActiveEntryIndex] != NULL) {
			ModifyFunction Enter = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Enter;
			if (Enter == NULL) Enter = DefaultEnter;
			Enter(ActiveMenu, ActiveEntryIndex);
			break;
		}
		// otherwise, no entry has the focus, so SELECT acts like BACK
		// (fall through)

	case CURSOR_BACK:
	{
		ModifyFunction Leave = (*ActiveMenu)->Leave;
		if (Leave == NULL) Leave = DefaultLeave;
		Leave(ActiveMenu, ActiveEntryIndex);
		break;
	}

	case CURSOR_UP:
	{
		ModifyFunction Up = (*ActiveMenu)->Up;
		if (Up == NULL) Up = DefaultUp;
		Up(ActiveMenu, ActiveEntryIndex);
		break;
	}

	case CURSOR_DOWN:
	{
		ModifyFunction Down = (*ActiveMenu)->Down;
		if (Down == NULL) Down = DefaultDown;
		Down(ActiveMenu, ActiveEntryIndex);
		break;
	}

	case CURSOR_LEFT:
		if ((*ActiveMenu)->Entries[*ActiveEntryIndex] != NULL) {
			EntryFunction Left = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Left;
			if (Left == NULL) Left = DefaultLeft;
			Left(*ActiveMenu, (*ActiveMenu)->Entries[*ActiveEntryIndex]);
		}
		break;

	case CURSOR_RIGHT:
		if ((*ActiveMenu)->Entries[*ActiveEntryIndex] != NULL) {
			EntryFunction Right = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Right;
			if (Right == NULL) Right = DefaultRight;
			Right(*ActiveMenu, (*ActiveMenu)->Entries[*ActiveEntryIndex]);
		}
		break;

	case CURSOR_TOUCH:
	{
		struct DS_InputState inputdata;
		uint32_t EntryIndex = 0;
		struct Entry* Entry = (*ActiveMenu)->Entries[0];
		struct TouchBounds Bounds;

		DS2_GetInputState(&inputdata);

		for (; Entry != NULL; EntryIndex++, Entry = (*ActiveMenu)->Entries[EntryIndex]) {
			EntryCanFocusFunction CanFocus = (*ActiveMenu)->Entries[EntryIndex]->CanFocus;
			if (CanFocus == NULL) CanFocus = DefaultCanFocus;
			if (!CanFocus(*ActiveMenu, Entry, EntryIndex))
				continue;

			EntryTouchBoundsFunction TouchBounds = (*ActiveMenu)->Entries[EntryIndex]->TouchBounds;
			if (TouchBounds == NULL) TouchBounds = DefaultTouchBounds;
			TouchBounds(*ActiveMenu, Entry, EntryIndex, &Bounds);

			if (inputdata.touch_x >= Bounds.X1 && inputdata.touch_x < Bounds.X2
			 && inputdata.touch_y >= Bounds.Y1 && inputdata.touch_y < Bounds.Y2) {
				*ActiveEntryIndex = EntryIndex;

				EntryTouchFunction Touch = (*ActiveMenu)->Entries[EntryIndex]->Touch;
				if (Touch == NULL) Touch = DefaultTouch;
				Touch(ActiveMenu, ActiveEntryIndex, inputdata.touch_x, inputdata.touch_y);
				break;
			}
		}

		break;
	}

	default:
		break;
	}
}

static void DefaultDisplayName(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	bool IsActive = (DrawnEntry == ActiveEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_ITEM : COLOR_INACTIVE_ITEM;
	if (IsActive) {
		show_icon(DS2_GetSubScreen(), &ICON_SUBSELA, SUBSELA_X, GUI_ROW1_Y + (Position - 1) * GUI_ROW_SY + SUBSELA_OFFSET_Y);
	}
	PRINT_STRING_BG(DS2_GetSubScreen(), *DrawnEntry->Name, TextColor, COLOR_TRANS, OPTION_TEXT_X, GUI_ROW1_Y + (Position - 1) * GUI_ROW_SY + TEXT_OFFSET_Y);
}

static void DefaultDisplayValue(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	if (DrawnEntry->Kind == KIND_OPTION || DrawnEntry->Kind == KIND_DISPLAY) {
		char Temp[21];
		Temp[0] = '\0';
		const char* Value = Temp;
		bool Error = false;
		if (DrawnEntry->Kind == KIND_OPTION) {
			if (*(uint32_t*) DrawnEntry->Target < DrawnEntry->ChoiceCount)
				Value = *DrawnEntry->Choices[*(uint32_t*) DrawnEntry->Target];
			else {
				Value = "Out of bounds";
				Error = true;
			}
		} else if (DrawnEntry->Kind == KIND_DISPLAY) {
			switch (DrawnEntry->DisplayType) {
				case TYPE_STRING:
					Value = (const char*) DrawnEntry->Target;
					break;
				case TYPE_INT32:
					sprintf(Temp, "%" PRIi32, *(int32_t*) DrawnEntry->Target);
					break;
				case TYPE_UINT32:
					sprintf(Temp, "%" PRIu32, *(uint32_t*) DrawnEntry->Target);
					break;
				case TYPE_INT64:
					sprintf(Temp, "%" PRIi64, *(int64_t*) DrawnEntry->Target);
					break;
				case TYPE_UINT64:
					sprintf(Temp, "%" PRIu64, *(uint64_t*) DrawnEntry->Target);
					break;
				default:
					Value = "Unknown type";
					Error = true;
					break;
			}
		}
		bool IsActive = (DrawnEntry == ActiveEntry);
		uint16_t TextColor = Error ? BGR555(0, 0, 31) : (IsActive ? COLOR_ACTIVE_ITEM : COLOR_INACTIVE_ITEM);
		uint32_t TextWidth = BDF_WidthUTF8s(Value);
		PRINT_STRING_BG(DS2_GetSubScreen(), Value, TextColor, COLOR_TRANS, DS_SCREEN_WIDTH - OPTION_TEXT_X - TextWidth, GUI_ROW1_Y + (Position - 1) * GUI_ROW_SY + TEXT_OFFSET_Y);
	}
}

static void DefaultDisplayBackground(struct Menu* ActiveMenu)
{
	show_icon(DS2_GetSubScreen(), &ICON_SUBBG, 0, 0);
}

static void DefaultDisplayData(struct Menu* ActiveMenu, struct Entry* ActiveEntry)
{
	uint32_t DrawnEntryIndex = 0;
	struct Entry* DrawnEntry = ActiveMenu->Entries[0];
	for (; DrawnEntry != NULL; DrawnEntryIndex++, DrawnEntry = ActiveMenu->Entries[DrawnEntryIndex]) {
		EntryDisplayFunction Function = DrawnEntry->DisplayName;
		if (Function == NULL) Function = &DefaultDisplayName;
		Function(DrawnEntry, ActiveEntry, DrawnEntryIndex);

		Function = DrawnEntry->DisplayValue;
		if (Function == NULL) Function = &DefaultDisplayValue;
		Function(DrawnEntry, ActiveEntry, DrawnEntryIndex);
	}
}

static void DefaultDisplayTitle(struct Menu* ActiveMenu)
{
	show_icon(DS2_GetSubScreen(), &ICON_TITLE, 0, 0);
	show_icon(DS2_GetSubScreen(), &ICON_TITLEICON, TITLE_ICON_X, TITLE_ICON_Y);

	draw_string_vcenter(DS2_GetSubScreen(), 0, 9, 256, COLOR_ACTIVE_ITEM, *ActiveMenu->Title);
}

// -- Shorthand for creating menu entries --

#define ENTRY_OPTION(_Name, _Target, _ChoiceCount) \
	.Kind = KIND_OPTION, .Name = _Name, .Target = _Target, .ChoiceCount = _ChoiceCount

#define ENTRY_DISPLAY(_Name, _Target, _DisplayType) \
	.Kind = KIND_DISPLAY, .Name = _Name, .Target = _Target, .DisplayType = _DisplayType

#define ENTRY_SUBMENU(_Name, _Target) \
	.Kind = KIND_SUBMENU, .Name = _Name, .Target = _Target

#define ENTRY_HOTKEY(_Name) \
	.Kind = KIND_CUSTOM, .Name = _Name, .DisplayValue = DisplayHotkey, .Enter = SetHotkey, .Touch = TouchEnter

#define ENTRY_MANDATORY_MAPPING(_Name) \
	.Kind = KIND_CUSTOM, .Name = _Name, .DisplayValue = DisplayMapping, .Enter = SetMapping, .Touch = TouchEnter

#define ENTRY_OPTIONAL_MAPPING(_Name) \
	.Kind = KIND_CUSTOM, .Name = _Name, .DisplayValue = DisplayMapping, .Enter = SetOrClearMapping, .Touch = TouchEnter

// -- Custom actions --

static bool CanFocusIfGameLoaded(struct Menu* ActiveMenu, struct Entry* ActiveEntry, uint32_t Position)
{
	return IsGameLoaded;
}

static void DisplayBack(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	if (DrawnEntry == ActiveEntry) {
		show_icon(DS2_GetSubScreen(), &ICON_BACK, BACK_BUTTON_X, BACK_BUTTON_Y);
	} else {
		show_icon(DS2_GetSubScreen(), &ICON_NBACK, BACK_BUTTON_X, BACK_BUTTON_Y);
	}
}

/* This function can be used to redirect any Entry's Touch function to its
 * Enter function. */
static void TouchEnter(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex, uint32_t X, uint32_t Y)
{
	ModifyFunction Enter = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Enter;
	if (Enter == NULL) Enter = DefaultEnter;
	Enter(ActiveMenu, ActiveEntryIndex);
}

static void TouchBoundsBack(struct Menu* ActiveMenu, struct Entry* ActiveEntry, uint32_t Position, struct TouchBounds* Bounds)
{
	*Bounds = (struct TouchBounds) {
		.X1 = BACK_BUTTON_TOUCH_X, .Y1 = 0,
		.X2 = DS_SCREEN_WIDTH, .Y2 = BACK_BUTTON_TOUCH_Y
	};
}

static struct Entry Back = {
	.Kind = KIND_CUSTOM,
	.DisplayName = DisplayBack,
	.Enter = DefaultLeave, .Touch = TouchEnter,
	.TouchBounds = TouchBoundsBack
};

/* --- MAIN MENU --- */

extern struct Menu MainMenu;

extern struct Menu AudioVideo;
extern struct Menu SavedStates;
extern struct Menu Tools;
extern struct Menu Options;
extern struct Menu NewGame;

static void DisplayBackgroundMainMenu(struct Menu* ActiveMenu)
{
	show_icon(DS2_GetSubScreen(), &ICON_MAINBG, 0, 0);
}

static void DisplayTitleMainMenu(struct Menu* ActiveMenu)
{
}

static void InputDispatchMainMenu(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	gui_action_type gui_action = get_gui_input();
	
	switch (gui_action) {
	case CURSOR_SELECT:
		if ((*ActiveMenu)->Entries[*ActiveEntryIndex] != NULL) {
			ModifyFunction Enter = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Enter;
			if (Enter == NULL) Enter = DefaultEnter;
			Enter(ActiveMenu, ActiveEntryIndex);
			break;
		}
		// otherwise, no entry has the focus, so SELECT acts like BACK
		// (fall through)

	case CURSOR_BACK:
	{
		ModifyFunction Leave = (*ActiveMenu)->Leave;
		if (Leave == NULL) Leave = DefaultLeave;
		Leave(ActiveMenu, ActiveEntryIndex);
		break;
	}

	/* The code in this function is complicated by the fact that
	 * option "2" (row 1, column 3, Cheats) is missing. The grid
	 * is as follows:
	 * 0 1
	 * 2 3 4
	 * 5 6 7 */
	case CURSOR_UP:
		     if (*ActiveEntryIndex < 2) *ActiveEntryIndex += 5;
		else if (*ActiveEntryIndex < 4) *ActiveEntryIndex -= 2;
		else if (*ActiveEntryIndex == 4) *ActiveEntryIndex = 7;
		else                            *ActiveEntryIndex -= 3;
		break;

	case CURSOR_DOWN:
		     if (*ActiveEntryIndex < 2) *ActiveEntryIndex += 2;
		else if (*ActiveEntryIndex < 5) *ActiveEntryIndex += 3;
		else if (*ActiveEntryIndex == 7) *ActiveEntryIndex = 4;
		else                            *ActiveEntryIndex -= 5;
		break;

	case CURSOR_LEFT:
		     if (*ActiveEntryIndex == 0) *ActiveEntryIndex += 1;
		else if (*ActiveEntryIndex == 2) *ActiveEntryIndex += 2;
		else if (*ActiveEntryIndex == 5) *ActiveEntryIndex += 2;
		else                             *ActiveEntryIndex -= 1;
		break;

	case CURSOR_RIGHT:
		     if (*ActiveEntryIndex == 1) *ActiveEntryIndex -= 1;
		else if (*ActiveEntryIndex == 4) *ActiveEntryIndex -= 2;
		else if (*ActiveEntryIndex == 7) *ActiveEntryIndex -= 2;
		else                             *ActiveEntryIndex += 1;
		break;

	case CURSOR_TOUCH:
	{
		struct DS_InputState inputdata;
		uint32_t EntryIndex;
		struct Entry* Entry;

		DS2_GetInputState(&inputdata);

		EntryIndex = inputdata.touch_x / 86 + (inputdata.touch_y / 80) * 3;

		if (EntryIndex == 2)  /* Where Cheats would be */
			break;
		else if (EntryIndex >= 3)
			EntryIndex--;

		Entry = (*ActiveMenu)->Entries[EntryIndex];

		EntryCanFocusFunction CanFocus = Entry->CanFocus;
		if (CanFocus == NULL) CanFocus = DefaultCanFocus;
		if (!CanFocus(*ActiveMenu, Entry, EntryIndex))
			break;

		*ActiveEntryIndex = EntryIndex;

		EntryTouchFunction Touch = Entry->Touch;
		if (Touch == NULL) Touch = DefaultTouch;
		Touch(ActiveMenu, ActiveEntryIndex, inputdata.touch_x, inputdata.touch_y);

		break;
	}

	default:
		break;
	}
}

static void DisplayAudioVideo(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	bool IsActive = (DrawnEntry == ActiveEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_MAIN : COLOR_INACTIVE_MAIN;
	show_icon(DS2_GetSubScreen(), IsActive ? &ICON_AVO : &ICON_NAVO, 19, 2);
	show_icon(DS2_GetSubScreen(), IsActive ? &ICON_MSEL : &ICON_MNSEL, 5, 57);
	draw_string_vcenter(DS2_GetSubScreen(), 7, 57, 75, TextColor, *DrawnEntry->Name);
}

static void DisplaySavedStates(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	bool IsActive = (DrawnEntry == ActiveEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_MAIN : COLOR_INACTIVE_MAIN;
	show_icon(DS2_GetSubScreen(), IsActive ? &ICON_SAVO : &ICON_NSAVO, 103, 2);
	show_icon(DS2_GetSubScreen(), IsActive ? &ICON_MSEL : &ICON_MNSEL, 89, 57);
	draw_string_vcenter(DS2_GetSubScreen(), 91, 57, 75, TextColor, *DrawnEntry->Name);
}

static void DisplayTools(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	bool IsActive = (DrawnEntry == ActiveEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_MAIN : COLOR_INACTIVE_MAIN;
	show_icon(DS2_GetSubScreen(), IsActive ? &ICON_TOOL : &ICON_NTOOL, 19, 75);
	show_icon(DS2_GetSubScreen(), IsActive ? &ICON_MSEL : &ICON_MNSEL, 5, 131);
	draw_string_vcenter(DS2_GetSubScreen(), 7, 131, 75, TextColor, *DrawnEntry->Name);
}

static void DisplayOptions(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	bool IsActive = (DrawnEntry == ActiveEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_MAIN : COLOR_INACTIVE_MAIN;
	show_icon(DS2_GetSubScreen(), IsActive ? &ICON_OTHER : &ICON_NOTHER, 103, 75);
	show_icon(DS2_GetSubScreen(), IsActive ? &ICON_MSEL : &ICON_MNSEL, 89, 131);
	draw_string_vcenter(DS2_GetSubScreen(), 91, 131, 75, TextColor, *DrawnEntry->Name);
}

static void ActionExit(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	// Please ensure that the Main Menu itself does not have entries of type
	// KIND_OPTION. The on-demand writing of settings to storage will not
	// occur after quit(), since it acts after the action function returns.
	quit();
	*ActiveMenu = NULL;
}

static void DisplayExit(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	bool IsActive = (DrawnEntry == ActiveEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_MAIN : COLOR_INACTIVE_MAIN;
	show_icon(DS2_GetSubScreen(), IsActive ? &ICON_EXIT : &ICON_NEXIT, 187, 75);
	show_icon(DS2_GetSubScreen(), IsActive ? &ICON_MSEL : &ICON_MNSEL, 173, 131);
	draw_string_vcenter(DS2_GetSubScreen(), 175, 131, 75, TextColor, *DrawnEntry->Name);
}

static void ActionReturn(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	if (IsGameLoaded) {
		*ActiveMenu = NULL;
	}
}

static void DisplayReturn(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	bool IsActive = (DrawnEntry == ActiveEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_MAIN : COLOR_INACTIVE_MAIN;
	show_icon(DS2_GetSubScreen(), IsActive ? &ICON_MAINITEM : &ICON_NMAINITEM, 170, 154);
	draw_string_vcenter(DS2_GetSubScreen(), 170, 165, 85, TextColor, *DrawnEntry->Name);
}

static void ActionReset(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	if (IsGameLoaded) {
		reset_gba();
		reg[CHANGED_PC_STATUS] = 1;
		*ActiveMenu = NULL;
	}
}

static void DisplayReset(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	bool IsActive = (DrawnEntry == ActiveEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_MAIN : COLOR_INACTIVE_MAIN;
	show_icon(DS2_GetSubScreen(), IsActive ? &ICON_MAINITEM : &ICON_NMAINITEM, 85, 154);
	draw_string_vcenter(DS2_GetSubScreen(), 85, 165, 85, TextColor, *DrawnEntry->Name);
}

static void DisplayNewGame(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	bool IsActive = (DrawnEntry == ActiveEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_MAIN : COLOR_INACTIVE_MAIN;
	show_icon(DS2_GetSubScreen(), IsActive ? &ICON_MAINITEM : &ICON_NMAINITEM, 0, 154);
	draw_string_vcenter(DS2_GetSubScreen(), 0, 165, 85, TextColor, *DrawnEntry->Name);
}

static struct Entry MainMenu_AudioVideo = {
	ENTRY_SUBMENU(&msg[MSG_MAIN_MENU_VIDEO_AUDIO], &AudioVideo),
	.DisplayName = DisplayAudioVideo
};

static struct Entry MainMenu_SavedStates = {
	ENTRY_SUBMENU(&msg[MSG_MAIN_MENU_SAVED_STATES], &SavedStates),
	.DisplayName = DisplaySavedStates
};

static struct Entry MainMenu_Tools = {
	ENTRY_SUBMENU(&msg[MSG_MAIN_MENU_TOOLS], &Tools),
	.DisplayName = DisplayTools
};

static struct Entry MainMenu_Options = {
	ENTRY_SUBMENU(&msg[MSG_MAIN_MENU_OPTIONS], &Options),
	.DisplayName = DisplayOptions
};

static struct Entry MainMenu_Exit = {
	.Kind = KIND_CUSTOM, .Name = &msg[MSG_MAIN_MENU_EXIT],
	.Enter = ActionExit, .Touch = TouchEnter,
	.DisplayName = DisplayExit
};

static struct Entry MainMenu_NewGame = {
	ENTRY_SUBMENU(&msg[MSG_MAIN_MENU_NEW_GAME], &NewGame),
	.DisplayName = DisplayNewGame
};

static struct Entry MainMenu_Reset = {
	.Kind = KIND_CUSTOM, .Name = &msg[MSG_MAIN_MENU_RESET_GAME],
	.Enter = ActionReset, .Touch = TouchEnter,
	.DisplayName = DisplayReset
};

static struct Entry MainMenu_Return = {
	.Kind = KIND_CUSTOM, .Name = &msg[MSG_MAIN_MENU_RETURN_TO_GAME],
	.Enter = ActionReturn, .Touch = TouchEnter,
	.DisplayName = DisplayReturn
};

struct Menu MainMenu = {
	.Parent = NULL, .Title = NULL,
	.DisplayBackground = DisplayBackgroundMainMenu, .DisplayTitle = DisplayTitleMainMenu,
	.InputDispatch = InputDispatchMainMenu,
	.Leave = ActionReturn,
	.Entries = { &MainMenu_AudioVideo, &MainMenu_SavedStates, &MainMenu_Tools, &MainMenu_Options, &MainMenu_Exit, &MainMenu_NewGame, &MainMenu_Reset, &MainMenu_Return, NULL },
	.ActiveEntryIndex = 5  /* Start out at New Game */
};

/* --- Main Menu > AUDIO & VIDEO --- */

static struct Entry AudioVideo_FastForward = {
	ENTRY_OPTION(&msg[MSG_VIDEO_FAST_FORWARD], &game_fast_forward, 2),
	.Choices = { &msg[MSG_GENERAL_OFF], &msg[MSG_GENERAL_ON] },
	.Action = game_set_frameskip
};

static struct Entry AudioVideo_Sound = {
	ENTRY_OPTION(&msg[MSG_AUDIO_SOUND], &sound_on, 2),
	.Choices = { &msg[MSG_AUDIO_MUTED], &msg[MSG_AUDIO_ENABLED] }
};

static struct Entry AudioVideo_Frameskip = {
	ENTRY_OPTION(&msg[MSG_VIDEO_FRAME_SKIPPING], &game_persistent_config.frameskip_value, 12),
	.Choices = { &msg[MSG_VIDEO_FRAME_SKIPPING_AUTOMATIC], &msg[MSG_VIDEO_FRAME_SKIPPING_0],
	             &msg[MSG_VIDEO_FRAME_SKIPPING_1], &msg[MSG_VIDEO_FRAME_SKIPPING_2],
	             &msg[MSG_VIDEO_FRAME_SKIPPING_3], &msg[MSG_VIDEO_FRAME_SKIPPING_4],
	             &msg[MSG_VIDEO_FRAME_SKIPPING_5], &msg[MSG_VIDEO_FRAME_SKIPPING_6],
	             &msg[MSG_VIDEO_FRAME_SKIPPING_7], &msg[MSG_VIDEO_FRAME_SKIPPING_8],
	             &msg[MSG_VIDEO_FRAME_SKIPPING_9], &msg[MSG_VIDEO_FRAME_SKIPPING_10] },
	.Action = game_set_frameskip
};

static struct Entry AudioVideo_DisplayFPS = {
	ENTRY_OPTION(&msg[MSG_VIDEO_FRAMES_PER_SECOND_COUNTER], &gpsp_persistent_config.DisplayFPS, 2),
	.Choices = { &msg[MSG_GENERAL_OFF], &msg[MSG_GENERAL_ON] }
};

static struct Entry AudioVideo_BootSelection = {
	ENTRY_OPTION(&msg[MSG_VIDEO_BOOT_MODE], &gpsp_persistent_config.BootFromBIOS, 2),
	.Choices = { &msg[MSG_VIDEO_BOOT_MODE_CARTRIDGE], &msg[MSG_VIDEO_BOOT_MODE_LOGO] }
};

static struct Entry AudioVideo_GameScreen = {
	ENTRY_OPTION(&msg[MSG_VIDEO_GAME_SCREEN], &gpsp_persistent_config.BottomScreenGame, 2),
	.Choices = { &msg[MSG_VIDEO_GAME_SCREEN_TOP], &msg[MSG_VIDEO_GAME_SCREEN_BOTTOM] }
};

struct Menu AudioVideo = {
	.Parent = &MainMenu, .Title = &msg[MSG_MAIN_MENU_VIDEO_AUDIO],
	.Entries = { &Back, &AudioVideo_FastForward, &AudioVideo_Sound, &AudioVideo_Frameskip, &AudioVideo_DisplayFPS, &AudioVideo_BootSelection, &AudioVideo_GameScreen, NULL },
	.ActiveEntryIndex = 1  /* Start out after Back */
};

/* --- Main Menu > SAVED STATES --- */

static void PostChangeSavedState()
{
	char file[PATH_MAX];

	if (IsGameLoaded) {
		if (SavedStateFileExists(savestate_index)) {
			ReGBA_GetSavedStateFilename(file, CurrentGamePath, savestate_index);
			load_game_stat_snapshot(file);
		} else {
			DS2_FillScreen(DS_ENGINE_MAIN, COLOR_BLACK);
			draw_string_vcenter(DS2_GetMainScreen(), 0, 88, 256, COLOR_WHITE, msg[MSG_TOP_SCREEN_NO_SAVED_STATE_IN_SLOT]);
			DS2_FlipMainScreen();
		}
	} else {
		DS2_FillScreen(DS_ENGINE_MAIN, COLOR_BLACK);
		draw_string_vcenter(DS2_GetMainScreen(), 0, 88, 256, COLOR_WHITE, msg[MSG_TOP_SCREEN_NO_GAME_LOADED]);
		DS2_FlipMainScreen();
	}
}

static void clear_savestate_slot(uint32_t index)
{
	char file[PATH_MAX];

	ReGBA_GetSavedStateFilename(file, CurrentGamePath, index);
	remove(file);
	SavedStateCacheInvalidate();
}

static void InitSavedStates(struct Menu** ActiveMenu)
{
	PostChangeSavedState();
}

static void EndSavedStates(struct Menu* ActiveMenu)
{
	DS2_FillScreen(DS_ENGINE_MAIN, COLOR_BLACK);
	if (IsGameLoaded) {
		blit_to_screen(MainMenu.UserData, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT, -1, -1);
	} else {
		draw_string_vcenter(DS2_GetMainScreen(), 0, 88, 256, COLOR_WHITE, msg[MSG_TOP_SCREEN_NO_GAME_LOADED]);
	}
	DS2_FlipMainScreen();
}

static void SaveState(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	if (IsGameLoaded) {
		if (SavedStateFileExists(savestate_index)) {
			draw_message_box(DS2_GetSubScreen());
			draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_SAVESTATE_FULL]);
			if (draw_yesno_dialog(DS_ENGINE_SUB, msg[MSG_GENERAL_CONFIRM_WITH_A], msg[MSG_GENERAL_CANCEL_WITH_B]) == 0)
				return;

			clear_savestate_slot(savestate_index);
		}

		draw_message_box(DS2_GetSubScreen());
		draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SAVED_STATE_CREATING]);
		DS2_UpdateScreen(DS_ENGINE_SUB);

		HighFrequencyCPU();
		int flag = save_state(savestate_index, MainMenu.UserData);
		LowFrequencyCPU();

		draw_message_box(DS2_GetSubScreen());
		if (flag < 0) {
			draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SAVED_STATE_CREATION_FAILED]);
		} else {
			draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SAVED_STATE_CREATION_SUCCEEDED]);
		}

		DS2_UpdateScreen(DS_ENGINE_SUB);

		SavedStateCacheInvalidate();

		usleep(500000); // let the progress message linger
		PostChangeSavedState();
	}
}

static void LoadState(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	if (IsGameLoaded) {
		draw_message_box(DS2_GetSubScreen());
		draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SAVED_STATE_LOADING]);
		DS2_UpdateScreen(DS_ENGINE_SUB);

		HighFrequencyCPU();
		int flag = load_state(savestate_index);
		LowFrequencyCPU();
		if (flag == 0) {
			draw_message_box(DS2_GetSubScreen());
			draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SAVED_STATE_LOAD_SUCCEEDED]);
			DS2_UpdateScreen(DS_ENGINE_SUB);
			*ActiveMenu = NULL;
		} else {
			draw_message_box(DS2_GetSubScreen());
			draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SAVED_STATE_LOAD_FAILED]);
			DS2_UpdateScreen(DS_ENGINE_SUB);
			usleep(500000); // let the failure show
		}
	}
}

static void DeleteState(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	char Text[4096];

	if (IsGameLoaded) {
		draw_message_box(DS2_GetSubScreen());

		if (SavedStateFileExists(savestate_index)) {
			sprintf(Text, msg[FMT_DIALOG_SAVED_STATE_DELETE_ONE], savestate_index + 1);
			draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, Text);

			if (draw_yesno_dialog(DS_ENGINE_SUB, msg[MSG_GENERAL_CONFIRM_WITH_A], msg[MSG_GENERAL_CANCEL_WITH_B])) {
				DS2_AwaitNoButtons();
				clear_savestate_slot(savestate_index);
				PostChangeSavedState();
			}
		} else {
			draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SAVED_STATE_ALREADY_EMPTY]);
			DS2_UpdateScreen(DS_ENGINE_SUB);
			usleep(500000);
		}
	}
}

static void DeleteAllStates()
{
	if (IsGameLoaded) {
		uint32_t i;
		bool AnyExists = false;

		draw_message_box(DS2_GetSubScreen());
		draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_DIALOG_SAVED_STATE_DELETE_ALL]);

		for (i = 0; i < SAVE_STATE_SLOT_NUM; i++) {
			if (SavedStateFileExists(i)) {
				AnyExists = true;
				break;
			}
		}

		if (AnyExists) {
			if (draw_yesno_dialog(DS_ENGINE_SUB, msg[MSG_GENERAL_CONFIRM_WITH_A], msg[MSG_GENERAL_CANCEL_WITH_B])) {
				DS2_AwaitNoButtons();
				for (i = 0; i < SAVE_STATE_SLOT_NUM; i++) {
					clear_savestate_slot(i);
				}
				PostChangeSavedState();
				savestate_index = 0;
			}
		} else {
			draw_message_box(DS2_GetSubScreen());
			draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SAVED_STATE_ALREADY_EMPTY]);
			DS2_UpdateScreen(DS_ENGINE_SUB);
			usleep(500000);
		}
	}
}

static void DisplayNameSavedStates(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	DefaultDisplayName(DrawnEntry, ActiveEntry, Position * 2 - 1);
}

static void DisplayValueSavedStates(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	uint32_t i;

	if (DrawnEntry == ActiveEntry) {
		char Value[11];
		sprintf(Value, "%" PRIu32, (uint32_t) (savestate_index + 1));

		uint32_t TextWidth = BDF_WidthUTF8s(Value);
		PRINT_STRING_BG(DS2_GetSubScreen(), Value, COLOR_ACTIVE_ITEM, COLOR_TRANS, DS_SCREEN_WIDTH - OPTION_TEXT_X - TextWidth, GUI_ROW1_Y + (Position * 2 - 2) * GUI_ROW_SY + TEXT_OFFSET_Y);
	}

	for (i = 0; i < SAVE_STATE_SLOT_NUM; i++) {
		bool Selected = (*(uint32_t*) DrawnEntry->Target == i),
		     Exists = SavedStateFileExists(i);
		uint8_t X = SavedStateSquareX(i);

		show_icon(DS2_GetSubScreen(),
			Selected ? (Exists ? &ICON_STATEFULL : &ICON_STATEEMPTY)
			         : (Exists ? &ICON_NSTATEFULL : &ICON_NSTATEEMPTY),
			X,
			GUI_ROW1_Y + (Position * 2 - 1) * GUI_ROW_SY + (GUI_ROW_SY - ICON_STATEFULL.y) / 2);
	}
}

static void TouchBoundsSavedStates(struct Menu* ActiveMenu, struct Entry* ActiveEntry, uint32_t Position, struct TouchBounds* Bounds)
{
	*Bounds = (struct TouchBounds) {
		.X1 = 0, .Y1 = GUI_ROW1_Y + (Position * 2 - 2) * GUI_ROW_SY,
		.X2 = 256, .Y2 = GUI_ROW1_Y + (Position * 2) * GUI_ROW_SY
	};
}

static void TouchBoundsDeleteAll(struct Menu* ActiveMenu, struct Entry* ActiveEntry, uint32_t Position, struct TouchBounds* Bounds)
{
	DefaultTouchBounds(ActiveMenu, ActiveEntry, Position * 2 - 1, Bounds);
}

static void TouchSavedStates(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex, uint32_t X, uint32_t Y)
{
	if (Y < GUI_ROW1_Y + (*ActiveEntryIndex * 2 - 1) * GUI_ROW_SY) {
		/* Touching the Create/Load/Delete row itself */
		DefaultTouch(ActiveMenu, ActiveEntryIndex, X, Y);
	} else {
		/* Touching the row of squares below */
		uint32_t i;

		for (i = 0; i < SAVE_STATE_SLOT_NUM; i++) {
			uint8_t StartX = SavedStateSquareX(i);
			if (X >= StartX && X < StartX + ICON_STATEFULL.x)
				break;
		}

		if (i < SAVE_STATE_SLOT_NUM) {  /* A square was touched */
			if (i == savestate_index) {  /* Touched the selected square */
				ModifyFunction Enter = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Enter;
				if (Enter == NULL) Enter = DefaultEnter;
				Enter(ActiveMenu, ActiveEntryIndex);
			} else {  /* Touched a new square */
				savestate_index = i;

				ActionFunction Action = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Action;
				if (Action != NULL)
					Action();
			}
		}
	}
}

static struct Entry SavedStates_Create = {
	ENTRY_OPTION(&msg[MSG_SAVED_STATE_CREATE], &savestate_index, SAVE_STATE_SLOT_NUM),
	.DisplayName = DisplayNameSavedStates, .DisplayValue = DisplayValueSavedStates,
	.TouchBounds = TouchBoundsSavedStates, .Touch = TouchSavedStates,
	.Action = PostChangeSavedState,
	.Enter = SaveState
};

static struct Entry SavedStates_Load = {
	ENTRY_OPTION(&msg[MSG_SAVED_STATE_LOAD], &savestate_index, SAVE_STATE_SLOT_NUM),
	.DisplayName = DisplayNameSavedStates, .DisplayValue = DisplayValueSavedStates,
	.TouchBounds = TouchBoundsSavedStates, .Touch = TouchSavedStates,
	.Action = PostChangeSavedState,
	.Enter = LoadState
};

static struct Entry SavedStates_Delete = {
	ENTRY_OPTION(&msg[MSG_SAVED_STATE_DELETE_ONE], &savestate_index, SAVE_STATE_SLOT_NUM),
	.DisplayName = DisplayNameSavedStates, .DisplayValue = DisplayValueSavedStates,
	.TouchBounds = TouchBoundsSavedStates, .Touch = TouchSavedStates,
	.Action = PostChangeSavedState,
	.Enter = DeleteState
};

static struct Entry SavedStates_DeleteAll = {
	.Kind = KIND_ACTION, .Name = &msg[MSG_SAVED_STATE_DELETE_ALL],
	.DisplayName = DisplayNameSavedStates,
	.TouchBounds = TouchBoundsDeleteAll,
	.Action = DeleteAllStates
};

struct Menu SavedStates = {
	.Parent = &MainMenu, .Title = &msg[MSG_MAIN_MENU_SAVED_STATES],
	.Entries = { &Back, &SavedStates_Create, &SavedStates_Load, &SavedStates_Delete, &SavedStates_DeleteAll, NULL },
	.Init = InitSavedStates, .End = EndSavedStates,
	.ActiveEntryIndex = 1  /* Start out after Back */
};

/* --- Main Menu > TOOLS --- */

extern struct Menu Screenshots;
extern struct Menu GlobalHotkeys;
extern struct Menu GameHotkeys;
extern struct Menu GlobalMappings;
extern struct Menu GameMappings;
extern struct Menu Debugging;

static struct Entry Tools_Screenshots = {
	ENTRY_SUBMENU(&msg[MSG_TOOLS_SCREENSHOT_GENERAL], &Screenshots)
};

static struct Entry Tools_GlobalHotkeys = {
	ENTRY_SUBMENU(&msg[MSG_TOOLS_GLOBAL_HOTKEY_GENERAL], &GlobalHotkeys)
};

static struct Entry Tools_GameHotkeys = {
	ENTRY_SUBMENU(&msg[MSG_TOOLS_GAME_HOTKEY_GENERAL], &GameHotkeys),
	.CanFocus = CanFocusIfGameLoaded
};

static struct Entry Tools_GlobalMappings = {
	ENTRY_SUBMENU(&msg[MSG_TOOLS_GLOBAL_BUTTON_MAPPING_GENERAL], &GlobalMappings)
};

static struct Entry Tools_GameMappings = {
	ENTRY_SUBMENU(&msg[MSG_TOOLS_GAME_BUTTON_MAPPING_GENERAL], &GameMappings),
	.CanFocus = CanFocusIfGameLoaded
};

static struct Entry Tools_Rewind = {
	ENTRY_OPTION(&msg[MSG_VIDEO_REWINDING], &game_persistent_config.rewind_value, 7),
	.Choices = { &msg[MSG_VIDEO_REWINDING_0], &msg[MSG_VIDEO_REWINDING_1],
	             &msg[MSG_VIDEO_REWINDING_2], &msg[MSG_VIDEO_REWINDING_3],
	             &msg[MSG_VIDEO_REWINDING_4], &msg[MSG_VIDEO_REWINDING_5],
	             &msg[MSG_VIDEO_REWINDING_6] },
	.Action = game_set_rewind
};

static struct Entry Tools_Debugging = {
	ENTRY_SUBMENU(&msg[MSG_TOOLS_DEBUG_MENU_ENGLISH], &Debugging),
	.CanFocus = CanFocusIfGameLoaded
};

struct Menu Tools = {
	.Parent = &MainMenu, .Title = &msg[MSG_MAIN_MENU_TOOLS],
	.Entries = { &Back, &Tools_Screenshots, &Tools_GlobalHotkeys, &Tools_GameHotkeys, &Tools_GlobalMappings, &Tools_GameMappings, &Tools_Rewind, &Tools_Debugging, NULL },
	.ActiveEntryIndex = 1  /* Start out after Back */
};

/* --- Main Menu > Tools > SCREENSHOTS --- */

void save_screen_snapshot()
{
	draw_message_box(DS2_GetSubScreen());
	if (IsGameLoaded) {
		draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SCREENSHOT_CREATING]);
		DS2_UpdateScreen(DS_ENGINE_SUB);
		if (save_ss_bmp(MainMenu.UserData /* saved screenshot */)) {
			draw_message_box(DS2_GetSubScreen());
			draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SCREENSHOT_CREATION_SUCCEEDED]);
		} else {
			draw_message_box(DS2_GetSubScreen());
			draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SCREENSHOT_CREATION_FAILED]);
		}
	} else {
		draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_TOP_SCREEN_NO_SAVED_STATE_IN_SLOT]);
	}
	DS2_UpdateScreen(DS_ENGINE_SUB);
	usleep(500000);
}

static struct Entry Screenshots_Save = {
	.Kind = KIND_ACTION, .Name = &msg[MSG_SCREENSHOT_CREATE],
	.Action = save_screen_snapshot
};

static struct Entry Screenshots_Browse = {
	.Kind = KIND_ACTION, .Name = &msg[MSG_SCREENSHOT_BROWSE],
	.Action = play_screen_snapshot
};

struct Menu Screenshots = {
	.Parent = &Tools, .Title = &msg[MSG_TOOLS_SCREENSHOT_GENERAL],
	.Entries = { &Back, &Screenshots_Save, &Screenshots_Browse, NULL },
	.ActiveEntryIndex = 1  /* Start out after Back */
};

/* --- Main Menu > Tools > GLOBAL HOTKEYS --- */

void DisplayHotkey(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	bool IsActive = (DrawnEntry == ActiveEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_ITEM : COLOR_INACTIVE_ITEM;
	uint32_t Hotkey = *(uint32_t*) DrawnEntry->Target;
	bool Inherited = false;
	char Value[512];

	if (Hotkey == 0 && DrawnEntry->UserData != NULL) {
		Hotkey = *(uint32_t*) DrawnEntry->UserData;
		Inherited = true;
	}

	// Construct a UTF-8 string showing the buttons in the bitfield.
	Value[0] = '\0';
	if (Hotkey & DS_BUTTON_L)      strcpy(Value, HOTKEY_L_DISPLAY);
	if (Hotkey & DS_BUTTON_R)      strcat(Value, HOTKEY_R_DISPLAY);
	if (Hotkey & DS_BUTTON_A)      strcat(Value, HOTKEY_A_DISPLAY);
	if (Hotkey & DS_BUTTON_B)      strcat(Value, HOTKEY_B_DISPLAY);
	if (Hotkey & DS_BUTTON_Y)      strcat(Value, HOTKEY_Y_DISPLAY);
	if (Hotkey & DS_BUTTON_X)      strcat(Value, HOTKEY_X_DISPLAY);
	if (Hotkey & DS_BUTTON_START)  strcat(Value, HOTKEY_START_DISPLAY);
	if (Hotkey & DS_BUTTON_SELECT) strcat(Value, HOTKEY_SELECT_DISPLAY);
	if (Hotkey & DS_BUTTON_UP)     strcat(Value, HOTKEY_UP_DISPLAY);
	if (Hotkey & DS_BUTTON_DOWN)   strcat(Value, HOTKEY_DOWN_DISPLAY);
	if (Hotkey & DS_BUTTON_LEFT)   strcat(Value, HOTKEY_LEFT_DISPLAY);
	if (Hotkey & DS_BUTTON_RIGHT)  strcat(Value, HOTKEY_RIGHT_DISPLAY);

	if (Inherited && Hotkey != 0)
		strcat(Value, msg[MSG_BUTTON_MAPPING_INHERITED_FROM_GLOBAL]);

	uint32_t TextWidth = BDF_WidthUTF8s(Value);
	PRINT_STRING_BG(DS2_GetSubScreen(), Value, TextColor, COLOR_TRANS, DS_SCREEN_WIDTH - OPTION_TEXT_X - TextWidth, GUI_ROW1_Y + (Position - 1) * GUI_ROW_SY + TEXT_OFFSET_Y);
}

static void SetHotkey(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	uint32_t* Hotkey = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Target;

	draw_message_box(DS2_GetSubScreen());
	draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_HOTKEY_WAITING_FOR_KEYS]);

	uint16_t Keys = draw_hotkey_dialog(DS_ENGINE_SUB, msg[MSG_HOTKEY_DELETE_WITH_A], msg[MSG_HOTKEY_CANCEL_WITH_B]);
	if (Keys == DS_BUTTON_B)
		; // unmodified
	else if (Keys == DS_BUTTON_A)
		*Hotkey = 0; // clear
	else
		*Hotkey = Keys; // set
}

static struct Entry GlobalHotkeys_ReturnToMenu = {
	ENTRY_HOTKEY(&msg[MSG_HOTKEY_MAIN_MENU]),
	.Target = &gpsp_persistent_config.HotkeyReturnToMenu
};

static struct Entry GlobalHotkeys_FastForward = {
	ENTRY_HOTKEY(&msg[MSG_HOTKEY_TEMPORARY_FAST_FORWARD]),
	.Target = &gpsp_persistent_config.HotkeyTemporaryFastForward
};

static struct Entry GlobalHotkeys_Rewind = {
	ENTRY_HOTKEY(&msg[MSG_HOTKEY_REWIND]),
	.Target = &gpsp_persistent_config.HotkeyRewind
};

static struct Entry GlobalHotkeys_ToggleSound = {
	ENTRY_HOTKEY(&msg[MSG_HOTKEY_SOUND_TOGGLE]),
	.Target = &gpsp_persistent_config.HotkeyToggleSound
};

static struct Entry GlobalHotkeys_QuickSaveState = {
	ENTRY_HOTKEY(&msg[MSG_HOTKEY_QUICK_SAVE_STATE]),
	.Target = &gpsp_persistent_config.HotkeyQuickSaveState
};

static struct Entry GlobalHotkeys_QuickLoadState = {
	ENTRY_HOTKEY(&msg[MSG_HOTKEY_QUICK_LOAD_STATE]),
	.Target = &gpsp_persistent_config.HotkeyQuickLoadState
};

struct Menu GlobalHotkeys = {
	.Parent = &Tools, .Title = &msg[MSG_TOOLS_GLOBAL_HOTKEY_GENERAL],
	.Entries = { &Back, &GlobalHotkeys_ReturnToMenu, &GlobalHotkeys_FastForward, &GlobalHotkeys_Rewind, &GlobalHotkeys_ToggleSound, &GlobalHotkeys_QuickSaveState, &GlobalHotkeys_QuickLoadState, NULL },
	.ActiveEntryIndex = 1  /* Start out after Back */
};

/* --- Main Menu > Tools > GAME-SPECIFIC HOTKEYS --- */

static struct Entry GameHotkeys_ReturnToMenu = {
	ENTRY_HOTKEY(&msg[MSG_HOTKEY_MAIN_MENU]),
	.Target = &game_persistent_config.HotkeyReturnToMenu,
	.UserData = &gpsp_persistent_config.HotkeyReturnToMenu
};

static struct Entry GameHotkeys_FastForward = {
	ENTRY_HOTKEY(&msg[MSG_HOTKEY_TEMPORARY_FAST_FORWARD]),
	.Target = &game_persistent_config.HotkeyTemporaryFastForward,
	.UserData = &gpsp_persistent_config.HotkeyTemporaryFastForward
};

static struct Entry GameHotkeys_Rewind = {
	ENTRY_HOTKEY(&msg[MSG_HOTKEY_REWIND]),
	.Target = &game_persistent_config.HotkeyRewind,
	.UserData = &gpsp_persistent_config.HotkeyRewind
};

static struct Entry GameHotkeys_ToggleSound = {
	ENTRY_HOTKEY(&msg[MSG_HOTKEY_SOUND_TOGGLE]),
	.Target = &game_persistent_config.HotkeyToggleSound,
	.UserData = &gpsp_persistent_config.HotkeyToggleSound
};

static struct Entry GameHotkeys_QuickSaveState = {
	ENTRY_HOTKEY(&msg[MSG_HOTKEY_QUICK_SAVE_STATE]),
	.Target = &game_persistent_config.HotkeyQuickSaveState,
	.UserData = &gpsp_persistent_config.HotkeyQuickSaveState
};

static struct Entry GameHotkeys_QuickLoadState = {
	ENTRY_HOTKEY(&msg[MSG_HOTKEY_QUICK_LOAD_STATE]),
	.Target = &game_persistent_config.HotkeyQuickLoadState,
	.UserData = &gpsp_persistent_config.HotkeyQuickLoadState
};

struct Menu GameHotkeys = {
	.Parent = &Tools, .Title = &msg[MSG_TOOLS_GAME_HOTKEY_GENERAL],
	.Entries = { &Back, &GameHotkeys_ReturnToMenu, &GameHotkeys_FastForward, &GameHotkeys_Rewind, &GameHotkeys_ToggleSound, &GameHotkeys_QuickSaveState, &GameHotkeys_QuickLoadState, NULL },
	.ActiveEntryIndex = 1  /* Start out after Back */
};

/* --- Main Menu > Tools > GLOBAL MAPPINGS --- */

static void DisplayMapping(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	bool IsActive = (DrawnEntry == ActiveEntry);
	uint32_t Mapping = *(uint32_t*) DrawnEntry->Target;
	bool Inherited = false, Error = false;
	char temp[512];
	const char* Value = temp;
	temp[0] = '\0';

	if (Mapping == 0 && DrawnEntry->UserData != NULL) {
		Mapping = *(uint32_t*) DrawnEntry->UserData;
		Inherited = true;
	}

	if ((Mapping & (Mapping - 1)) != 0) {
		/* More than 1 bit is set. */
		Value = "Out of bounds";
		Error = true;
	} else {
		switch (Mapping) {
		case 0:                                                     break;
		case DS_BUTTON_L:      strcpy(temp, HOTKEY_L_DISPLAY);      break;
		case DS_BUTTON_R:      strcpy(temp, HOTKEY_R_DISPLAY);      break;
		case DS_BUTTON_A:      strcpy(temp, HOTKEY_A_DISPLAY);      break;
		case DS_BUTTON_B:      strcpy(temp, HOTKEY_B_DISPLAY);      break;
		case DS_BUTTON_Y:      strcpy(temp, HOTKEY_Y_DISPLAY);      break;
		case DS_BUTTON_X:      strcpy(temp, HOTKEY_X_DISPLAY);      break;
		case DS_BUTTON_START:  strcpy(temp, HOTKEY_START_DISPLAY);  break;
		case DS_BUTTON_SELECT: strcpy(temp, HOTKEY_SELECT_DISPLAY); break;
		case DS_BUTTON_UP:     strcpy(temp, HOTKEY_UP_DISPLAY);     break;
		case DS_BUTTON_DOWN:   strcpy(temp, HOTKEY_DOWN_DISPLAY);   break;
		case DS_BUTTON_LEFT:   strcpy(temp, HOTKEY_LEFT_DISPLAY);   break;
		case DS_BUTTON_RIGHT:  strcpy(temp, HOTKEY_RIGHT_DISPLAY);  break;
		default:
			Value = "Out of bounds";
			Error = true;
			break;
		}
	}

	if (Inherited && Mapping != 0)
		strcat(temp, msg[MSG_BUTTON_MAPPING_INHERITED_FROM_GLOBAL]);

	uint16_t TextColor = Error ? BGR555(0, 0, 31) : (IsActive ? COLOR_ACTIVE_ITEM : COLOR_INACTIVE_ITEM);
	uint32_t TextWidth = BDF_WidthUTF8s(Value);
	PRINT_STRING_BG(DS2_GetSubScreen(), Value, TextColor, COLOR_TRANS, DS_SCREEN_WIDTH - OPTION_TEXT_X - TextWidth, GUI_ROW1_Y + (Position - 1) * GUI_ROW_SY + TEXT_OFFSET_Y);
}

static void SetMapping(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	uint32_t* Mapping = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Target;
	struct DS_InputState inputdata;
	// These are the recognised buttons.
	const uint16_t Mask = DS_BUTTON_A | DS_BUTTON_B | DS_BUTTON_X | DS_BUTTON_Y | DS_BUTTON_START | DS_BUTTON_SELECT | DS_BUTTON_L | DS_BUTTON_R;

	draw_message_box(DS2_GetSubScreen());
	draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_MAPPING_WAITING_FOR_KEY]);
	DS2_UpdateScreen(DS_ENGINE_SUB);

	DS2_AwaitNoButtons(); // Originate from a keypress
	// Wait for a recognised key to become pressed
	do {
		DS2_AwaitInputChange(&inputdata);
	} while ((inputdata.buttons & Mask) == 0);

	// In case the user pressed multiple buttons at once, take the lowest set
	// bit.
	*Mapping = inputdata.buttons & (~inputdata.buttons + 1);
	set_button_map();
	DS2_AwaitNoButtons();
}

static void SetOrClearMapping(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	uint32_t* Mapping = (*ActiveMenu)->Entries[*ActiveEntryIndex]->Target;
	struct DS_InputState inputdata;
	// These are the recognised buttons.
	const uint16_t Mask = DS_BUTTON_A | DS_BUTTON_B | DS_BUTTON_X | DS_BUTTON_Y | DS_BUTTON_START | DS_BUTTON_SELECT | DS_BUTTON_L | DS_BUTTON_R;

	draw_message_box(DS2_GetSubScreen());
	draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_MAPPING_WAITING_FOR_KEY_OR_CLEAR]);
	DS2_UpdateScreen(DS_ENGINE_SUB);

	DS2_AwaitNoButtons(); // Originate from a keypress
	// Wait for a recognised key to become pressed
	do {
		DS2_AwaitInputChange(&inputdata);
	} while ((inputdata.buttons & Mask) == 0);

	uint16_t TotalKeys = 0;
	do {
		TotalKeys |= inputdata.buttons & Mask;
		DS2_AwaitInputChange(&inputdata);
	} while ((inputdata.buttons & Mask) != 0);

	// If 2 or more buttons are pressed, clear the mapping, otherwise set it.
	if ((TotalKeys & (TotalKeys - 1)) != 0) {
		*Mapping = 0;
	} else {
		*Mapping = TotalKeys;
	}
	set_button_map();
	DS2_AwaitNoButtons();
}

static struct Entry GlobalMappings_A = {
	ENTRY_MANDATORY_MAPPING(&msg[MSG_BUTTON_MAPPING_A]),
	.Target = &gpsp_persistent_config.ButtonMappings[0]
};

static struct Entry GlobalMappings_B = {
	ENTRY_MANDATORY_MAPPING(&msg[MSG_BUTTON_MAPPING_B]),
	.Target = &gpsp_persistent_config.ButtonMappings[1]
};

static struct Entry GlobalMappings_Select = {
	ENTRY_MANDATORY_MAPPING(&msg[MSG_BUTTON_MAPPING_SELECT]),
	.Target = &gpsp_persistent_config.ButtonMappings[2]
};

static struct Entry GlobalMappings_Start = {
	ENTRY_MANDATORY_MAPPING(&msg[MSG_BUTTON_MAPPING_START]),
	.Target = &gpsp_persistent_config.ButtonMappings[3]
};

static struct Entry GlobalMappings_R = {
	ENTRY_MANDATORY_MAPPING(&msg[MSG_BUTTON_MAPPING_R]),
	.Target = &gpsp_persistent_config.ButtonMappings[4]
};

static struct Entry GlobalMappings_L = {
	ENTRY_MANDATORY_MAPPING(&msg[MSG_BUTTON_MAPPING_L]),
	.Target = &gpsp_persistent_config.ButtonMappings[5]
};

static struct Entry GlobalMappings_RapidA = {
	ENTRY_OPTIONAL_MAPPING(&msg[MSG_BUTTON_MAPPING_RAPID_A]),
	.Target = &gpsp_persistent_config.ButtonMappings[6]
};

static struct Entry GlobalMappings_RapidB = {
	ENTRY_OPTIONAL_MAPPING(&msg[MSG_BUTTON_MAPPING_RAPID_B]),
	.Target = &gpsp_persistent_config.ButtonMappings[7]
};

struct Menu GlobalMappings = {
	.Parent = &Tools, .Title = &msg[MSG_TOOLS_GLOBAL_BUTTON_MAPPING_GENERAL],
	.Entries = { &Back, &GlobalMappings_A, &GlobalMappings_B, &GlobalMappings_Select, &GlobalMappings_Start, &GlobalMappings_R, &GlobalMappings_L, &GlobalMappings_RapidA, &GlobalMappings_RapidB, NULL },
	.ActiveEntryIndex = 1  /* Start out after Back */
};

/* --- Main Menu > Tools > GAME-SPECIFIC MAPPINGS --- */

static struct Entry GameMappings_A = {
	ENTRY_OPTIONAL_MAPPING(&msg[MSG_BUTTON_MAPPING_A]),
	.Target = &game_persistent_config.ButtonMappings[0],
	.UserData = &gpsp_persistent_config.ButtonMappings[0]
};

static struct Entry GameMappings_B = {
	ENTRY_OPTIONAL_MAPPING(&msg[MSG_BUTTON_MAPPING_B]),
	.Target = &game_persistent_config.ButtonMappings[1],
	.UserData = &gpsp_persistent_config.ButtonMappings[1]
};

static struct Entry GameMappings_Select = {
	ENTRY_OPTIONAL_MAPPING(&msg[MSG_BUTTON_MAPPING_SELECT]),
	.Target = &game_persistent_config.ButtonMappings[2],
	.UserData = &gpsp_persistent_config.ButtonMappings[2]
};

static struct Entry GameMappings_Start = {
	ENTRY_OPTIONAL_MAPPING(&msg[MSG_BUTTON_MAPPING_START]),
	.Target = &game_persistent_config.ButtonMappings[3],
	.UserData = &gpsp_persistent_config.ButtonMappings[3]
};

static struct Entry GameMappings_R = {
	ENTRY_OPTIONAL_MAPPING(&msg[MSG_BUTTON_MAPPING_R]),
	.Target = &game_persistent_config.ButtonMappings[4],
	.UserData = &gpsp_persistent_config.ButtonMappings[4]
};

static struct Entry GameMappings_L = {
	ENTRY_OPTIONAL_MAPPING(&msg[MSG_BUTTON_MAPPING_L]),
	.Target = &game_persistent_config.ButtonMappings[5],
	.UserData = &gpsp_persistent_config.ButtonMappings[5]
};

static struct Entry GameMappings_RapidA = {
	ENTRY_OPTIONAL_MAPPING(&msg[MSG_BUTTON_MAPPING_RAPID_A]),
	.Target = &game_persistent_config.ButtonMappings[6],
	.UserData = &gpsp_persistent_config.ButtonMappings[6]
};

static struct Entry GameMappings_RapidB = {
	ENTRY_OPTIONAL_MAPPING(&msg[MSG_BUTTON_MAPPING_RAPID_B]),
	.Target = &game_persistent_config.ButtonMappings[7],
	.UserData = &gpsp_persistent_config.ButtonMappings[7]
};

struct Menu GameMappings = {
	.Parent = &Tools, .Title = &msg[MSG_TOOLS_GAME_BUTTON_MAPPING_GENERAL],
	.Entries = { &Back, &GameMappings_A, &GameMappings_B, &GameMappings_Select, &GameMappings_Start, &GameMappings_R, &GameMappings_L, &GameMappings_RapidA, &GameMappings_RapidB, NULL },
	.ActiveEntryIndex = 1  /* Start out after Back */
};

/* --- Main Menu > Tools > DEBUGGING --- */

extern struct Menu CodeSize;
extern struct Menu MetadataClears;
#ifdef PERFORMANCE_IMPACTING_STATISTICS
extern struct Menu CodeReuse;
#endif
extern struct Menu ExecutionStats;
extern struct Menu ROMInformation;

const char* TextDebugging = "Performance and debugging";
const char* TextCodeSize = "Native code size...";
const char* TextMetadataClears = "Metadata clear count...";
#ifdef PERFORMANCE_IMPACTING_STATISTICS
const char* TextCodeReuse = "Code reuse...";
#endif
const char* TextExecutionStats = "Execution statistics...";
const char* TextROMInformation = "ROM information...";

static struct Entry Debugging_CodeSize = {
	ENTRY_SUBMENU(&TextCodeSize, &CodeSize)
};

static struct Entry Debugging_MetadataClears = {
	ENTRY_SUBMENU(&TextMetadataClears, &MetadataClears)
};

static struct Entry Debugging_ExecutionStats = {
	ENTRY_SUBMENU(&TextExecutionStats, &ExecutionStats)
};

#ifdef PERFORMANCE_IMPACTING_STATISTICS
static struct Entry Debugging_CodeReuse = {
	ENTRY_SUBMENU(&TextCodeReuse, &CodeReuse)
};
#endif

static struct Entry Debugging_ROMInformation = {
	ENTRY_SUBMENU(&TextROMInformation, &ROMInformation)
};

struct Menu Debugging = {
	.Parent = &Tools, .Title = &TextDebugging,
	.Entries = { &Back, &Debugging_CodeSize, &Debugging_MetadataClears,
#ifdef PERFORMANCE_IMPACTING_STATISTICS
		&Debugging_CodeReuse,
#endif
		&Debugging_ExecutionStats, &Debugging_ROMInformation, NULL },
	.ActiveEntryIndex = 1  /* Start out after Back */
};

/* --- Main Menu > Tools > Debugging > CODE SIZE --- */

const char* CACHE_NAMES[TRANSLATION_REGION_COUNT] = {
	"Read-only", "Writable"
};
const char* FLUSH_REASON_NAMES[CACHE_FLUSH_REASON_COUNT] = {
	"Init", "ROM", "Link", "Full"
};

const char* METADATA_AREA_NAMES[METADATA_AREA_COUNT] = {
	"BIOS", "EWRAM", "IWRAM", "VRAM", "ROM"
};
const char* CLEAR_REASON_NAMES[METADATA_CLEAR_REASON_COUNT] = {
	"Init", "ROM", "Link", "Full", "Tag", "State"
};

static void CodeSizeDisplayData(struct Menu* ActiveMenu, struct Entry* ActiveEntry)
{
	size_t i;
	char Text[512];

	DefaultDisplayData(ActiveMenu, ActiveEntry);

	PRINT_STRING_BG(DS2_GetSubScreen(), "Current", COLOR_INACTIVE_ITEM, COLOR_TRANS, OPTION_TEXT_X + 1 * (DS_SCREEN_WIDTH - OPTION_TEXT_X * 2) / 4, GUI_ROW1_Y + TEXT_OFFSET_Y);
	PRINT_STRING_BG(DS2_GetSubScreen(), "Peak", COLOR_INACTIVE_ITEM, COLOR_TRANS, OPTION_TEXT_X + 2 * (DS_SCREEN_WIDTH - OPTION_TEXT_X * 2) / 4, GUI_ROW1_Y + TEXT_OFFSET_Y);
	PRINT_STRING_BG(DS2_GetSubScreen(), "Flushed", COLOR_INACTIVE_ITEM, COLOR_TRANS, OPTION_TEXT_X + 3 * (DS_SCREEN_WIDTH - OPTION_TEXT_X * 2) / 4, GUI_ROW1_Y + TEXT_OFFSET_Y);
	for (i = 0; i < TRANSLATION_REGION_COUNT; i++) {
		PRINT_STRING_BG(DS2_GetSubScreen(), CACHE_NAMES[i], COLOR_INACTIVE_ITEM, COLOR_TRANS, OPTION_TEXT_X, GUI_ROW1_Y + (i + 1) * GUI_ROW_SY + TEXT_OFFSET_Y);
		uint32_t Current = 0;
		switch (i) {
			case TRANSLATION_REGION_READONLY: Current = readonly_next_code - readonly_code_cache; break;
			case TRANSLATION_REGION_WRITABLE: Current = writable_next_code - writable_code_cache; break;
		}
		sprintf(Text, "%" PRIu32, Current);
		PRINT_STRING_BG(DS2_GetSubScreen(), Text, COLOR_INACTIVE_ITEM, COLOR_TRANS, OPTION_TEXT_X + 1 * (DS_SCREEN_WIDTH - OPTION_TEXT_X * 2) / 4, GUI_ROW1_Y + (i + 1) * GUI_ROW_SY + TEXT_OFFSET_Y);
		sprintf(Text, "%" PRIu64, Stats.TranslationBytesPeak[i]);
		PRINT_STRING_BG(DS2_GetSubScreen(), Text, COLOR_INACTIVE_ITEM, COLOR_TRANS, OPTION_TEXT_X + 2 * (DS_SCREEN_WIDTH - OPTION_TEXT_X * 2) / 4, GUI_ROW1_Y + (i + 1) * GUI_ROW_SY + TEXT_OFFSET_Y);
		sprintf(Text, "%" PRIu64, Stats.TranslationBytesFlushed[i]);
		PRINT_STRING_BG(DS2_GetSubScreen(), Text, COLOR_INACTIVE_ITEM, COLOR_TRANS, OPTION_TEXT_X + 3 * (DS_SCREEN_WIDTH - OPTION_TEXT_X * 2) / 4, GUI_ROW1_Y + (i + 1) * GUI_ROW_SY + TEXT_OFFSET_Y);
	}
}

struct Menu CodeSize = {
	.Parent = &Debugging, .Title = &TextCodeSize,
	.Entries = { &Back, NULL },
	.DisplayData = CodeSizeDisplayData
};

/* --- Main Menu > Tools > Debugging > METADATA CLEARS --- */

static void MetadataClearsDisplayData(struct Menu* ActiveMenu, struct Entry* ActiveEntry)
{
	uint32_t reason, area;
	char Text[512];

	DefaultDisplayData(ActiveMenu, ActiveEntry);

	for (reason = 0; reason < METADATA_CLEAR_REASON_COUNT; reason++)
		PRINT_STRING_BG(DS2_GetSubScreen(), CLEAR_REASON_NAMES[reason], COLOR_INACTIVE_ITEM, COLOR_TRANS, OPTION_TEXT_X + (reason + 1) * ((DS_SCREEN_WIDTH - OPTION_TEXT_X * 2) / (METADATA_CLEAR_REASON_COUNT + 1)), GUI_ROW1_Y + TEXT_OFFSET_Y);
	for (area = 0; area < METADATA_AREA_COUNT; area++) {
		uint32_t y = GUI_ROW1_Y + (area + 1) * GUI_ROW_SY + TEXT_OFFSET_Y;
		PRINT_STRING_BG(DS2_GetSubScreen(), METADATA_AREA_NAMES[area], COLOR_INACTIVE_ITEM, COLOR_TRANS, OPTION_TEXT_X, y);
		for (reason = 0; reason < METADATA_CLEAR_REASON_COUNT; reason++)
		{
			uint32_t x = OPTION_TEXT_X + (reason + 1) * ((DS_SCREEN_WIDTH - OPTION_TEXT_X * 2) / (METADATA_CLEAR_REASON_COUNT + 1));
			sprintf(Text, "%" PRIu64, Stats.MetadataClearCount[area][reason]);
			PRINT_STRING_BG(DS2_GetSubScreen(), Text, COLOR_INACTIVE_ITEM, COLOR_TRANS, x, y);
		}
	}
	PRINT_STRING_BG(DS2_GetSubScreen(), "Partial clears", COLOR_INACTIVE_ITEM, COLOR_TRANS, OPTION_TEXT_X, GUI_ROW1_Y + 7 * GUI_ROW_SY + TEXT_OFFSET_Y);
	sprintf(Text, "%" PRIu64, Stats.PartialFlushCount);
	PRINT_STRING_BG(DS2_GetSubScreen(), Text, COLOR_INACTIVE_ITEM, COLOR_TRANS, DS_SCREEN_WIDTH / 2, GUI_ROW1_Y + 7 * GUI_ROW_SY + TEXT_OFFSET_Y);
}

struct Menu MetadataClears = {
	.Parent = &Debugging, .Title = &TextMetadataClears,
	.Entries = { &Back, NULL },
	.DisplayData = MetadataClearsDisplayData
};

#ifdef PERFORMANCE_IMPACTING_STATISTICS

/* --- Main Menu > Tools > Debugging > CODE REUSE --- */

const char* TextBlocksRecompiled = "Blocks recompiled";
const char* TextOpcodesRecompiled = "Opcodes recompiled";
const char* TextBlocksReused = "Blocks reused";
const char* TextOpcodesReused = "Opcodes reused";

static struct Entry CodeReuse_BlocksRecompiled = {
	ENTRY_DISPLAY(&TextBlocksRecompiled, &Stats.BlockRecompilationCount, TYPE_UINT64)
};

static struct Entry CodeReuse_OpcodesRecompiled = {
	ENTRY_DISPLAY(&TextOpcodesRecompiled, &Stats.OpcodeRecompilationCount, TYPE_UINT64)
};

static struct Entry CodeReuse_BlocksReused = {
	ENTRY_DISPLAY(&TextBlocksReused, &Stats.BlockReuseCount, TYPE_UINT64)
};

static struct Entry CodeReuse_OpcodesReused = {
	ENTRY_DISPLAY(&TextOpcodesReused, &Stats.OpcodeReuseCount, TYPE_UINT64)
};

struct Menu CodeReuse = {
	.Parent = &Debugging, .Title = &TextCodeReuse,
	.Entries = { &Back, &CodeReuse_BlocksRecompiled, &CodeReuse_OpcodesRecompiled, &CodeReuse_BlocksReused, &CodeReuse_OpcodesReused, NULL }
};

#endif

/* --- Main Menu > Tools > Debugging > EXECUTION STATS --- */

const char* TextBufferUnderruns = "Sound buffer underruns";
const char* TextFramesEmulated = "Frames emulated";
const char* TextFramesRendered = "Frames rendered";
#ifdef PERFORMANCE_IMPACTING_STATISTICS
const char* TextARMOpcodes = "ARM opcodes decoded";
const char* TextThumbOpcodes = "Thumb opcodes decoded";
const char* TextThumbROMConstants = "Thumb ROM constants";
const char* TextWrongAddressLines = "Memory accessors patched";
#endif

static struct Entry ExecutionStats_BufferUnderruns = {
	ENTRY_DISPLAY(&TextBufferUnderruns, &Stats.SoundBufferUnderrunCount, TYPE_UINT64)
};

static struct Entry ExecutionStats_FramesEmulated = {
	ENTRY_DISPLAY(&TextFramesEmulated, &Stats.TotalEmulatedFrames, TYPE_UINT64)
};

static struct Entry ExecutionStats_FramesRendered = {
	ENTRY_DISPLAY(&TextFramesRendered, &Stats.TotalRenderedFrames, TYPE_UINT64)
};

#ifdef PERFORMANCE_IMPACTING_STATISTICS
static struct Entry ExecutionStats_ARMOpcodes = {
	ENTRY_DISPLAY(&TextARMOpcodes, &Stats.ARMOpcodesDecoded, TYPE_UINT64)
};

static struct Entry ExecutionStats_ThumbOpcodes = {
	ENTRY_DISPLAY(&TextThumbOpcodes, &Stats.ThumbOpcodesDecoded, TYPE_UINT64)
};

static struct Entry ExecutionStats_ThumbROMConstants = {
	ENTRY_DISPLAY(&TextThumbROMConstants, &Stats.ThumbROMConstants, TYPE_UINT64)
};

static struct Entry ExecutionStats_WrongAddressLines = {
	ENTRY_DISPLAY(&TextWrongAddressLines, &Stats.WrongAddressLineCount, TYPE_UINT32)
};
#endif

struct Menu ExecutionStats = {
	.Parent = &Debugging, .Title = &TextExecutionStats,
	.Entries = { &Back, &ExecutionStats_BufferUnderruns, &ExecutionStats_FramesEmulated, &ExecutionStats_FramesRendered,
#ifdef PERFORMANCE_IMPACTING_STATISTICS
		&ExecutionStats_ARMOpcodes, &ExecutionStats_ThumbOpcodes, &ExecutionStats_ThumbROMConstants, &ExecutionStats_WrongAddressLines,
#endif
		NULL }
};

/* --- Main Menu > Tools > Debugging > ROM INFORMATION --- */

const char* TextGameName = "game_name";
const char* TextGameCode = "game_code";
const char* TextVenderCode /* sic */ = "vender_code";

static struct Entry ROMInformation_GameName = {
	ENTRY_DISPLAY(&TextGameName, &gamepak_title, TYPE_STRING)
};

static struct Entry ROMInformation_GameCode = {
	ENTRY_DISPLAY(&TextGameCode, &gamepak_code, TYPE_STRING)
};

static struct Entry ROMInformation_VenderCode /* sic */ = {
	ENTRY_DISPLAY(&TextVenderCode, &gamepak_maker, TYPE_STRING)
};

struct Menu ROMInformation = {
	.Parent = &Debugging, .Title = &TextROMInformation,
	.Entries = { &Back, &ROMInformation_GameName, &ROMInformation_GameCode, &ROMInformation_VenderCode, NULL }
};

/* --- Main Menu > OPTIONS --- */

static void DisplayLanguageValue(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	char Temp[21];
	Temp[0] = '\0';
	const char* Value = Temp;
	bool Error = false;

	if (*(uint32_t*) DrawnEntry->Target < DrawnEntry->ChoiceCount)
		Value = lang[*(uint32_t*) DrawnEntry->Target];
	else {
		Value = "Out of bounds";
		Error = true;
	}

	bool IsActive = (DrawnEntry == ActiveEntry);
	uint16_t TextColor = Error ? BGR555(0, 0, 31) : (IsActive ? COLOR_ACTIVE_ITEM : COLOR_INACTIVE_ITEM);
	uint32_t TextWidth = BDF_WidthUTF8s(Value);
	PRINT_STRING_BG(DS2_GetSubScreen(), Value, TextColor, COLOR_TRANS, DS_SCREEN_WIDTH - OPTION_TEXT_X - TextWidth, GUI_ROW1_Y + (Position - 1) * GUI_ROW_SY + TEXT_OFFSET_Y);
}

void PostChangeLanguage()
{
	HighFrequencyCPU(); // crank it up

	load_language_msg(LANGUAGE_PACK, gpsp_persistent_config.language);

	if (!IsGameLoaded) {
		DS2_FillScreen(DS_ENGINE_MAIN, 0);
		draw_string_vcenter(DS2_GetMainScreen(), 0, 88, 256, COLOR_WHITE, msg[MSG_TOP_SCREEN_NO_GAME_LOADED]);
		DS2_FlipMainScreen();
	}

	LowFrequencyCPU(); // and back down
}

static void DisplayFrequencyValue(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	char Value[15];

	sprintf(Value, "%" PRIu32 " MHz", *(uint32_t*) DrawnEntry->Target / 1000000);

	bool IsActive = (DrawnEntry == ActiveEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_ITEM : COLOR_INACTIVE_ITEM;
	uint32_t TextWidth = BDF_WidthUTF8s(Value);
	PRINT_STRING_BG(DS2_GetSubScreen(), Value, TextColor, COLOR_TRANS, DS_SCREEN_WIDTH - OPTION_TEXT_X - TextWidth, GUI_ROW1_Y + (Position - 1) * GUI_ROW_SY + TEXT_OFFSET_Y);
}

static void LeftCPUFrequency(struct Menu* ActiveMenu, struct Entry* ActiveEntry)
{
	uint32_t* Target = (uint32_t*) ActiveEntry->Target;
	if (*Target <= UINT32_C(252000000))
		*Target = UINT32_C(240000000);
	else
		*Target = (*Target - 1) / UINT32_C(12000000) * UINT32_C(12000000);

	ActionFunction Action = ActiveEntry->Action;
	if (Action != NULL)
		Action();
}

static void RightCPUFrequency(struct Menu* ActiveMenu, struct Entry* ActiveEntry)
{
	uint32_t* Target = (uint32_t*) ActiveEntry->Target;
	if (*Target >= UINT32_C(468000000))
		*Target = UINT32_C(480000000);
	else
		*Target = (*Target + UINT32_C(12000000)) / UINT32_C(12000000) * UINT32_C(12000000);

	ActionFunction Action = ActiveEntry->Action;
	if (Action != NULL)
		Action();
}

static void LeftRAMFrequency(struct Menu* ActiveMenu, struct Entry* ActiveEntry)
{
	uint32_t* Target = (uint32_t*) ActiveEntry->Target;
	if (*Target <= UINT32_C(64000000))
		*Target = UINT32_C(60000000);
	else
		*Target = (*Target - 1) / UINT32_C(4000000) * UINT32_C(4000000);

	ActionFunction Action = ActiveEntry->Action;
	if (Action != NULL)
		Action();
}

static void RightRAMFrequency(struct Menu* ActiveMenu, struct Entry* ActiveEntry)
{
	uint32_t* Target = (uint32_t*) ActiveEntry->Target;
	if (*Target >= UINT32_C(236000000))
		*Target = UINT32_C(240000000);
	else
		*Target = (*Target + UINT32_C(4000000)) / UINT32_C(4000000) * UINT32_C(4000000);

	ActionFunction Action = ActiveEntry->Action;
	if (Action != NULL)
		Action();
}

void LoadDefaults()
{
	draw_message_box(DS2_GetSubScreen());
	draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_DIALOG_RESET]);

	if (draw_yesno_dialog(DS_ENGINE_SUB, msg[MSG_GENERAL_CONFIRM_WITH_A], msg[MSG_GENERAL_CANCEL_WITH_B])) {
		char file[PATH_MAX];

		DS2_AwaitNoButtons();
		draw_message_box(DS2_GetSubScreen());
		draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_RESETTING]);
		DS2_UpdateScreen(DS_ENGINE_SUB);

		sprintf(file, "%s/%s", main_path, GPSP_CONFIG_FILENAME);
		remove(file);

		init_default_gpsp_config();
		PostChangeLanguage();
		init_game_config();
	} else {
		DS2_AwaitNoButtons();
	}
}

void ShowVersion()
{
	char line_buffer[512];

	draw_message_box(DS2_GetSubScreen());
#ifdef GIT_VERSION
#define STRINGIFY(x) XSTRINGIFY(x)
#define XSTRINGIFY(x) #x
	sprintf(line_buffer, "%s\n%s %s\nNebuleon/ReGBA commit %s", msg[MSG_EMULATOR_NAME], msg[MSG_WORD_EMULATOR_VERSION], NDSGBA_VERSION, STRINGIFY(GIT_VERSION));
#else
	sprintf(line_buffer, "%s\n%s %s", msg[MSG_EMULATOR_NAME], msg[MSG_WORD_EMULATOR_VERSION], NDSGBA_VERSION);
#endif
	draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, line_buffer);
	DS2_UpdateScreen(DS_ENGINE_SUB);

	DS2_AwaitNoButtons(); // invoked from the menu
	DS2_AwaitAnyButtons(); // wait until the user presses something
	DS2_AwaitNoButtons(); // don't give that button to the menu
}

static struct Entry Options_Language = {
	ENTRY_OPTION(&msg[MSG_OPTIONS_LANGUAGE], &gpsp_persistent_config.language, LANG_END),
	.DisplayValue = DisplayLanguageValue,
	.Action = PostChangeLanguage
};

static struct Entry Options_CPUFrequency = {
	ENTRY_OPTION(&msg[MSG_OPTIONS_CPU_FREQUENCY], &gpsp_persistent_config.cpu_hz, 0),
	.DisplayValue = DisplayFrequencyValue,
	.Left = LeftCPUFrequency, .Right = RightCPUFrequency
};

static struct Entry Options_RAMFrequency = {
	ENTRY_OPTION(&msg[MSG_OPTIONS_RAM_FREQUENCY], &gpsp_persistent_config.mem_hz, 0),
	.DisplayValue = DisplayFrequencyValue,
	.Left = LeftRAMFrequency, .Right = RightRAMFrequency
};

static struct Entry Options_Reset = {
	.Kind = KIND_CUSTOM, .Name = &msg[MSG_OPTIONS_RESET],
	.Enter = LoadDefaults, .Touch = TouchEnter
};

static struct Entry Options_Version = {
	.Kind = KIND_CUSTOM, .Name = &msg[MSG_OPTIONS_VERSION],
	.Enter = ShowVersion, .Touch = TouchEnter
};

struct Menu Options = {
	.Parent = &MainMenu, .Title = &msg[MSG_MAIN_MENU_OPTIONS],
	.Entries = { &Back, &Options_Language, &Options_CPUFrequency, &Options_RAMFrequency, &Options_Reset, &Options_Version, NULL },
	.ActiveEntryIndex = 1  /* Start out after Back */
};

/* --- Main Menu > NEW GAME --- */

extern struct Menu RecentGames;

static void ActionNewGame(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	const char *file_ext[] = { ".gba", ".bin", ".zip", NULL };
	char tmp_filename[PATH_MAX];
	char file_path[PATH_MAX];

	if (load_file(file_ext, tmp_filename, g_default_rom_dir) == 0) {
		sprintf(file_path, "%s/%s", g_default_rom_dir, tmp_filename);

		if (LoadGameAndItsData(file_path))
			*ActiveMenu = NULL;
	}
}

static struct Entry NewGame_FromCard = {
	.Kind = KIND_CUSTOM, .Name = &msg[MSG_LOAD_GAME_FROM_CARD],
	.Enter = ActionNewGame, .Touch = TouchEnter
};

static struct Entry NewGame_Recent = {
	ENTRY_SUBMENU(&msg[MSG_LOAD_GAME_RECENTLY_PLAYED], &RecentGames)
};

struct Menu NewGame = {
	.Parent = &MainMenu, .Title = &msg[MSG_LOAD_GAME_MENU_TITLE],
	.Entries = { &Back, &NewGame_FromCard, &NewGame_Recent, NULL },
	.ActiveEntryIndex = 1  /* Start out after Back */
};

/* --- Main Menu > New Game > RECENTLY PLAYED --- */

static int32_t RecentGamesScroll;

extern struct Entry RecentGame[5];

static void BuildRecentGame(struct Menu* ActiveMenu, uint32_t EntryIndex)
{
	char* file = strrchr(gpsp_persistent_config.latest_file[EntryIndex - 1], '/');

	if (ActiveMenu->Entries[EntryIndex]->UserData != NULL) {
		draw_hscroll_over(ActiveMenu->Entries[EntryIndex]->UserData);
		ActiveMenu->Entries[EntryIndex]->UserData = NULL;
	}

	ActiveMenu->Entries[EntryIndex]->UserData = hscroll_init(DS2_GetSubScreen(),
		OPTION_TEXT_X,
		GUI_ROW1_Y + (EntryIndex - 1) * GUI_ROW_SY + TEXT_OFFSET_Y,
		OPTION_TEXT_SX,
		COLOR_TRANS,
		EntryIndex == ActiveMenu->ActiveEntryIndex ? COLOR_ACTIVE_ITEM : COLOR_INACTIVE_ITEM,
		file + 1);
}

static void InitRecentGames(struct Menu** ActiveMenu)
{
	uint32_t i;
	char* file;

	for (i = 1; i < sizeof(RecentGame) / sizeof(RecentGame[0]); i++)
		memcpy(&RecentGame[i], &RecentGame[0], sizeof(RecentGame[0]));

	/* Figure out how many entries in the file contain full paths. */
	for (i = 0; i < sizeof(RecentGame) / sizeof(RecentGame[0]); i++) {
		file = strrchr(gpsp_persistent_config.latest_file[i], '/');
		if (file == NULL)
			break;
	}

	/* Activate the first entry if there are files; Back otherwise. */
	(*ActiveMenu)->ActiveEntryIndex = (i > 0) ? 1 : 0;
	/* Store the upper bound on the menu's entries, exclusive, in UserData. */
	(*ActiveMenu)->UserData = (void*) (i + 1);

	for (i = 1; i < (uint32_t) (*ActiveMenu)->UserData; i++) {
		BuildRecentGame(*ActiveMenu, i);
	}

	RecentGamesScroll = 0;
}

static void EndRecentGames(struct Menu* ActiveMenu)
{
	uint32_t i, max = (uint32_t) ActiveMenu->UserData;

	for (i = 1; i < max; i++) {
		draw_hscroll_over(ActiveMenu->Entries[i]->UserData);
		ActiveMenu->Entries[i]->UserData = NULL;
	}

	ActiveMenu->UserData = NULL;
}

static void UpRecentGames(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	uint32_t PreviousEntryIndex = *ActiveEntryIndex;

	DefaultUp(ActiveMenu, ActiveEntryIndex);
	if (PreviousEntryIndex != 0) {
		BuildRecentGame(*ActiveMenu, PreviousEntryIndex);
	}
	if (*ActiveEntryIndex != 0) {
		BuildRecentGame(*ActiveMenu, *ActiveEntryIndex);
	}
}

static void DownRecentGames(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	uint32_t PreviousEntryIndex = *ActiveEntryIndex;

	DefaultDown(ActiveMenu, ActiveEntryIndex);
	if (PreviousEntryIndex != 0) {
		BuildRecentGame(*ActiveMenu, PreviousEntryIndex);
	}
	if (*ActiveEntryIndex != 0) {
		BuildRecentGame(*ActiveMenu, *ActiveEntryIndex);
	}
}

static bool CanFocusRecentGame(struct Menu* ActiveMenu, struct Entry* ActiveEntry, uint32_t Position)
{
	return Position < (uint32_t) ActiveMenu->UserData;
}

static void LeftRecentGame(struct Menu* ActiveMenu, struct Entry* ActiveEntry)
{
	RecentGamesScroll = 5;
}

static void RightRecentGame(struct Menu* ActiveMenu, struct Entry* ActiveEntry)
{
	RecentGamesScroll = -5;
}

static void DisplayNameRecentGame(struct Entry* DrawnEntry, struct Entry* ActiveEntry, uint32_t Position)
{
	bool IsActive = (DrawnEntry == ActiveEntry);
	if (IsActive) {
		show_icon(DS2_GetSubScreen(), &ICON_SUBSELA, SUBSELA_X, GUI_ROW1_Y + (Position - 1) * GUI_ROW_SY + SUBSELA_OFFSET_Y);
		draw_hscroll(DrawnEntry->UserData, RecentGamesScroll);
		RecentGamesScroll = 0;
	} else {
		draw_hscroll(DrawnEntry->UserData, 0);
	}
}

static void LoadRecentGame(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex)
{
	/* Copy the file name into this, which is guaranteed not to overlap with
	 * gpsp_persistent_config.latest_file. Otherwise, recent file reordering
	 * will fail. */
	char path[PATH_MAX];
	strcpy(path, gpsp_persistent_config.latest_file[*ActiveEntryIndex - 1]);
	char* slash = strrchr(path, '/');

	// Change g_default_rom_dir to the path contained in the entry.
	*slash = '\0';
	strcpy(g_default_rom_dir, path);
	*slash = '/';

	if (LoadGameAndItsData(path)) {
		if (latest_save >= 0) {
			load_state(latest_save);
		}
		*ActiveMenu = NULL;
	}
}

static void TouchRecentGame(struct Menu** ActiveMenu, uint32_t* ActiveEntryIndex, uint32_t X, uint32_t Y)
{
	LoadRecentGame(ActiveMenu, ActiveEntryIndex);
	if (*ActiveMenu != NULL) {
		uint32_t i;

		/* Loading the game has failed. Redo all the entries with the current
		 * active state of whatever was touched. */
		for (i = 1; i < (uint32_t) (*ActiveMenu)->UserData; i++) {
			BuildRecentGame(*ActiveMenu, i);
		}
	}
}

struct Entry RecentGame[5] = {
	/* These need to be distinct to store UserData for each of them, but only
	 * the first entry is written out. It's copied in InitRecentGames. */
	{
		.Kind = KIND_CUSTOM,
		.DisplayName = DisplayNameRecentGame, .CanFocus = CanFocusRecentGame,
		.Left = LeftRecentGame, .Right = RightRecentGame,
		.Enter = LoadRecentGame, .Touch = TouchRecentGame
	}
};

struct Menu RecentGames = {
	.Parent = &NewGame, .Title = &msg[MSG_LOAD_GAME_RECENTLY_PLAYED],
	.Entries = { &Back, &RecentGame[0], &RecentGame[1], &RecentGame[2], &RecentGame[3], &RecentGame[4], NULL },
	.Init = InitRecentGames, .End = EndRecentGames,
	.Up = UpRecentGames, .Down = DownRecentGames
};

void PreserveConfigs(GAME_CONFIG_FILE* GameConfig, GPSP_CONFIG_FILE* GpspConfig)
{
	memcpy(GameConfig, &game_persistent_config, sizeof(GAME_CONFIG_FILE));
	memcpy(GpspConfig, &gpsp_persistent_config, sizeof(GPSP_CONFIG_FILE));
}

void SaveConfigsIfNeeded(GAME_CONFIG_FILE* GameConfig, GPSP_CONFIG_FILE* GpspConfig)
{
	if (memcmp(GameConfig, &game_persistent_config, sizeof(GAME_CONFIG_FILE)) != 0) {
		save_game_config_file();
		memcpy(GameConfig, &game_persistent_config, sizeof(GAME_CONFIG_FILE));
	}
	if (memcmp(GpspConfig, &gpsp_persistent_config, sizeof(GPSP_CONFIG_FILE)) != 0) {
		save_config_file();
		memcpy(GpspConfig, &gpsp_persistent_config, sizeof(GPSP_CONFIG_FILE));
	}
}

uint32_t ReGBA_Menu(enum ReGBA_MenuEntryReason EntryReason)
{
	GAME_CONFIG_FILE PreviousGameConfig; // Compared with current settings to
	GPSP_CONFIG_FILE PreviousGpspConfig; // determine if they need to be saved
	struct Menu *ActiveMenu = &MainMenu, *PreviousMenu = ActiveMenu;

	LowFrequencyCPU();
	// Avoid entering the menu with menu keys pressed.
	DS2_AwaitNoButtons();

	PreserveConfigs(&PreviousGameConfig, &PreviousGpspConfig);

	if (EntryReason == REGBA_MENU_ENTRY_REASON_NO_ROM) {
		// Try automatically loading a game passed through argv.
		if (strlen(argv[1]) > 0 && LoadGameAndItsData(argv[1]))
			goto exit;
		else {
			DS2_FillScreen(DS_ENGINE_MAIN, COLOR_BLACK);
			draw_string_vcenter(DS2_GetMainScreen(), 0, 88, 256, COLOR_WHITE, msg[MSG_TOP_SCREEN_NO_GAME_LOADED]);
			DS2_FlipMainScreen();
		}
	} else {
		DS2_SetScreenBacklights(DS_SCREEN_BOTH);
		DS2_SetScreenSwap(true);
	}

	/* Save the GBA screen to MainMenu.UserData. */
	MainMenu.UserData = malloc(GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT * sizeof(uint16_t));
	if (!MainMenu.UserData) {
		goto exit;
	}
	copy_screen(MainMenu.UserData);

	if (MainMenu.Init != NULL) {
		MainMenu.Init(&ActiveMenu);
		while (PreviousMenu != ActiveMenu) {
			if (PreviousMenu != NULL && PreviousMenu->End != NULL)
				PreviousMenu->End(PreviousMenu);
			PreviousMenu = ActiveMenu;
			if (ActiveMenu != NULL && ActiveMenu->Init != NULL)
				ActiveMenu->Init(&ActiveMenu);
		}
	}

	while (ActiveMenu != NULL) {
		// Draw.
		MenuFunction DisplayBackground = ActiveMenu->DisplayBackground;
		if (DisplayBackground == NULL) DisplayBackground = DefaultDisplayBackground;
		DisplayBackground(ActiveMenu);

		MenuFunction DisplayTitle = ActiveMenu->DisplayTitle;
		if (DisplayTitle == NULL) DisplayTitle = DefaultDisplayTitle;
		DisplayTitle(ActiveMenu);

		EntryFunction DisplayData = ActiveMenu->DisplayData;
		if (DisplayData == NULL) DisplayData = DefaultDisplayData;
		DisplayData(ActiveMenu, ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]);

		DS2_UpdateScreen(DS_ENGINE_SUB);
		DS2_AwaitScreenUpdate(DS_ENGINE_SUB);

		// Get input.
		ModifyFunction InputDispatch = ActiveMenu->InputDispatch;
		if (InputDispatch == NULL) InputDispatch = DefaultInputDispatch;
		InputDispatch(&ActiveMenu, &ActiveMenu->ActiveEntryIndex);

		// Possibly finalise this menu and activate and initialise a new one.
		while (ActiveMenu != PreviousMenu) {
			if (PreviousMenu != NULL && PreviousMenu->End != NULL)
				PreviousMenu->End(PreviousMenu);

			SaveConfigsIfNeeded(&PreviousGameConfig, &PreviousGpspConfig);

			// Keep moving down until a menu entry can be focused, if
			// the first one can't be.
			if (ActiveMenu != NULL && ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex] != NULL) {
				uint32_t Sentinel = ActiveMenu->ActiveEntryIndex;
				EntryCanFocusFunction CanFocus = ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]->CanFocus;
				if (CanFocus == NULL) CanFocus = DefaultCanFocus;
				while (!CanFocus(ActiveMenu, ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex], ActiveMenu->ActiveEntryIndex)) {
					MoveDown(ActiveMenu, &ActiveMenu->ActiveEntryIndex);
					if (ActiveMenu->ActiveEntryIndex == Sentinel) {
						// If we went through all of them, we cannot focus anything.
						// Place the focus on the NULL entry.
						ActiveMenu->ActiveEntryIndex = FindNullEntry(ActiveMenu);
						break;
					}
					CanFocus = ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]->CanFocus;
					if (CanFocus == NULL) CanFocus = DefaultCanFocus;
				}
			}

			PreviousMenu = ActiveMenu;
			if (ActiveMenu != NULL && ActiveMenu->Init != NULL)
				ActiveMenu->Init(&ActiveMenu);
		}
	}

	free(MainMenu.UserData);
	MainMenu.UserData = NULL;

exit:
	SaveConfigsIfNeeded(&PreviousGameConfig, &PreviousGpspConfig);

	// Avoid leaving the menu with GBA keys pressed (namely the one bound to
	// the native exit button, B).
	DS2_AwaitNoButtons();

	// Clear the Sub Screen as its backlight is about to be turned off.
	DS2_FillScreen(DS_ENGINE_SUB, COLOR_BLACK);
	DS2_UpdateScreen(DS_ENGINE_SUB);

	if (gpsp_persistent_config.BottomScreenGame) {
		DS2_SetScreenSwap(false);
		DS2_SetScreenBacklights(DS_SCREEN_LOWER);
	} else {
		DS2_SetScreenSwap(true);
		DS2_SetScreenBacklights(DS_SCREEN_UPPER);
	}

	GameFrequencyCPU();

	Stats.LastFPSCalculationTime = clock();
	StatsStopFPS(); // FPS will be 0 if we return, so stop it showing
	UpdateBorder();
	return 0;
}

/*--------------------------------------------------------
	Load language message
--------------------------------------------------------*/
int load_language_msg(const char *filename, uint32_t language)
{
	FILE *fp;
	char msg_path[PATH_MAX];
	char string[256];
	const char* start;
	const char* end;
	char *pt, *dst;
	uint32_t loop = 0, len;
	int ret;

	sprintf(msg_path, "%s/%s", main_path, filename);
	fp = fopen(msg_path, "rb");
	if (fp == NULL)
		return -1;

	switch (language) {
	case ENGLISH:
	default:
		start = "STARTENGLISH";
		end = "ENDENGLISH";
		break;
	case CHINESE_SIMPLIFIED:
		start = "STARTCHINESESIM";
		end = "ENDCHINESESIM";
		break;
	case FRENCH:
		start = "STARTFRENCH";
		end = "ENDFRENCH";
		break;
	case GERMAN:
		start = "STARTGERMAN";
		end = "ENDGERMAN";
		break;
	case DUTCH:
		start = "STARTDUTCH";
		end = "ENDDUTCH";
		break;
	case SPANISH:
		start = "STARTSPANISH";
		end = "ENDSPANISH";
		break;
	case ITALIAN:
		start = "STARTITALIAN";
		end = "ENDITALIAN";
		break;
	case PORTUGUESE_BRAZILIAN:
		start = "STARTPORTUGUESEBR";
		end = "ENDPORTUGUESEBR";
		break;
	case CHINESE_TRADITIONAL:
		start = "STARTCHINESETRA";
		end = "ENDCHINESETRA";
		break;
	}
	size_t start_len = strlen(start), end_len = strlen(end);

	// find the start flag
	ret = 0;
	do {
		pt = fgets(string, sizeof(string), fp);
		if (pt == NULL) {
			ret = -2;
			goto load_language_msg_error;
		}
	} while (strncmp(pt, start, start_len) != 0);

	dst = msg_data;
	msg[0] = dst;

	while (loop != MSG_END) {
		while (1) {
			pt = fgets(string, sizeof(string), fp);
			if (pt != NULL) {
				if (pt[0] == '#' || pt[0] == '\r' || pt[0] == '\n')
					continue;
				else
					break;
			} else {
				ret = -3;
				goto load_language_msg_error;
			}
		}

		if (strncmp(pt, end, end_len) == 0)
			break;

		len = strlen(pt);

		// Replace key definitions (*letter) with Pictochat icons
		// while copying.
		size_t srcChar, dstLen = 0;
		for (srcChar = 0; srcChar < len; srcChar++)
		{
			if (pt[srcChar] == '*') {
				switch (pt[srcChar + 1]) {
				case 'A':
					memcpy(&dst[dstLen], HOTKEY_A_DISPLAY, sizeof(HOTKEY_A_DISPLAY) - 1);
					srcChar++;
					dstLen += sizeof(HOTKEY_A_DISPLAY) - 1;
					break;
				case 'B':
					memcpy(&dst[dstLen], HOTKEY_B_DISPLAY, sizeof(HOTKEY_B_DISPLAY) - 1);
					srcChar++;
					dstLen += sizeof(HOTKEY_B_DISPLAY) - 1;
					break;
				case 'X':
					memcpy(&dst[dstLen], HOTKEY_X_DISPLAY, sizeof(HOTKEY_X_DISPLAY) - 1);
					srcChar++;
					dstLen += sizeof(HOTKEY_X_DISPLAY) - 1;
					break;
				case 'Y':
					memcpy(&dst[dstLen], HOTKEY_Y_DISPLAY, sizeof(HOTKEY_Y_DISPLAY) - 1);
					srcChar++;
					dstLen += sizeof(HOTKEY_Y_DISPLAY) - 1;
					break;
				case 'L':
					memcpy(&dst[dstLen], HOTKEY_L_DISPLAY, sizeof(HOTKEY_L_DISPLAY) - 1);
					srcChar++;
					dstLen += sizeof(HOTKEY_L_DISPLAY) - 1;
					break;
				case 'R':
					memcpy(&dst[dstLen], HOTKEY_R_DISPLAY, sizeof(HOTKEY_R_DISPLAY) - 1);
					srcChar++;
					dstLen += sizeof(HOTKEY_R_DISPLAY) - 1;
					break;
				case 'S':
					memcpy(&dst[dstLen], HOTKEY_START_DISPLAY, sizeof(HOTKEY_START_DISPLAY) - 1);
					srcChar++;
					dstLen += sizeof(HOTKEY_START_DISPLAY) - 1;
					break;
				case 's':
					memcpy(&dst[dstLen], HOTKEY_SELECT_DISPLAY, sizeof(HOTKEY_SELECT_DISPLAY) - 1);
					srcChar++;
					dstLen += sizeof(HOTKEY_SELECT_DISPLAY) - 1;
					break;
				case 'u':
					memcpy(&dst[dstLen], HOTKEY_UP_DISPLAY, sizeof(HOTKEY_UP_DISPLAY) - 1);
					srcChar++;
					dstLen += sizeof(HOTKEY_UP_DISPLAY) - 1;
					break;
				case 'd':
					memcpy(&dst[dstLen], HOTKEY_DOWN_DISPLAY, sizeof(HOTKEY_DOWN_DISPLAY) - 1);
					srcChar++;
					dstLen += sizeof(HOTKEY_DOWN_DISPLAY) - 1;
					break;
				case 'l':
					memcpy(&dst[dstLen], HOTKEY_LEFT_DISPLAY, sizeof(HOTKEY_LEFT_DISPLAY) - 1);
					srcChar++;
					dstLen += sizeof(HOTKEY_LEFT_DISPLAY) - 1;
					break;
				case 'r':
					memcpy(&dst[dstLen], HOTKEY_RIGHT_DISPLAY, sizeof(HOTKEY_RIGHT_DISPLAY) - 1);
					srcChar++;
					dstLen += sizeof(HOTKEY_RIGHT_DISPLAY) - 1;
					break;
				case '\0':
					dst[dstLen] = pt[srcChar];
					dstLen++;
					break;
				default:
					memcpy(&dst[dstLen], &pt[srcChar], 2);
					srcChar++;
					dstLen += 2;
					break;
				}
			} else {
				dst[dstLen] = pt[srcChar];
				dstLen++;
			}
		}

		dst += dstLen;
		// at a line return, when "\n" paded, this message not end
		if (*(dst - 1) == 0x0A) {
			pt = strrchr(pt, '\\');
			if ((pt != NULL) && (*(pt + 1) == 'n')) {
				if (*(dst - 2) == 0x0D) {
					*(dst - 4) = '\n';
					dst -= 3;
				} else {
					*(dst - 3) = '\n';
					dst -= 2;
				}
			} else /* the message ends */ {
				if (*(dst - 2) == 0x0D)
					dst -= 1;
				*(dst - 1) = '\0';
				msg[++loop] = dst;
			}
		}
	}

load_language_msg_error:
	fclose(fp);
	return ret;
}

/*--------------------------------------------------------
  加载字体库
--------------------------------------------------------*/
int load_font()
{
    return BDF_font_init();
}

/*--------------------------------------------------------
  游戏的设置文件
--------------------------------------------------------*/
int32_t save_game_config_file()
{
	char game_config_filename[PATH_MAX];
	FILE_TAG_TYPE game_config_file;
	char FileNameNoExt[PATH_MAX];

	if (!IsGameLoaded) return -1;

	GetFileNameNoExtension(FileNameNoExt, CurrentGamePath);

	sprintf(game_config_filename, "%s/%s_0.rts", DEFAULT_CFG_DIR, FileNameNoExt);

	FILE_OPEN(game_config_file, game_config_filename, WRITE);
	if (FILE_CHECK_VALID(game_config_file)) {
		FILE_WRITE(game_config_file, GAME_CONFIG_HEADER, GAME_CONFIG_HEADER_SIZE);
		FILE_WRITE_VARIABLE(game_config_file, game_persistent_config);
		FILE_CLOSE(game_config_file);
		return 0;
	}

	return -1;
}

/*--------------------------------------------------------
  dsGBA 的配置文件
--------------------------------------------------------*/
int32_t save_config_file()
{
	char gpsp_config_path[PATH_MAX];
	FILE_TAG_TYPE gpsp_config_file;

	sprintf(gpsp_config_path, "%s/%s", main_path, GPSP_CONFIG_FILENAME);

	FILE_OPEN(gpsp_config_file, gpsp_config_path, WRITE);
	if (FILE_CHECK_VALID(gpsp_config_file)) {
		FILE_WRITE(gpsp_config_file, GPSP_CONFIG_HEADER, GPSP_CONFIG_HEADER_SIZE);
		FILE_WRITE_VARIABLE(gpsp_config_file, gpsp_persistent_config);
		FILE_CLOSE(gpsp_config_file);
		return 0;
	}

	return -1;
}

/******************************************************************************
 * local function definition
 ******************************************************************************/
void reorder_latest_file(const char* GamePath)
{
	int32_t i, FoundIndex = -1;

	// Is the file's name already here?
	for (i = 0; i < 5; i++) {
		char* RecentFileName = gpsp_persistent_config.latest_file[i];
		if (RecentFileName && *RecentFileName) {
			if (strcmp(RecentFileName, GamePath) == 0) {
				FoundIndex = i; // Yes.
				break;
			}
		}
	}

	if (FoundIndex > -1) {
		// Already here, move all of those until the existing one 1 down
		memmove(gpsp_persistent_config.latest_file[1],
			gpsp_persistent_config.latest_file[0],
			FoundIndex * sizeof(gpsp_persistent_config.latest_file[0]));
	} else {
		// Not here, move everything down
		memmove(gpsp_persistent_config.latest_file[1],
			gpsp_persistent_config.latest_file[0],
			4 * sizeof(gpsp_persistent_config.latest_file[0]));
	}

	//Removing rom_path due to confusion, replacing with g_default_rom_dir
	strcpy(gpsp_persistent_config.latest_file[0], GamePath);
}


static int rtc_time_cmp(const struct ReGBA_RTC* t1, const struct ReGBA_RTC* t2)
{
	int result;
	if ((result = (int) t1->year - (int) t2->year) != 0)
		return result;
	if ((result = (int) t1->month - (int) t2->month) != 0)
		return result;
	if ((result = (int) t1->day - (int) t2->day) != 0)
		return result;
	if ((result = (int) t1->hours - (int) t2->hours) != 0)
		return result;
	if ((result = (int) t1->minutes - (int) t2->minutes) != 0)
		return result;
	if ((result = (int) t1->seconds - (int) t2->seconds) != 0)
		return result;
	return 0;
}

int load_game_stat_snapshot(const char* file)
{
	FILE* fp;
	uint16_t* screen;
	uint32_t y;

	fp = fopen(file, "r");
	if (NULL == fp)
		return -1;

	screen = malloc(GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT * sizeof(uint16_t));
	if (screen == NULL)
		return -2;

	fseek(fp, SVS_HEADER_SIZE, SEEK_SET);

	struct ReGBA_RTC save_time;
	char date_str[20];

	fread(&save_time, 1, sizeof(save_time), fp);
    sprintf(date_str, "%04d-%02d-%02d %02d:%02d:%02d",
		save_time.year + 2000, save_time.month, save_time.day, save_time.hours, save_time.minutes, save_time.seconds);

	DS2_FillScreen(DS_ENGINE_MAIN, COLOR_BLACK);
	PRINT_STRING_BG(DS2_GetMainScreen(), date_str, COLOR_WHITE, COLOR_TRANS, 1, 1);

	if (fread(screen, GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT * sizeof(uint16_t), 1, fp) == 0) {
		free(screen);
		fclose(fp);
		return -4;
	}

	for (y = 0; y < GBA_SCREEN_HEIGHT; y++) {
		memcpy(DS2_GetMainScreen()
			+ (y + (DS_SCREEN_HEIGHT - GBA_SCREEN_HEIGHT) / 2) * DS_SCREEN_WIDTH
			+ (DS_SCREEN_WIDTH - GBA_SCREEN_WIDTH) / 2,
			screen + y * GBA_SCREEN_WIDTH,
			GBA_SCREEN_WIDTH * sizeof(uint16_t));
	}

	free(screen);
	fclose(fp);
	DS2_FlipMainScreen();
	return 0;
}

static void get_savestate_filelist(const char* GamePath)
{
	int i;
	char savestate_path[PATH_MAX];
	char FileNameNoExt[PATH_MAX];
	char *pt;
	FILE *fp;
	size_t read;
	uint8_t header[SVS_HEADER_SIZE];
	// Which is the latest?
	/* global */ latest_save = -1;
	struct ReGBA_RTC latest_save_time, current_time;
	memset(&latest_save_time, 0, sizeof(struct ReGBA_RTC));

	GetFileNameNoExtension(FileNameNoExt, GamePath);
	sprintf(savestate_path, "%s/%s", DEFAULT_SAVE_DIR, FileNameNoExt);
	pt = savestate_path + strlen(savestate_path);
	for (i = 0; i < SAVE_STATE_SLOT_NUM; i++)
	{
		sprintf(pt, "_%d.rts", i+1);

		fp = fopen(savestate_path, "r");
		if (fp != NULL) {
			SavedStateExistenceCache[i] = true;
			read = fread(header, 1, SVS_HEADER_SIZE, fp);
			if (read < SVS_HEADER_SIZE || !(
			    memcmp(header, SVS_HEADER_E, SVS_HEADER_SIZE) == 0
			 || memcmp(header, SVS_HEADER_F, SVS_HEADER_SIZE) == 0
			)) {
				fclose(fp);
				continue;
			}

			/* Read back the time stamp */
			fread(&current_time, 1, sizeof(struct ReGBA_RTC), fp);
			if (rtc_time_cmp(&current_time, &latest_save_time) > 0) {
				latest_save = i;
				latest_save_time = current_time;
			}
			fclose(fp);
		} else
			SavedStateExistenceCache[i] = false;

		SavedStateExistenceCached[i] = true;
	}

	if (latest_save < 0)
		savestate_index = 0;
	else
		savestate_index = latest_save;
}

uint8_t SavedStateSquareX(uint32_t slot)
{
	return (DS_SCREEN_WIDTH * (slot + 1) / (SAVE_STATE_SLOT_NUM + 1))
		- ICON_STATEFULL.x / 2;
}

bool SavedStateFileExists(uint32_t slot)
{
	if (SavedStateExistenceCached[slot])
		return SavedStateExistenceCache[slot];

	char SavedStateFilename[PATH_MAX];
	ReGBA_GetSavedStateFilename(SavedStateFilename, CurrentGamePath, slot);
	FILE *SavedStateFile = fopen(SavedStateFilename, "r");
	bool Result = SavedStateFile != NULL;
	if (Result) {
		fclose(SavedStateFile);
	}
	SavedStateExistenceCache[slot] = Result;
	SavedStateExistenceCached[slot] = true;
	return Result;
}

void SavedStateCacheInvalidate(void)
{
	int i;
	for (i = 0; i < SAVE_STATE_SLOT_NUM; i++)
		SavedStateExistenceCached[i] = false;
}

void QuickLoadState(void)
{
	DS2_SetScreenBacklights(DS_SCREEN_BOTH);

	DS2_FillScreen(DS_ENGINE_SUB, COLOR_BLACK);
	draw_message_box(DS2_GetSubScreen());
	draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SAVED_STATE_LOADING]);
	DS2_UpdateScreen(DS_ENGINE_SUB);

	int flag = 0;

	HighFrequencyCPU();
	flag = load_state(0);
	GameFrequencyCPU();

	if (flag != 0) {
		draw_message_box(DS2_GetSubScreen());
		draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SAVED_STATE_LOAD_FAILED]);
		usleep(500000); // let the failure show
	}

	DS2_FillScreen(DS_ENGINE_SUB, COLOR_BLACK);
	DS2_UpdateScreen(DS_ENGINE_SUB);

	if (gpsp_persistent_config.BottomScreenGame) {
		DS2_SetScreenBacklights(DS_SCREEN_LOWER);
	} else {
		DS2_SetScreenBacklights(DS_SCREEN_UPPER);
	}
}

void QuickSaveState(void)
{
	uint16_t* Screen = malloc(GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT * sizeof(uint16_t));
	copy_screen(Screen);

	DS2_SetScreenBacklights(DS_SCREEN_BOTH);

	DS2_FillScreen(DS_ENGINE_SUB, BGR555(0, 0, 0));
	draw_message_box(DS2_GetSubScreen());
	draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SAVED_STATE_CREATING]);
	DS2_UpdateScreen(DS_ENGINE_SUB);

	HighFrequencyCPU();
	int flag = save_state(0, Screen);
	GameFrequencyCPU();
	if (flag < 0) {
		draw_message_box(DS2_GetSubScreen());
		draw_string_vcenter(DS2_GetSubScreen(), MESSAGE_BOX_TEXT_X, MESSAGE_BOX_TEXT_Y, MESSAGE_BOX_TEXT_SX, COLOR_MSSG, msg[MSG_PROGRESS_SAVED_STATE_CREATION_FAILED]);
		usleep(500000); // let the failure show
	}

	SavedStateCacheInvalidate();

	DS2_FillScreen(DS_ENGINE_SUB, BGR555(0, 0, 0));
	DS2_UpdateScreen(DS_ENGINE_SUB);

	if (gpsp_persistent_config.BottomScreenGame) {
		DS2_SetScreenBacklights(DS_SCREEN_LOWER);
	} else {
		DS2_SetScreenBacklights(DS_SCREEN_UPPER);
	}
}

void LowFrequencyCPU()
{
	DS2_LowClockSpeed();
}

void HighFrequencyCPU()
{
	DS2_HighClockSpeed();
}

void GameFrequencyCPU()
{
	uint32_t cpu_hz = gpsp_persistent_config.cpu_hz, mem_hz = gpsp_persistent_config.mem_hz;
	DS2_SetClockSpeed(&cpu_hz, &mem_hz);
}

static uint32_t save_ss_bmp(const uint16_t *image)
{
    static const uint8_t header[] = {
		'B',  'M',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
		0x00, 0x00,
		GBA_SCREEN_WIDTH & 0xFF, (GBA_SCREEN_WIDTH >> 8) & 0xFF,
		(GBA_SCREEN_WIDTH >> 16) & 0xFF, (GBA_SCREEN_WIDTH >> 24) & 0xFF,
		GBA_SCREEN_HEIGHT & 0xFF, (GBA_SCREEN_HEIGHT >> 8) & 0xFF,
		(GBA_SCREEN_HEIGHT >> 16) & 0xFF, (GBA_SCREEN_HEIGHT >> 24) & 0xFF,
		0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	uint32_t ret = 0;
	char FileNameNoExt[PATH_MAX];
	char save_ss_path[PATH_MAX];
	size_t len;
	time_t rawtime = time(NULL);
	struct tm* tm = localtime(&rawtime);
	uint8_t *rgb_data, *rgb_cur;
	uint32_t x, y;

	rgb_cur = rgb_data = malloc(GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT * 3);
	if (rgb_data == NULL)
		return 0;

	GetFileNameNoExtension(FileNameNoExt, CurrentGamePath);
	sprintf(save_ss_path, "%s/%s_", DEFAULT_SS_DIR, FileNameNoExt);
	len = strlen(save_ss_path);
	strftime(save_ss_path + len, sizeof(save_ss_path) - len, "%m%d%H%M%S.bmp", tm);

	for (y = 0; y < GBA_SCREEN_HEIGHT; y++) {
		for (x = 0; x < GBA_SCREEN_WIDTH; x++) {
			uint16_t col = image[(GBA_SCREEN_HEIGHT - y - 1) * GBA_SCREEN_WIDTH + x];
			*rgb_cur++ = BGR555_B(col) << 3;
			*rgb_cur++ = BGR555_G(col) << 3;
			*rgb_cur++ = BGR555_R(col) << 3;
		}
	}

	FILE *ss = fopen(save_ss_path, "wb");
	if (ss == NULL)
		goto with_alloc;
	if (fwrite(header, sizeof(header), 1, ss) == 0)
		goto with_file;
	if (fwrite(rgb_data, GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT * 3, 1, ss) == 0)
		goto with_file;
	ret = 1;

with_file:
	fclose(ss);
with_alloc:
	free(rgb_data);

	return ret;
}

uint32_t save_menu_ss_bmp(const uint16_t *image)
{
    static const uint8_t header[] = {
		'B',  'M',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
		0x00, 0x00,
		DS_SCREEN_WIDTH & 0xFF, (DS_SCREEN_WIDTH >> 8) & 0xFF,
		(DS_SCREEN_WIDTH >> 16) & 0xFF, (DS_SCREEN_WIDTH >> 24) & 0xFF,
		DS_SCREEN_HEIGHT & 0xFF, (DS_SCREEN_HEIGHT >> 8) & 0xFF,
		(DS_SCREEN_HEIGHT >> 16) & 0xFF, (DS_SCREEN_HEIGHT >> 24) & 0xFF,
		0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	uint32_t ret = 0;
	char save_ss_path[PATH_MAX];
	size_t len;
	time_t rawtime = time(NULL);
	struct tm* tm = localtime(&rawtime);
	uint8_t *rgb_data, *rgb_cur;
	uint32_t x, y;

	rgb_cur = rgb_data = malloc(DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT * 3);
	if (rgb_data == NULL)
		return 0;

	sprintf(save_ss_path, "%s/%s", DEFAULT_SS_DIR, "gui_");
	len = strlen(save_ss_path);
	strftime(save_ss_path + len, sizeof(save_ss_path) - len, "%m%d%H%M%S.bmp", tm);

	for (y = 0; y < DS_SCREEN_HEIGHT; y++) {
		for (x = 0; x < DS_SCREEN_WIDTH; x++) {
			uint16_t col = image[(DS_SCREEN_HEIGHT - y - 1) * DS_SCREEN_WIDTH + x];
			*rgb_cur++ = BGR555_B(col) << 3;
			*rgb_cur++ = BGR555_G(col) << 3;
			*rgb_cur++ = BGR555_R(col) << 3;
		}
	}

	FILE *ss = fopen(save_ss_path, "wb");
	if (ss == NULL)
		goto with_alloc;
	if (fwrite(header, sizeof(header), 1, ss) == 0)
		goto with_file;
	if (fwrite(rgb_data, DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT * 3, 1, ss) == 0)
		goto with_file;
	ret = 1;

with_file:
	fclose(ss);
with_alloc:
	free(rgb_data);

	return ret;
}

void game_set_frameskip()
{
	if (game_fast_forward || temporary_fast_forward) {
		// If fast-forward is active, force frameskipping to be 9.
		AUTO_SKIP = 0;
		SKIP_RATE = 9;
	} else if (game_persistent_config.frameskip_value == 0) {
		// If the value for the persistent setting is 0 ('Keep up with game'),
		// then we set auto frameskip and a value of 2 for now.
		AUTO_SKIP = 1;
		SKIP_RATE = 2;
	} else {
		AUTO_SKIP = 0;
		// Otherwise, values from 1..N map to frameskipping 0..N-1.
		SKIP_RATE = game_persistent_config.frameskip_value - 1;
	}
}

void set_button_map() {
  memcpy(game_config.gamepad_config_map, gamepad_config_map_init, sizeof(gamepad_config_map_init));
  {
    int i;
    for (i = 0; i < 6; i++) {
      if (gpsp_persistent_config.ButtonMappings[i] == 0) {
        gpsp_persistent_config.ButtonMappings[0] = DS_BUTTON_A;
        gpsp_persistent_config.ButtonMappings[1] = DS_BUTTON_B;
        gpsp_persistent_config.ButtonMappings[2] = DS_BUTTON_SELECT;
        gpsp_persistent_config.ButtonMappings[3] = DS_BUTTON_START;
        gpsp_persistent_config.ButtonMappings[4] = DS_BUTTON_R;
        gpsp_persistent_config.ButtonMappings[5] = DS_BUTTON_L;
        break;
      }
    }
  }

  /* GBA A */ game_config.gamepad_config_map[0] = game_persistent_config.ButtonMappings[0] != 0 ? game_persistent_config.ButtonMappings[0] : gpsp_persistent_config.ButtonMappings[0];
  /* GBA B */ game_config.gamepad_config_map[1] = game_persistent_config.ButtonMappings[1] != 0 ? game_persistent_config.ButtonMappings[1] : gpsp_persistent_config.ButtonMappings[1];
  /* GBA SELECT */ game_config.gamepad_config_map[2] = game_persistent_config.ButtonMappings[2] != 0 ? game_persistent_config.ButtonMappings[2] : gpsp_persistent_config.ButtonMappings[2];
  /* GBA START */ game_config.gamepad_config_map[3] = game_persistent_config.ButtonMappings[3] != 0 ? game_persistent_config.ButtonMappings[3] : gpsp_persistent_config.ButtonMappings[3];
  /* GBA R */ game_config.gamepad_config_map[8] = game_persistent_config.ButtonMappings[4] != 0 ? game_persistent_config.ButtonMappings[4] : gpsp_persistent_config.ButtonMappings[4];
  /* GBA L */ game_config.gamepad_config_map[9] = game_persistent_config.ButtonMappings[5] != 0 ? game_persistent_config.ButtonMappings[5] : gpsp_persistent_config.ButtonMappings[5];
  /* Rapid fire A */ game_config.gamepad_config_map[10] = game_persistent_config.ButtonMappings[6] != 0 ? game_persistent_config.ButtonMappings[6] : gpsp_persistent_config.ButtonMappings[6];
  /* Rapid fire B */ game_config.gamepad_config_map[11] = game_persistent_config.ButtonMappings[7] != 0 ? game_persistent_config.ButtonMappings[7] : gpsp_persistent_config.ButtonMappings[7];
}

void game_set_rewind() {
	if (game_persistent_config.rewind_value == 0) {
		game_config.backward = 0;
	} else {
		game_config.backward = 1;
		game_config.backward_time = game_persistent_config.rewind_value - 1;
		init_rewind();
		switch (game_config.backward_time)
		{
		case  0: frame_interval = 15; break;
		case  1: frame_interval = 30; break;
		case  2: frame_interval = 60; break;
		case  3: frame_interval = 120; break;
		case  4: frame_interval = 300; break;
		case  5: frame_interval = 600; break;
		default: frame_interval = 60; break;
		}
	}
}

/*
* GUI Initialize
*/
static bool Get_Args(char *file, char **filebuf){
  FILE* dat = fopen(file, "rb");
  if (dat) {
    int i = 0;
    while (!feof (dat)) {
      fgets(filebuf[i], 512, dat);
      int len = strlen(filebuf[i]);
      if (filebuf[i][len - 1] == '\n')
        filebuf[i][len - 1] = '\0';
      i++;
    }

    fclose(dat);
    remove(file);
    return i;
  }
  return 0;
}

int CheckLoad_Arg(){
  char *argarray[2];
  argarray[0] = argv[0];
  argarray[1] = argv[1];

  if (!Get_Args("/plgargs.dat", argarray))
    return 0;

  return 1;
}

int gui_init(uint32_t lang_id)
{
	int flag;

	HighFrequencyCPU(); // Crank it up. When the menu starts, -> 0.

	// Find the "TEMPGBA" system directory
	DIR *current_dir;

	if (CheckLoad_Arg()) {
		// Copy the full path to the plugin file from argv[0].
		strcpy(main_path, "fat:");
		strcat(main_path, argv[0]);
		// Remove the name of the plugin file, leaving only the directories.
		char *endStr = strrchr(main_path, '/');
		*endStr = '\0';

		// Check to make sure the directory is a valid TempGBA folder.
		char tempPath[PATH_MAX];
		strcpy(tempPath, main_path);
		strcat(tempPath, "/system/gui");
		DIR *testDir = opendir(tempPath);
		if (!testDir)
			// Not a valid TempGBA install.
			strcpy(main_path, "fat:/TEMPGBA");
		else
			// Test was successful, do nothing.
			closedir(testDir);
	} else
		strcpy(main_path, "fat:/TEMPGBA");

	/* Various existence checks. */
	current_dir = opendir(main_path);
	if (current_dir)
		closedir(current_dir);
	else {
		strcpy(main_path, "fat:/_SYSTEM/PLUGINS/TEMPGBA");
		current_dir = opendir(main_path);
		if (current_dir)
			closedir(current_dir);
		else {
			fprintf(stderr, "/TEMPGBA: Directory missing\nPress any key to return to\nthe menu\n\n/TEMPGBA: Dossier manquant\nAppuyer sur une touche pour\nretourner au menu");
			goto gui_init_err;
		}
	}

	show_logo();
	DS2_UpdateScreen(DS_ENGINE_SUB);

	load_config_file();
	lang_id = gpsp_persistent_config.language;

	flag = icon_init(lang_id);
	if (flag != 0) {
		fprintf(stderr, "Some icons are missing\nLoad them onto your card\nPress any key to return to\nthe menu\n\nDes icones sont manquantes\nChargez-les sur votre carte\nAppuyer sur une touche pour\nretourner au menu");
		goto gui_init_err;
	}

	flag = color_init();
	if (flag != 0) {
		fprintf(stderr, "SYSTEM/GUI/uicolors.txt\nis missing\nPress any key to return to\nthe menu\n\nSYSTEM/GUI/uicolors.txt\nest manquant\nAppuyer sur une touche pour\nretourner au menu");
		goto gui_init_err;
	}

	flag = load_font();
	if (flag != 0) {
		fprintf(stderr, "Font library initialisation\nerror (%d)\nPress any key to return to\nthe menu\n\nErreur d'initalisation de la\npolice de caracteres (%d)\nAppuyer sur une touche pour\nretourner au menu", flag, flag);
		goto gui_init_err;
	}

	flag = load_language_msg(LANGUAGE_PACK, lang_id);
	if (flag != 0) {
		fprintf(stderr, "Language pack initialisation\nerror (%d)\nPress any key to return to\nthe menu\n\nErreur d'initalisation du\npack de langue (%d)\nAppuyer sur une touche pour\nretourner au menu", flag, flag);
		goto gui_init_err;
	}

	StatsInit();

	return 1;

gui_init_err:
	DS2_AwaitAnyButtons();
	exit(EXIT_FAILURE);
}
