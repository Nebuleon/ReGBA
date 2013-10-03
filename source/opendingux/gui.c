/* ReGBA - In-application menu
 *
 * Copyright (C) 2013 Dingoonity user Nebuleon
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

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include "common.h"
#include "port.h"

#define COLOR_BACKGROUND       RGB888_TO_RGB565(  0,  48,   0)
#define COLOR_INACTIVE_TEXT    RGB888_TO_RGB565( 64, 160,  64)
#define COLOR_INACTIVE_OUTLINE RGB888_TO_RGB565(  0,   0,   0)
#define COLOR_ACTIVE_TEXT      RGB888_TO_RGB565(255, 255, 255)
#define COLOR_ACTIVE_OUTLINE   RGB888_TO_RGB565(  0,   0,   0)
#define COLOR_TITLE_TEXT       RGB888_TO_RGB565(128, 255, 128)
#define COLOR_TITLE_OUTLINE    RGB888_TO_RGB565(  0,  96,   0)
#define COLOR_ERROR_TEXT       RGB888_TO_RGB565(255,  64,  64)
#define COLOR_ERROR_OUTLINE    RGB888_TO_RGB565( 80,   0,   0)

static SDL_Rect ScreenRectangle = {
	0, 0, GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT
};

enum MenuEntryKind {
	KIND_OPTION,
	KIND_SUBMENU,
	KIND_DISPLAY,
	KIND_CUSTOM,
};

enum MenuDataType {
	TYPE_STRING,
	TYPE_INT32,
	TYPE_UINT32,
	TYPE_INT64,
	TYPE_UINT64,
};

struct MenuEntry;

struct Menu;

/*
 * MenuModifyFunction is the type of a function that acts on an input in the
 * menu. The function is assigned this input via the MenuEntry struct's
 * various Button<Name>Function members, InitFunction, EndFunction, etc.
 * Variables:
 *   1: On entry into the function, points to a memory location containing
 *     a pointer to the active menu. On exit from the function, the menu may
 *     be modified to a new one, in which case the function has chosen to
 *     activate that new menu; the EndFunction of the old menu is called,
 *     then the InitFunction of the new menu is called.
 * 
 *     The exception to this rule is the NULL menu. If NULL is chosen to be
 *     activated, then no InitFunction is called; additionally, the menu is
 *     exited.
 *   2: On entry into the function, points to a memory location containing the
 *     index among the active menu's Entries array corresponding to the active
 *     menu entry. On exit from the function, the menu entry index may be
 *     modified to a new one, in which case the function has chosen to
 *     activate that new menu entry. If the menu itself has changed, the code
 *     in ReGBA_Menu will activate the first item of that menu.
 */
typedef void (*MenuModifyFunction) (struct Menu**, uint32_t*);

/*
 * MenuEntryDisplayFunction is the type of a function that displays an element
 * (the name or the value, depending on which member receives a function of
 * this type) of a single menu entry.
 * Input:
 *   1: A pointer to the data for the menu entry whose part is being drawn.
 *   2: A pointer to the data for the active menu entry.
 */
typedef void (*MenuEntryDisplayFunction) (struct MenuEntry*, struct MenuEntry*);

/*
 * MenuEntryFunction is the type of a function that displays an element
 * (the name or the value, depending on which member receives a function of
 * this type) of a single menu entry.
 * Input:
 *   1: The menu entry whose part is being drawn.
 *   2: The active menu entry.
 */
typedef void (*MenuEntryFunction) (struct Menu*, struct MenuEntry*);

/*
 * MenuFunction is the type of a function that runs when a menu is being
 * initialised or finalised, depending on which member receives a function of
 * this type.
 * Input:
 *   1: The menu that is being initialised or finalised.
 */
typedef void (*MenuFunction) (struct Menu*);

struct MenuEntry {
	enum MenuEntryKind Kind;
	char* Name;
	char* PersistentName;
	enum MenuDataType DisplayType;
	uint32_t Position;      // 0-based line number with default functions.
	                        // Custom display functions may give it a new
	                        // meaning.
	void* Target;           // With KIND_OPTION, must point to uint32_t.
	                        // With KIND_DISPLAY, must point to the data type
	                        // specified by DisplayType.
	                        // With KIND_SUBMENU, this is struct Menu*.
	// DisplayChoices and ChoiceCount are only used with KIND_OPTION.
	uint32_t ChoiceCount;
	MenuModifyFunction ButtonEnterFunction;
	MenuEntryFunction ButtonLeftFunction;
	MenuEntryFunction ButtonRightFunction;
	MenuEntryDisplayFunction DisplayNameFunction;
	MenuEntryDisplayFunction DisplayValueFunction;
	void* UserData;
	struct {
		char* Pretty;
		char* Persistent;
	} Choices[];
};

struct Menu {
	struct Menu* Parent;
	char* Title;
	MenuFunction DisplayBackgroundFunction;
	MenuFunction DisplayTitleFunction;
	MenuEntryFunction DisplayDataFunction;
	MenuModifyFunction ButtonUpFunction;
	MenuModifyFunction ButtonDownFunction;
	MenuModifyFunction ButtonLeaveFunction;
	MenuFunction InitFunction;
	MenuFunction EndFunction;
	void* UserData;
	uint32_t ActiveEntryIndex;
	struct MenuEntry* Entries[]; // Entries are ended by a NULL pointer value.
};

static void DefaultUpFunction(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	if (*ActiveMenuEntryIndex == 0)  // went over the top
	{  // go back to the bottom
		while ((*ActiveMenu)->Entries[*ActiveMenuEntryIndex] != NULL)
			(*ActiveMenuEntryIndex)++;
	}
	(*ActiveMenuEntryIndex)--;
}

static void DefaultDownFunction(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	(*ActiveMenuEntryIndex)++;
	if ((*ActiveMenu)->Entries[*ActiveMenuEntryIndex] == NULL)  // fell through the bottom
		*ActiveMenuEntryIndex = 0;  // go back to the top
}

static void DefaultRightFunction(struct Menu* ActiveMenu, struct MenuEntry* ActiveMenuEntry)
{
	if (ActiveMenuEntry->Kind == KIND_OPTION)
	{
		uint32_t* Target = (uint32_t*) ActiveMenuEntry->Target;
		(*Target)++;
		if (*Target >= ActiveMenuEntry->ChoiceCount)
			*Target = 0;
	}
}

static void DefaultLeftFunction(struct Menu* ActiveMenu, struct MenuEntry* ActiveMenuEntry)
{
	if (ActiveMenuEntry->Kind == KIND_OPTION)
	{
		uint32_t* Target = (uint32_t*) ActiveMenuEntry->Target;
		if (*Target == 0)
			*Target = ActiveMenuEntry->ChoiceCount;
		(*Target)--;
	}
}

static void DefaultEnterFunction(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	if ((*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Kind == KIND_SUBMENU)
	{
		*ActiveMenu = (struct Menu*) (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Target;
	}
}

static void DefaultLeaveFunction(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	*ActiveMenu = (*ActiveMenu)->Parent;
}

static void DefaultDisplayNameFunction(struct MenuEntry* DrawnMenuEntry, struct MenuEntry* ActiveMenuEntry)
{
	uint32_t TextWidth = GetRenderedWidth(DrawnMenuEntry->Name);
	if (TextWidth <= GCW0_SCREEN_WIDTH - 2)
	{
		bool IsActive = (DrawnMenuEntry == ActiveMenuEntry);
		uint16_t TextColor = IsActive ? COLOR_ACTIVE_TEXT : COLOR_INACTIVE_TEXT;
		uint16_t OutlineColor = IsActive ? COLOR_ACTIVE_OUTLINE : COLOR_INACTIVE_OUTLINE;
		print_string_outline(DrawnMenuEntry->Name, TextColor, OutlineColor, 1, GetRenderedHeight(" ") * (DrawnMenuEntry->Position + 2) + 1);
	}
	else
		ReGBA_Trace("W: Hid name '%s' from the menu due to it being too long", DrawnMenuEntry->Name);
}

static void print_u64(char* Result, uint64_t Value)
{
	if (Value == 0)
		strcpy(Result, "0");
	else
	{
		uint_fast8_t Length = 0;
		uint64_t Temp = Value;
		while (Temp > 0)
		{
			Temp /= 10;
			Length++;
		}
		Result[Length] = '\0';
		while (Value > 0)
		{
			Length--;
			Result[Length] = '0' + (Value % 10);
			Value /= 10;
		}
	}
}

static void print_i64(char* Result, int64_t Value)
{
	if (Value < -9223372036854775807)
		strcpy(Result, "-9223372036854775808");
	else if (Value < 0)
	{
		Result[0] = '-';
		print_u64(Result + 1, (uint64_t) -Value);
	}
	else
		print_u64(Result, (uint64_t) Value);
}

static void DefaultDisplayValueFunction(struct MenuEntry* DrawnMenuEntry, struct MenuEntry* ActiveMenuEntry)
{
	if (DrawnMenuEntry->Kind == KIND_OPTION || DrawnMenuEntry->Kind == KIND_DISPLAY)
	{
		char* Value;
		char Temp[21];
		bool Error = false;
		if (DrawnMenuEntry->Kind == KIND_OPTION)
		{
			if (*(uint32_t*) DrawnMenuEntry->Target < DrawnMenuEntry->ChoiceCount)
				Value = DrawnMenuEntry->Choices[*(uint32_t*) DrawnMenuEntry->Target].Pretty;
			else
			{
				Value = "Out of bounds";
				Error = true;
			}
		}
		else if (DrawnMenuEntry->Kind == KIND_DISPLAY)
		{
			switch (DrawnMenuEntry->DisplayType)
			{
				case TYPE_STRING:
					Value = (char*) DrawnMenuEntry->Target;
					break;
				case TYPE_INT32:
					sprintf(Temp, "%" PRIi32, *(int32_t*) DrawnMenuEntry->Target);
					Value = Temp;
					break;
				case TYPE_UINT32:
					sprintf(Temp, "%" PRIu32, *(uint32_t*) DrawnMenuEntry->Target);
					Value = Temp;
					break;
				case TYPE_INT64:
					print_i64(Temp, *(int64_t*) DrawnMenuEntry->Target);
					Value = Temp;
					break;
				case TYPE_UINT64:
					print_u64(Temp, *(uint64_t*) DrawnMenuEntry->Target);
					Value = Temp;
					break;
				default:
					Value = "Unknown type";
					Error = true;
					break;
			}
		}
		uint32_t TextWidth = GetRenderedWidth(Value);
		if (TextWidth <= GCW0_SCREEN_WIDTH - 2)
		{
			bool IsActive = (DrawnMenuEntry == ActiveMenuEntry);
			uint16_t TextColor = Error ? COLOR_ERROR_TEXT : (IsActive ? COLOR_ACTIVE_TEXT : COLOR_INACTIVE_TEXT);
			uint16_t OutlineColor = Error ? COLOR_ERROR_OUTLINE : (IsActive ? COLOR_ACTIVE_OUTLINE : COLOR_INACTIVE_OUTLINE);
			print_string_outline(Value, TextColor, OutlineColor, GCW0_SCREEN_WIDTH - TextWidth - 1, GetRenderedHeight(" ") * (DrawnMenuEntry->Position + 2) + 1);
		}
		else
			ReGBA_Trace("W: Hid value '%s' from the menu due to it being too long", Value);
	}
}

static void DefaultDisplayBackgroundFunction(struct Menu* ActiveMenu)
{
	SDL_FillRect(OutputSurface, &ScreenRectangle, COLOR_BACKGROUND);
}

static void DefaultDisplayDataFunction(struct Menu* ActiveMenu, struct MenuEntry* ActiveMenuEntry)
{
	uint32_t DrawnMenuEntryIndex = 0;
	struct MenuEntry* DrawnMenuEntry = ActiveMenu->Entries[0];
	for (; DrawnMenuEntry != NULL; DrawnMenuEntryIndex++, DrawnMenuEntry = ActiveMenu->Entries[DrawnMenuEntryIndex])
	{
		MenuEntryDisplayFunction Function = DrawnMenuEntry->DisplayNameFunction;
		if (Function == NULL) Function = &DefaultDisplayNameFunction;
		(*Function)(DrawnMenuEntry, ActiveMenuEntry);

		Function = DrawnMenuEntry->DisplayValueFunction;
		if (Function == NULL) Function = &DefaultDisplayValueFunction;
		(*Function)(DrawnMenuEntry, ActiveMenuEntry);

		DrawnMenuEntry++;
	}
}

static void DefaultDisplayTitleFunction(struct Menu* ActiveMenu)
{
	uint32_t TextWidth = GetRenderedWidth(ActiveMenu->Title);
	if (TextWidth <= GCW0_SCREEN_WIDTH - 2)
		print_string_outline(ActiveMenu->Title, COLOR_TITLE_TEXT, COLOR_TITLE_OUTLINE, (GCW0_SCREEN_WIDTH - TextWidth) / 2, 1);
	else
		ReGBA_Trace("W: Hid title '%s' from the menu due to it being too long", ActiveMenu->Title);
}

// -- Custom actions --

static void ActionExit(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	quit();
	*ActiveMenu = NULL;
}

static void ActionReturn(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	*ActiveMenu = NULL;
}

static void ActionReset(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	reset_gba();
	reg[CHANGED_PC_STATUS] = 1;
	*ActiveMenu = NULL;
}

// -- Forward declarations --

static struct Menu MainMenu;
static struct Menu DebugMenu;

// -- Debug > Native code stats --

static struct MenuEntry NativeCodeMenu_ROPeak = {
	.Kind = KIND_DISPLAY, .Position = 0, .Name = "Read-only bytes at peak",
	.DisplayType = TYPE_UINT64, .Target = &Stats.TranslationBytesPeak[TRANSLATION_REGION_READONLY]
};

static struct MenuEntry NativeCodeMenu_RWPeak = {
	.Kind = KIND_DISPLAY, .Position = 1, .Name = "Writable bytes at peak",
	.DisplayType = TYPE_UINT64, .Target = &Stats.TranslationBytesPeak[TRANSLATION_REGION_WRITABLE]
};

static struct MenuEntry NativeCodeMenu_ROFlushed = {
	.Kind = KIND_DISPLAY, .Position = 2, .Name = "Read-only bytes flushed",
	.DisplayType = TYPE_UINT64, .Target = &Stats.TranslationBytesFlushed[TRANSLATION_REGION_READONLY]
};

static struct MenuEntry NativeCodeMenu_RWFlushed = {
	.Kind = KIND_DISPLAY, .Position = 3, .Name = "Writable bytes flushed",
	.DisplayType = TYPE_UINT64, .Target = &Stats.TranslationBytesFlushed[TRANSLATION_REGION_WRITABLE]
};

static struct Menu NativeCodeMenu = {
	.Parent = &DebugMenu, .Title = "Native code statistics",
	.Entries = { &NativeCodeMenu_ROPeak, &NativeCodeMenu_RWPeak, &NativeCodeMenu_ROFlushed, &NativeCodeMenu_RWFlushed, NULL }
};

static struct MenuEntry DebugMenu_NativeCode = {
	.Kind = KIND_SUBMENU, .Position = 0, .Name = "Native code statistics...",
	.Target = &NativeCodeMenu
};

// -- Debug > Metadata stats --

static struct MenuEntry MetadataMenu_ROFull = {
	.Kind = KIND_DISPLAY, .Position = 0, .Name = "Read-only area full",
	.DisplayType = TYPE_UINT64, .Target = &Stats.TranslationFlushCount[TRANSLATION_REGION_READONLY][FLUSH_REASON_FULL_CACHE]
};

static struct MenuEntry MetadataMenu_RWFull = {
	.Kind = KIND_DISPLAY,.Position = 1, .Name = "Writable area full",
	.DisplayType = TYPE_UINT64, .Target = &Stats.TranslationFlushCount[TRANSLATION_REGION_WRITABLE][FLUSH_REASON_FULL_CACHE]
};

static struct MenuEntry MetadataMenu_BIOSLastTag = {
	.Kind = KIND_DISPLAY, .Position = 2, .Name = "BIOS tags full",
	.DisplayType = TYPE_UINT64, .Target = &Stats.MetadataClearCount[METADATA_AREA_BIOS][CLEAR_REASON_LAST_TAG]
};

static struct MenuEntry MetadataMenu_EWRAMLastTag = {
	.Kind = KIND_DISPLAY, .Position = 3, .Name = "EWRAM tags full",
	.DisplayType = TYPE_UINT64, .Target = &Stats.MetadataClearCount[METADATA_AREA_EWRAM][CLEAR_REASON_LAST_TAG]
};

static struct MenuEntry MetadataMenu_IWRAMLastTag = {
	.Kind = KIND_DISPLAY, .Position = 4, .Name = "IWRAM tags full",
	.DisplayType = TYPE_UINT64, .Target = &Stats.MetadataClearCount[METADATA_AREA_IWRAM][CLEAR_REASON_LAST_TAG]
};

static struct MenuEntry MetadataMenu_VRAMLastTag = {
	.Kind = KIND_DISPLAY, .Position = 5, .Name = "VRAM tags full",
	.DisplayType = TYPE_UINT64, .Target = &Stats.MetadataClearCount[METADATA_AREA_VRAM][CLEAR_REASON_LAST_TAG]
};

static struct MenuEntry MetadataMenu_PartialClears = {
	.Kind = KIND_DISPLAY, .Position = 7, .Name = "Partial clears",
	.DisplayType = TYPE_UINT64, .Target = &Stats.PartialFlushCount
};

static struct Menu MetadataMenu = {
	.Parent = &DebugMenu, .Title = "Metadata clear statistics",
	.Entries = { &MetadataMenu_ROFull, &MetadataMenu_RWFull, &MetadataMenu_BIOSLastTag, &MetadataMenu_EWRAMLastTag, &MetadataMenu_IWRAMLastTag, &MetadataMenu_VRAMLastTag, &MetadataMenu_PartialClears, NULL }
};

static struct MenuEntry DebugMenu_Metadata = {
	.Kind = KIND_SUBMENU, .Position = 1, .Name = "Metadata clear statistics...",
	.Target = &MetadataMenu
};

// -- Debug > Execution stats --

static struct MenuEntry ExecutionMenu_SoundUnderruns = {
	.Kind = KIND_DISPLAY, .Position = 0, .Name = "Sound buffer underruns",
	.DisplayType = TYPE_UINT64, .Target = &Stats.SoundBufferUnderrunCount
};

static struct MenuEntry ExecutionMenu_FramesEmulated = {
	.Kind = KIND_DISPLAY, .Position = 1, .Name = "Frames emulated",
	.DisplayType = TYPE_UINT64, .Target = &Stats.TotalEmulatedFrames
};

#ifdef PERFORMANCE_IMPACTING_STATISTICS
static struct MenuEntry ExecutionMenu_ARMOps = {
	.Kind = KIND_DISPLAY, .Position = 2, .Name = "ARM opcodes decoded",
	.DisplayType = TYPE_UINT64, .Target = &Stats.ARMOpcodesDecoded
};

static struct MenuEntry ExecutionMenu_ThumbOps = {
	.Kind = KIND_DISPLAY, .Position = 3, .Name = "Thumb opcodes decoded",
	.DisplayType = TYPE_UINT64, .Target = &Stats.ThumbOpcodesDecoded
};

static struct MenuEntry ExecutionMenu_MemAccessors = {
	.Kind = KIND_DISPLAY, .Position = 4, .Name = "Memory accessors patched",
	.DisplayType = TYPE_UINT32, .Target = &Stats.WrongAddressLineCount
};
#endif

static struct Menu ExecutionMenu = {
	.Parent = &DebugMenu, .Title = "Execution statistics",
	.Entries = { &ExecutionMenu_SoundUnderruns, &ExecutionMenu_FramesEmulated
#ifdef PERFORMANCE_IMPACTING_STATISTICS
		, &ExecutionMenu_ARMOps, &ExecutionMenu_ThumbOps, &ExecutionMenu_MemAccessors
#endif
		, NULL }
};

static struct MenuEntry DebugMenu_Execution = {
	.Kind = KIND_SUBMENU, .Position = 2, .Name = "Execution statistics...",
	.Target = &ExecutionMenu
};

// -- Debug > Code reuse stats --

#ifdef PERFORMANCE_IMPACTING_STATISTICS
static struct MenuEntry ReuseMenu_OpsRecompiled = {
	.Kind = KIND_DISPLAY, .Position = 0, .Name = "Opcodes recompiled",
	.DisplayType = TYPE_UINT64, .Target = &Stats.OpcodeRecompilationCount
};

static struct MenuEntry ReuseMenu_BlocksRecompiled = {
	.Kind = KIND_DISPLAY, .Position = 1, .Name = "Blocks recompiled",
	.DisplayType = TYPE_UINT64, .Target = &Stats.BlockRecompilationCount
};

static struct MenuEntry ReuseMenu_OpsReused = {
	.Kind = KIND_DISPLAY, .Position = 2, .Name = "Opcodes reused",
	.DisplayType = TYPE_UINT64, .Target = &Stats.OpcodeReuseCount
};

static struct MenuEntry ReuseMenu_BlocksReused = {
	.Kind = KIND_DISPLAY, .Position = 3, .Name = "Blocks reused",
	.DisplayType = TYPE_UINT64, .Target = &Stats.BlockReuseCount
};

static struct Menu ReuseMenu = {
	.Parent = &DebugMenu, .Title = "Code reuse statistics",
	.Entries = { &ReuseMenu_OpsRecompiled, &ReuseMenu_BlocksRecompiled, &ReuseMenu_OpsReused, &ReuseMenu_BlocksReused, NULL }
};

static struct MenuEntry DebugMenu_Reuse = {
	.Kind = KIND_SUBMENU, .Position = 3, .Name = "Code reuse statistics...",
	.Target = &ReuseMenu
};
#endif

static struct MenuEntry ROMInfoMenu_GameName = {
	.Kind = KIND_DISPLAY, .Position = 0, .Name = "game_name =",
	.DisplayType = TYPE_STRING, .Target = gamepak_title
};

static struct MenuEntry ROMInfoMenu_GameCode = {
	.Kind = KIND_DISPLAY, .Position = 1, .Name = "game_code =",
	.DisplayType = TYPE_STRING, .Target = gamepak_code
};

static struct MenuEntry ROMInfoMenu_VendorCode = {
	.Kind = KIND_DISPLAY, .Position = 2, .Name = "vender_code =",
	.DisplayType = TYPE_STRING, .Target = gamepak_maker
};

static struct Menu ROMInfoMenu = {
	.Parent = &DebugMenu, .Title = "ROM information",
	.Entries = { &ROMInfoMenu_GameName, &ROMInfoMenu_GameCode, &ROMInfoMenu_VendorCode, NULL }
};

static struct MenuEntry DebugMenu_ROMInfo = {
	.Kind = KIND_SUBMENU, .Position = 5, .Name = "ROM information...",
	.Target = &ROMInfoMenu
};

// -- Debug --

static struct Menu DebugMenu = {
	.Parent = &MainMenu, .Title = "Performance and debugging",
	.Entries = { &DebugMenu_NativeCode, &DebugMenu_Metadata, &DebugMenu_Execution
#ifdef PERFORMANCE_IMPACTING_STATISTICS
		, &DebugMenu_Reuse
#endif
		, &DebugMenu_ROMInfo
		, NULL }
};

// -- Main Menu --

// TODO Put this in a dedicated Display Settings menu
static struct MenuEntry MainMenu_BootSource = {
	.Kind = KIND_OPTION, .Position = 0, .Name = "Boot from", .PersistentName = "boot_from",
	.Target = &BootFromBIOS,
	.ChoiceCount = 2, .Choices = { { "Cartridge ROM", "cartridge" }, { "GBA BIOS", "gba_bios" } }
};

// TODO Put this in a dedicated Display Settings menu
static struct MenuEntry MainMenu_FPSCounter = {
	.Kind = KIND_OPTION, .Position = 1, .Name = "FPS counter", .PersistentName = "fps_counter",
	.Target = &ShowFPS,
	.ChoiceCount = 2, .Choices = { { "Hide", "hide" }, { "Show", "show" } }
};

// TODO Put this in a dedicated Display Settings menu
static struct MenuEntry MainMenu_ScaleMode = {
	.Kind = KIND_OPTION, .Position = 2, .Name = "Image size", .PersistentName = "image_size",
	.Target = &ScaleMode,
	.ChoiceCount = 2, .Choices = { { "Full screen", "fullscreen" }, { "1:1 GBA pixels", "original" } }
};

// TODO Put this in a dedicated Display Settings menu
static struct MenuEntry MainMenu_Frameskip = {
	.Kind = KIND_OPTION, .Position = 3, .Name = "Frame skipping", .PersistentName = "frameskip",
	.Target = &UserFrameskip,
	.ChoiceCount = 5, .Choices = { { "Automatic", "auto" }, { "0 (~60 FPS)", "0" }, { "1 (~30 FPS)", "1" }, { "2 (~20 FPS)", "2" }, { "3 (~15 FPS)", "3" } }
};

static struct MenuEntry MainMenu_Debug = {
	.Kind = KIND_SUBMENU, .Position = 7, .Name = "Performance and debugging...",
	.Target = &DebugMenu
};

static struct MenuEntry MainMenu_Reset = {
	.Kind = KIND_CUSTOM, .Position = 9, .Name = "Reset the game",
	.ButtonEnterFunction = &ActionReset
};

static struct MenuEntry MainMenu_Return = {
	.Kind = KIND_CUSTOM, .Position = 10, .Name = "Return to the game",
	.ButtonEnterFunction = &ActionReturn
};

static struct MenuEntry MainMenu_Exit = {
	.Kind = KIND_CUSTOM, .Position = 11, .Name = "Exit",
	.ButtonEnterFunction = &ActionExit
};

static struct Menu MainMenu = {
	.Parent = NULL, .Title = "ReGBA Main Menu",
	.Entries = { &MainMenu_BootSource, &MainMenu_FPSCounter, &MainMenu_ScaleMode, &MainMenu_Frameskip, &MainMenu_Debug, &MainMenu_Reset, &MainMenu_Return, &MainMenu_Exit, NULL }
};

u32 ReGBA_Menu(enum ReGBA_MenuEntryReason EntryReason)
{
	SDL_PauseAudio(SDL_ENABLE);
	struct Menu* ActiveMenu = &MainMenu;
	if (MainMenu.InitFunction != NULL)
		(*(MainMenu.InitFunction))(&MainMenu);

	while (ActiveMenu != NULL)
	{
		// Draw.
		MenuFunction DisplayBackgroundFunction = ActiveMenu->DisplayBackgroundFunction;
		if (DisplayBackgroundFunction == NULL) DisplayBackgroundFunction = DefaultDisplayBackgroundFunction;
		(*DisplayBackgroundFunction)(ActiveMenu);

		MenuFunction DisplayTitleFunction = ActiveMenu->DisplayTitleFunction;
		if (DisplayTitleFunction == NULL) DisplayTitleFunction = DefaultDisplayTitleFunction;
		(*DisplayTitleFunction)(ActiveMenu);

		MenuEntryFunction DisplayDataFunction = ActiveMenu->DisplayDataFunction;
		if (DisplayDataFunction == NULL) DisplayDataFunction = DefaultDisplayDataFunction;
		(*DisplayDataFunction)(ActiveMenu, ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]);

		SDL_Flip(OutputSurface);
		
		// Wait. (This is for platforms on which flips don't wait for vertical
		// sync.)
		usleep(5000);

		struct Menu* PreviousMenu = ActiveMenu;

		// Get input.
		enum GUI_Action Action = GetGUIAction();
		
		switch (Action)
		{
			case GUI_ACTION_ENTER:
			{
				MenuModifyFunction ButtonEnterFunction = ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]->ButtonEnterFunction;
				if (ButtonEnterFunction == NULL) ButtonEnterFunction = DefaultEnterFunction;
				(*ButtonEnterFunction)(&ActiveMenu, &ActiveMenu->ActiveEntryIndex);
				break;
			}

			case GUI_ACTION_LEAVE:
			{
				MenuModifyFunction ButtonLeaveFunction = ActiveMenu->ButtonLeaveFunction;
				if (ButtonLeaveFunction == NULL) ButtonLeaveFunction = DefaultLeaveFunction;
				(*ButtonLeaveFunction)(&ActiveMenu, &ActiveMenu->ActiveEntryIndex);
				break;
			}

			case GUI_ACTION_UP:
			{
				MenuModifyFunction ButtonUpFunction = ActiveMenu->ButtonUpFunction;
				if (ButtonUpFunction == NULL) ButtonUpFunction = DefaultUpFunction;
				(*ButtonUpFunction)(&ActiveMenu, &ActiveMenu->ActiveEntryIndex);
				break;
			}

			case GUI_ACTION_DOWN:
			{
				MenuModifyFunction ButtonDownFunction = ActiveMenu->ButtonDownFunction;
				if (ButtonDownFunction == NULL) ButtonDownFunction = DefaultDownFunction;
				(*ButtonDownFunction)(&ActiveMenu, &ActiveMenu->ActiveEntryIndex);
				break;
			}

			case GUI_ACTION_LEFT:
			{
				MenuEntryFunction ButtonLeftFunction = ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]->ButtonLeftFunction;
				if (ButtonLeftFunction == NULL) ButtonLeftFunction = DefaultLeftFunction;
				(*ButtonLeftFunction)(ActiveMenu, ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]);
				break;
			}

			case GUI_ACTION_RIGHT:
			{
				MenuEntryFunction ButtonRightFunction = ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]->ButtonRightFunction;
				if (ButtonRightFunction == NULL) ButtonRightFunction = DefaultRightFunction;
				(*ButtonRightFunction)(ActiveMenu, ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]);
				break;
			}
			
			default:
				break;
		}

		// Possibly finalise this menu and activate and initialise a new one.
		if (ActiveMenu != PreviousMenu)
		{
			if (PreviousMenu->EndFunction != NULL)
				(*(PreviousMenu->EndFunction))(PreviousMenu);
			if (ActiveMenu != NULL && ActiveMenu->InitFunction != NULL)
				(*(ActiveMenu->InitFunction))(ActiveMenu);
		}
	}

	// Avoid leaving the menu with GBA keys pressed (namely the one bound to
	// the native exit button, B).
	while (ReGBA_GetPressedButtons() != 0)
	{
		// Draw.
		SDL_Flip(OutputSurface);
		
		// Wait. (This is for platforms on which flips don't wait for vertical
		// sync.)
		usleep(5000);
	}

	SDL_PauseAudio(SDL_DISABLE);
	StatsStopFPS();
	timespec Now;
	clock_gettime(CLOCK_MONOTONIC, &Now);
	Stats.LastFPSCalculationTime = Now;
	return 0;
}

static void Menu_SaveOption(FILE_TAG_TYPE fd, struct MenuEntry *entry)
{
	char buf[256];

	snprintf(buf, 256, "%s = %s #%s\n", entry->PersistentName,
			entry->Choices[*((uint32_t*)entry->Target)].Persistent, entry->Choices[*((uint32_t*)entry->Target)].Pretty);

	FILE_WRITE(fd, &buf, strlen(buf) * sizeof(buf[0]));
}

static void Menu_SaveIterateRecurse(FILE_TAG_TYPE fd, struct Menu *menu)
{
	struct MenuEntry *cur;
	int i=0;

	while ((cur = menu->Entries[i++])) {
		switch (cur->Kind) {
		case KIND_SUBMENU:
			Menu_SaveIterateRecurse(fd, cur->Target);
			break;
		case KIND_OPTION:
			Menu_SaveOption(fd, cur);
			break;
		default:
			break;
		}
	}
}

static struct MenuEntry *Menu_FindByPersistentName(struct Menu *menu, char *name)
{
	struct MenuEntry *retcode = NULL;

	struct MenuEntry *cur;
	int i=0;

	while ((cur = menu->Entries[i++])) {
		switch (cur->Kind) {
		case KIND_SUBMENU:
			if ((retcode = Menu_FindByPersistentName(cur->Target, name)))
				return retcode;
			break;
		case KIND_OPTION:
			if (!strcmp(cur->PersistentName, name))
				return cur;
			break;
		default:
			break;
		}
	}

	return retcode;
}

bool ReGBA_SaveSettings(char *cfg_name)
{
	char fname[MAX_PATH + 1];
	if (strlen(main_path) + strlen(cfg_name) + 5 /* / .cfg */ > MAX_PATH)
	{
		ReGBA_Trace("E: Somehow you hit the filename size limit :o\n");
		return false;
	}
	sprintf(fname, "%s/%s.cfg", main_path, cfg_name);
	FILE_TAG_TYPE fd;

	FILE_OPEN(fd, fname, WRITE);
	if(FILE_CHECK_VALID(fd)) {
		Menu_SaveIterateRecurse(fd, &MainMenu);
	}
	else
	{
		ReGBA_Trace("E: Couldn't open file %s for writing.\n", fname);
		return false;
	}

	FILE_CLOSE(fd);
	return true;
}

void ReGBA_LoadSettings(char *cfg_name)
{
	char fname[MAX_PATH + 1];
	if (strlen(main_path) + strlen(cfg_name) + 5 /* / .cfg */ > MAX_PATH)
	{
		ReGBA_Trace("E: Somehow you hit the filename size limit :o\n");
		return;
	}
	sprintf(fname, "%s/%s.cfg", main_path, cfg_name);

	FILE_TAG_TYPE fd;

	FILE_OPEN(fd, fname, READ);

	if(FILE_CHECK_VALID(fd)) {
		char line[257];

		while(fgets(line, 256, fd))
		{
			int gotEqual=0;
			line[256] = '\0';

			char opt[256];
			char arg[256];

			memset(opt, '\0', 256);
			memset(arg, '\0', 256);

			char *head, *ptr;

			if (line[0] == '#')
				continue;

			for (head = &line[0], ptr = &opt[0]; *head != '\0'; head++)
			{
				if ((*head == ' ') || (*head == '=') || (*head == '\n'))
				{
					if (*head == '=')
						gotEqual = 1;

					break;
				}

				*ptr++ = tolower(*head);
			}

			if (strlen(opt) < 1) //opt's len must be > 0
				continue;

			struct MenuEntry *entry = Menu_FindByPersistentName(&MainMenu, opt);
			if (!entry)
			{
				ReGBA_Trace("W: Option '%s' is invalid", opt);
			}

			for (; *head != '\0'; head++)
			{
				if (*head == '=') {
					gotEqual = 1;
					continue;
				}
				if (*head != ' ')
				{
					break;
				}
			}

			for (ptr = &arg[0]; *head != '\0'; head++)
			{
				if ((*head == ' ') || (*head == '=') || (*head == '\n') || (*head == '#'))
				{
					if (*head == '=')
						gotEqual = 1;

					break;
				}

				*ptr++ = tolower(*head);
			}

			if (strlen(arg) < 1) { //opt's len must be > 0
				ReGBA_Trace("W: Malformed line '%s'.\n", line);
				continue;
			}

			if (!gotEqual) {
				ReGBA_Trace("W: Malformed line '%s'.\n", line);
				continue;
			}

			int i;
			for (i=0; i<entry->ChoiceCount; i++)
			{
				if (!strcmp(entry->Choices[i].Persistent, arg))
				{
					*(uint32_t*)entry->Target = i;
				}
			}
		}
	}
	else
	{
		ReGBA_Trace("E: Couldn't open file %s for loading.\n", fname);
	}
}
