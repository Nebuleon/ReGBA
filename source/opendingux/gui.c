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

#include "common.h"

#define COLOR_BACKGROUND       RGB888_TO_RGB565(  0,  48,   0)
#define COLOR_ERROR_BACKGROUND RGB888_TO_RGB565( 64,   0,   0)
#define COLOR_INACTIVE_TEXT    RGB888_TO_RGB565( 64, 160,  64)
#define COLOR_INACTIVE_OUTLINE RGB888_TO_RGB565(  0,   0,   0)
#define COLOR_ACTIVE_TEXT      RGB888_TO_RGB565(255, 255, 255)
#define COLOR_ACTIVE_OUTLINE   RGB888_TO_RGB565(  0,   0,   0)
#define COLOR_TITLE_TEXT       RGB888_TO_RGB565(128, 255, 128)
#define COLOR_TITLE_OUTLINE    RGB888_TO_RGB565(  0,  96,   0)
#define COLOR_ERROR_TEXT       RGB888_TO_RGB565(255,  64,  64)
#define COLOR_ERROR_OUTLINE    RGB888_TO_RGB565( 80,   0,   0)

// -- Shorthand for creating menu entries --

#define MENU_PER_GAME \
	.DisplayTitleFunction = DisplayPerGameTitleFunction

#define ENTRY_OPTION(_PersistentName, _Name, _Target) \
	.Kind = KIND_OPTION, .PersistentName = _PersistentName, .Name = _Name, .Target = _Target

#define ENTRY_DISPLAY(_Name, _Target, _DisplayType) \
	.Kind = KIND_DISPLAY, .Name = _Name, .Target = _Target, .DisplayType = _DisplayType

#define ENTRY_SUBMENU(_Name, _Target) \
	.Kind = KIND_SUBMENU, .Name = _Name, .Target = _Target

#define ENTRY_MANDATORY_MAPPING \
	.ChoiceCount = 0, \
	.ButtonLeftFunction = NullLeftFunction, .ButtonRightFunction = NullRightFunction, \
	.ButtonEnterFunction = ActionSetMapping, .DisplayValueFunction = DisplayButtonMappingValue, \
	.LoadFunction = LoadMappingFunction, .SaveFunction = SaveMappingFunction

#define ENTRY_OPTIONAL_MAPPING \
	.ChoiceCount = 0, \
	.ButtonLeftFunction = NullLeftFunction, .ButtonRightFunction = NullRightFunction, \
	.ButtonEnterFunction = ActionSetOrClearMapping, .DisplayValueFunction = DisplayButtonMappingValue, \
	.LoadFunction = LoadMappingFunction, .SaveFunction = SaveMappingFunction

#define ENTRY_MANDATORY_HOTKEY \
	.ChoiceCount = 0, \
	.ButtonLeftFunction = NullLeftFunction, .ButtonRightFunction = NullRightFunction, \
	.ButtonEnterFunction = ActionSetHotkey, .DisplayValueFunction = DisplayHotkeyValue, \
	.LoadFunction = LoadHotkeyFunction, .SaveFunction = SaveHotkeyFunction

#define ENTRY_OPTIONAL_HOTKEY \
	.ChoiceCount = 0, \
	.ButtonLeftFunction = NullLeftFunction, .ButtonRightFunction = NullRightFunction, \
	.ButtonEnterFunction = ActionSetOrClearHotkey, .DisplayValueFunction = DisplayHotkeyValue, \
	.LoadFunction = LoadHotkeyFunction, .SaveFunction = SaveHotkeyFunction

// -- Data --

static uint32_t SelectedState = 0;

// -- Forward declarations --

static struct Menu PerGameMainMenu;
static struct Menu DisplayMenu;
static struct Menu InputMenu;
static struct Menu HotkeyMenu;
static struct Menu ErrorScreen;
static struct Menu DebugMenu;

/*
 * A strut is an invisible line that cannot receive the focus, does not
 * display anything and does not act in any way.
 */
static struct MenuEntry Strut;

static void SavedStateUpdatePreview(struct Menu* ActiveMenu);

static bool DefaultCanFocusFunction(struct Menu* ActiveMenu, struct MenuEntry* ActiveMenuEntry)
{
	if (ActiveMenuEntry->Kind == KIND_DISPLAY)
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

static bool MoveUp(struct Menu* ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	if (*ActiveMenuEntryIndex == 0)  // Going over the top?
	{  // Go back to the bottom.
		uint32_t NullEntry = FindNullEntry(ActiveMenu);
		if (NullEntry == 0)
			return false;
		*ActiveMenuEntryIndex = NullEntry - 1;
		return true;
	}
	(*ActiveMenuEntryIndex)--;
	return true;
}

static bool MoveDown(struct Menu* ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	if (*ActiveMenuEntryIndex == 0 && ActiveMenu->Entries[*ActiveMenuEntryIndex] == NULL)
		return false;
	if (ActiveMenu->Entries[*ActiveMenuEntryIndex] == NULL)  // Is the sentinel "active"?
		*ActiveMenuEntryIndex = 0;  // Go back to the top.
	(*ActiveMenuEntryIndex)++;
	if (ActiveMenu->Entries[*ActiveMenuEntryIndex] == NULL)  // Going below the bottom?
		*ActiveMenuEntryIndex = 0;  // Go back to the top.
	return true;
}

static void DefaultUpFunction(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	if (MoveUp(*ActiveMenu, ActiveMenuEntryIndex))
	{
		// Keep moving up until a menu entry can be focused.
		uint32_t Sentinel = *ActiveMenuEntryIndex;
		MenuEntryCanFocusFunction CanFocusFunction = (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->CanFocusFunction;
		if (CanFocusFunction == NULL) CanFocusFunction = DefaultCanFocusFunction;
		while (!(*CanFocusFunction)(*ActiveMenu, (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]))
		{
			MoveUp(*ActiveMenu, ActiveMenuEntryIndex);
			if (*ActiveMenuEntryIndex == Sentinel)
			{
				// If we went through all of them, we cannot focus anything.
				// Place the focus on the NULL entry.
				*ActiveMenuEntryIndex = FindNullEntry(*ActiveMenu);
				return;
			}
			CanFocusFunction = (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->CanFocusFunction;
			if (CanFocusFunction == NULL) CanFocusFunction = DefaultCanFocusFunction;
		}
	}
}

static void DefaultDownFunction(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	if (MoveDown(*ActiveMenu, ActiveMenuEntryIndex))
	{
		// Keep moving down until a menu entry can be focused.
		uint32_t Sentinel = *ActiveMenuEntryIndex;
		MenuEntryCanFocusFunction CanFocusFunction = (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->CanFocusFunction;
		if (CanFocusFunction == NULL) CanFocusFunction = DefaultCanFocusFunction;
		while (!(*CanFocusFunction)(*ActiveMenu, (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]))
		{
			MoveDown(*ActiveMenu, ActiveMenuEntryIndex);
			if (*ActiveMenuEntryIndex == Sentinel)
			{
				// If we went through all of them, we cannot focus anything.
				// Place the focus on the NULL entry.
				*ActiveMenuEntryIndex = FindNullEntry(*ActiveMenu);
				return;
			}
			CanFocusFunction = (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->CanFocusFunction;
			if (CanFocusFunction == NULL) CanFocusFunction = DefaultCanFocusFunction;
		}
	}
}

static void DefaultRightFunction(struct Menu* ActiveMenu, struct MenuEntry* ActiveMenuEntry)
{
	if (ActiveMenuEntry->Kind == KIND_OPTION
	|| (ActiveMenuEntry->Kind == KIND_CUSTOM /* chose to use this function */
	 && ActiveMenuEntry->Target != NULL)
	)
	{
		uint32_t* Target = (uint32_t*) ActiveMenuEntry->Target;
		(*Target)++;
		if (*Target >= ActiveMenuEntry->ChoiceCount)
			*Target = 0;
	}
}

static void DefaultLeftFunction(struct Menu* ActiveMenu, struct MenuEntry* ActiveMenuEntry)
{
	if (ActiveMenuEntry->Kind == KIND_OPTION
	|| (ActiveMenuEntry->Kind == KIND_CUSTOM /* chose to use this function */
	 && ActiveMenuEntry->Target != NULL)
	)
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

static void DefaultDisplayNameFunction(struct MenuEntry* DrawnMenuEntry, struct MenuEntry* ActiveMenuEntry, uint32_t Position)
{
	bool IsActive = (DrawnMenuEntry == ActiveMenuEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_TEXT : COLOR_INACTIVE_TEXT;
	uint16_t OutlineColor = IsActive ? COLOR_ACTIVE_OUTLINE : COLOR_INACTIVE_OUTLINE;
	PrintStringOutline(DrawnMenuEntry->Name, TextColor, OutlineColor, OutputSurface->pixels, OutputSurface->pitch, 0, GetRenderedHeight(" ") * (Position + 2), GCW0_SCREEN_WIDTH, GetRenderedHeight(" ") + 2, LEFT, TOP);
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

static void DefaultDisplayValueFunction(struct MenuEntry* DrawnMenuEntry, struct MenuEntry* ActiveMenuEntry, uint32_t Position)
{
	if (DrawnMenuEntry->Kind == KIND_OPTION || DrawnMenuEntry->Kind == KIND_DISPLAY)
	{
		char Temp[21];
		Temp[0] = '\0';
		char* Value = Temp;
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
					break;
				case TYPE_UINT32:
					sprintf(Temp, "%" PRIu32, *(uint32_t*) DrawnMenuEntry->Target);
					break;
				case TYPE_INT64:
					print_i64(Temp, *(int64_t*) DrawnMenuEntry->Target);
					break;
				case TYPE_UINT64:
					print_u64(Temp, *(uint64_t*) DrawnMenuEntry->Target);
					break;
				default:
					Value = "Unknown type";
					Error = true;
					break;
			}
		}
		bool IsActive = (DrawnMenuEntry == ActiveMenuEntry);
		uint16_t TextColor = Error ? COLOR_ERROR_TEXT : (IsActive ? COLOR_ACTIVE_TEXT : COLOR_INACTIVE_TEXT);
		uint16_t OutlineColor = Error ? COLOR_ERROR_OUTLINE : (IsActive ? COLOR_ACTIVE_OUTLINE : COLOR_INACTIVE_OUTLINE);
		PrintStringOutline(Value, TextColor, OutlineColor, OutputSurface->pixels, OutputSurface->pitch, 0, GetRenderedHeight(" ") * (Position + 2), GCW0_SCREEN_WIDTH, GetRenderedHeight(" ") + 2, RIGHT, TOP);
	}
}

static void DefaultDisplayBackgroundFunction(struct Menu* ActiveMenu)
{
	if (SDL_MUSTLOCK(OutputSurface))
		SDL_UnlockSurface(OutputSurface);
	SDL_FillRect(OutputSurface, NULL, COLOR_BACKGROUND);
	if (SDL_MUSTLOCK(OutputSurface))
		SDL_LockSurface(OutputSurface);
}

static void DefaultDisplayDataFunction(struct Menu* ActiveMenu, struct MenuEntry* ActiveMenuEntry)
{
	uint32_t DrawnMenuEntryIndex = 0;
	struct MenuEntry* DrawnMenuEntry = ActiveMenu->Entries[0];
	for (; DrawnMenuEntry != NULL; DrawnMenuEntryIndex++, DrawnMenuEntry = ActiveMenu->Entries[DrawnMenuEntryIndex])
	{
		MenuEntryDisplayFunction Function = DrawnMenuEntry->DisplayNameFunction;
		if (Function == NULL) Function = &DefaultDisplayNameFunction;
		(*Function)(DrawnMenuEntry, ActiveMenuEntry, DrawnMenuEntryIndex);

		Function = DrawnMenuEntry->DisplayValueFunction;
		if (Function == NULL) Function = &DefaultDisplayValueFunction;
		(*Function)(DrawnMenuEntry, ActiveMenuEntry, DrawnMenuEntryIndex);

		DrawnMenuEntry++;
	}
}

static void DefaultDisplayTitleFunction(struct Menu* ActiveMenu)
{
	PrintStringOutline(ActiveMenu->Title, COLOR_TITLE_TEXT, COLOR_TITLE_OUTLINE, OutputSurface->pixels, OutputSurface->pitch, 0, 0, GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT, CENTER, TOP);
}

static void DisplayPerGameTitleFunction(struct Menu* ActiveMenu)
{
	PrintStringOutline(ActiveMenu->Title, COLOR_TITLE_TEXT, COLOR_TITLE_OUTLINE, OutputSurface->pixels, OutputSurface->pitch, 0, 0, GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT, CENTER, TOP);
	char ForGame[MAX_PATH * 2];
	char FileNameNoExt[MAX_PATH + 1];
	GetFileNameNoExtension(FileNameNoExt, CurrentGamePath);
	sprintf(ForGame, "for %s", FileNameNoExt);
	PrintStringOutline(ForGame, COLOR_TITLE_TEXT, COLOR_TITLE_OUTLINE, OutputSurface->pixels, OutputSurface->pitch, 0, GetRenderedHeight(" "), GCW0_SCREEN_WIDTH, GetRenderedHeight(" ") + 2, CENTER, TOP);
}

void DefaultLoadFunction(struct MenuEntry* ActiveMenuEntry, char* Value)
{
	uint32_t i;
	for (i = 0; i < ActiveMenuEntry->ChoiceCount; i++)
	{
		if (strcasecmp(ActiveMenuEntry->Choices[i].Persistent, Value) == 0)
		{
			*(uint32_t*) ActiveMenuEntry->Target = i;
			return;
		}
	}
	ReGBA_Trace("W: Value '%s' for option '%s' not valid; ignored", Value, ActiveMenuEntry->PersistentName);
}

void DefaultSaveFunction(struct MenuEntry* ActiveMenuEntry, char* Value)
{
	snprintf(Value, 256, "%s = %s #%s\n", ActiveMenuEntry->PersistentName,
		ActiveMenuEntry->Choices[*((uint32_t*) ActiveMenuEntry->Target)].Persistent,
		ActiveMenuEntry->Choices[*((uint32_t*) ActiveMenuEntry->Target)].Pretty);
}

// -- Custom initialisation/finalisation --

static void SavedStateMenuInit(struct Menu** ActiveMenu)
{
	(*ActiveMenu)->UserData = calloc(GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT, sizeof(uint16_t));
	if ((*ActiveMenu)->UserData == NULL)
	{
		ReGBA_Trace("E: Memory allocation error while entering the Saved States menu");
		*ActiveMenu = (*ActiveMenu)->Parent;
	}
	else
		SavedStateUpdatePreview(*ActiveMenu);
}

static void SavedStateMenuEnd(struct Menu* ActiveMenu)
{
	if (ActiveMenu->UserData != NULL)
	{
		free(ActiveMenu->UserData);
		ActiveMenu->UserData = NULL;
	}
}

// -- Custom display --

static char* OpenDinguxButtonText[OPENDINGUX_BUTTON_COUNT] = {
	"L",
	"R",
	"D-pad Down",
	"D-pad Up",
	"D-pad Left",
	"D-pad Right",
	"Start",
	"Select",
	"B",
	"A",
	LEFT_FACE_BUTTON_NAME,
	TOP_FACE_BUTTON_NAME,
	"Analog Down",
	"Analog Up",
	"Analog Left",
	"Analog Right",
};

/*
 * Retrieves the button text for a single OpenDingux button.
 * Input:
 *   Button: The single button to describe. If this value is 0, the value is
 *   considered valid and "None" is the description text.
 * Output:
 *   Valid: A pointer to a Boolean variable which is updated with true if
 *     Button was a single button or none, or false otherwise.
 * Returns:
 *   A pointer to a null-terminated string describing Button. This string must
 *   never be freed, as it is statically allocated.
 */
static char* GetButtonText(enum OpenDingux_Buttons Button, bool* Valid)
{
	uint_fast8_t i;
	if (Button == 0)
	{
		*Valid = true;
		return "None";
	}
	else
	{
		for (i = 0; i < OPENDINGUX_BUTTON_COUNT; i++)
		{
			if (Button == 1 << i)
			{
				*Valid = true;
				return OpenDinguxButtonText[i];
			}
		}
		*Valid = false;
		return "Invalid";
	}
}

/*
 * Retrieves the button text for an OpenDingux button combination.
 * Input:
 *   Button: The buttons to describe. If this value is 0, the description text
 *   is "None". If there are multiple buttons in the bitfield, they are all
 *   added, separated by a '+' character.
 * Output:
 *   Result: A pointer to a buffer which is updated with the description of
 *   the button combination.
 */
static void GetButtonsText(enum OpenDingux_Buttons Buttons, char* Result)
{
	uint_fast8_t i;
	if (Buttons == 0)
	{
		strcpy(Result, "None");
	}
	else
	{
		Result[0] = '\0';
		bool AfterFirst = false;
		for (i = 0; i < OPENDINGUX_BUTTON_COUNT; i++)
		{
			if ((Buttons & 1 << i) != 0)
			{
				if (AfterFirst)
					strcat(Result, "+");
				AfterFirst = true;
				strcat(Result, OpenDinguxButtonText[i]);
			}
		}
	}
}

static void DisplayButtonMappingValue(struct MenuEntry* DrawnMenuEntry, struct MenuEntry* ActiveMenuEntry, uint32_t Position)
{
	bool Valid;
	char* Value = GetButtonText(*(uint32_t*) DrawnMenuEntry->Target, &Valid);

	bool IsActive = (DrawnMenuEntry == ActiveMenuEntry);
	uint16_t TextColor = Valid ? (IsActive ? COLOR_ACTIVE_TEXT : COLOR_INACTIVE_TEXT) : COLOR_ERROR_TEXT;
	uint16_t OutlineColor = Valid ? (IsActive ? COLOR_ACTIVE_OUTLINE : COLOR_INACTIVE_OUTLINE) : COLOR_ERROR_OUTLINE;
	PrintStringOutline(Value, TextColor, OutlineColor, OutputSurface->pixels, OutputSurface->pitch, 0, GetRenderedHeight(" ") * (Position + 2), GCW0_SCREEN_WIDTH, GetRenderedHeight(" ") + 2, RIGHT, TOP);
}

static void DisplayHotkeyValue(struct MenuEntry* DrawnMenuEntry, struct MenuEntry* ActiveMenuEntry, uint32_t Position)
{
	char Value[256];
	GetButtonsText(*(uint32_t*) DrawnMenuEntry->Target, Value);

	bool IsActive = (DrawnMenuEntry == ActiveMenuEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_TEXT : COLOR_INACTIVE_TEXT;
	uint16_t OutlineColor = IsActive ? COLOR_ACTIVE_OUTLINE : COLOR_INACTIVE_OUTLINE;
	PrintStringOutline(Value, TextColor, OutlineColor, OutputSurface->pixels, OutputSurface->pitch, 0, GetRenderedHeight(" ") * (Position + 2), GCW0_SCREEN_WIDTH, GetRenderedHeight(" ") + 2, RIGHT, TOP);
}

static void DisplayErrorBackgroundFunction(struct Menu* ActiveMenu)
{
	if (SDL_MUSTLOCK(OutputSurface))
		SDL_UnlockSurface(OutputSurface);
	SDL_FillRect(OutputSurface, NULL, COLOR_ERROR_BACKGROUND);
	if (SDL_MUSTLOCK(OutputSurface))
		SDL_LockSurface(OutputSurface);
}

static void SavedStateMenuDisplayData(struct Menu* ActiveMenu, struct MenuEntry* ActiveMenuEntry)
{
	PrintStringOutline("Preview", COLOR_INACTIVE_TEXT, COLOR_INACTIVE_OUTLINE, OutputSurface->pixels, OutputSurface->pitch, GCW0_SCREEN_WIDTH - GBA_SCREEN_WIDTH / 2, GetRenderedHeight(" ") * 2, GBA_SCREEN_WIDTH / 2, GetRenderedHeight(" ") + 2, LEFT, TOP);

	gba_render_half((uint16_t*) OutputSurface->pixels, (uint16_t*) ActiveMenu->UserData,
		GCW0_SCREEN_WIDTH - GBA_SCREEN_WIDTH / 2,
		GetRenderedHeight(" ") * 3 + 1,
		GBA_SCREEN_WIDTH * sizeof(uint16_t),
		OutputSurface->pitch);

	DefaultDisplayDataFunction(ActiveMenu, ActiveMenuEntry);
}

static void SavedStateSelectionDisplayValue(struct MenuEntry* DrawnMenuEntry, struct MenuEntry* ActiveMenuEntry, uint32_t Position)
{
	char Value[11];
	sprintf(Value, "%" PRIu32, *(uint32_t*) DrawnMenuEntry->Target + 1);

	bool IsActive = (DrawnMenuEntry == ActiveMenuEntry);
	uint16_t TextColor = IsActive ? COLOR_ACTIVE_TEXT : COLOR_INACTIVE_TEXT;
	uint16_t OutlineColor = IsActive ? COLOR_ACTIVE_OUTLINE : COLOR_INACTIVE_OUTLINE;
	PrintStringOutline(Value, TextColor, OutlineColor, OutputSurface->pixels, OutputSurface->pitch, 0, GetRenderedHeight(" ") * (Position + 2), GCW0_SCREEN_WIDTH - GBA_SCREEN_WIDTH / 2 - 16, GetRenderedHeight(" ") + 2, RIGHT, TOP);
}

static void SavedStateUpdatePreview(struct Menu* ActiveMenu)
{
	memset(ActiveMenu->UserData, 0, GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT * sizeof(u16));
	char SavedStateFilename[MAX_PATH + 1];
	if (!ReGBA_GetSavedStateFilename(SavedStateFilename, CurrentGamePath, SelectedState))
	{
		ReGBA_Trace("W: Failed to get the name of saved state #%d for '%s'", SelectedState, CurrentGamePath);
		return;
	}

	FILE_TAG_TYPE fp;
	FILE_OPEN(fp, SavedStateFilename, READ);
	if (!FILE_CHECK_VALID(fp))
		return;

	FILE_SEEK(fp, SVS_HEADER_SIZE + sizeof(struct ReGBA_RTC), SEEK_SET);

	FILE_READ(fp, ActiveMenu->UserData, GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT * sizeof(u16));

	FILE_CLOSE(fp);
}

// -- Custom saving --

static char OpenDinguxButtonSave[OPENDINGUX_BUTTON_COUNT] = {
	'L',
	'R',
	'v', // D-pad directions.
	'^',
	'<',
	'>', // (end)
	'S',
	's',
	'B',
	'A',
	'Y', // Using the SNES/DS/A320 mapping, this is the left face button.
	'X', // Using the SNES/DS/A320 mapping, this is the upper face button.
	'd', // Analog nub directions (GCW Zero).
	'u',
	'l',
	'r', // (end)
};

static void LoadMappingFunction(struct MenuEntry* ActiveMenuEntry, char* Value)
{
	uint32_t Mapping = 0;
	if (Value[0] != 'x')
	{
		uint_fast8_t i;
		for (i = 0; i < OPENDINGUX_BUTTON_COUNT; i++)
			if (Value[0] == OpenDinguxButtonSave[i])
			{
				Mapping = 1 << i;
				break;
			}
	}
	*(uint32_t*) ActiveMenuEntry->Target = Mapping;
}

static void SaveMappingFunction(struct MenuEntry* ActiveMenuEntry, char* Value)
{
	char Temp[32];
	Temp[0] = '\0';
	uint_fast8_t i;
	for (i = 0; i < OPENDINGUX_BUTTON_COUNT; i++)
		if (*(uint32_t*) ActiveMenuEntry->Target == 1 << i)
		{
			Temp[0] = OpenDinguxButtonSave[i];
			sprintf(&Temp[1], " #%s", OpenDinguxButtonText[i]);
			break;
		}
	if (Temp[0] == '\0')
		strcpy(Temp, "x #None");
	snprintf(Value, 256, "%s = %s\n", ActiveMenuEntry->PersistentName, Temp);
}

static void LoadHotkeyFunction(struct MenuEntry* ActiveMenuEntry, char* Value)
{
	uint32_t Hotkey = 0;
	if (Value[0] != 'x')
	{
		char* Ptr = Value;
		while (*Ptr)
		{
			uint_fast8_t i;
			for (i = 0; i < OPENDINGUX_BUTTON_COUNT; i++)
				if (*Ptr == OpenDinguxButtonSave[i])
				{
					Hotkey |= 1 << i;
					break;
				}
			Ptr++;
		}
	}
	*(uint32_t*) ActiveMenuEntry->Target = Hotkey;
}

static void SaveHotkeyFunction(struct MenuEntry* ActiveMenuEntry, char* Value)
{
	char Temp[192];
	char* Ptr = Temp;
	uint_fast8_t i;
	for (i = 0; i < OPENDINGUX_BUTTON_COUNT; i++)
		if ((*(uint32_t*) ActiveMenuEntry->Target & (1 << i)) != 0)
		{
			*Ptr++ = OpenDinguxButtonSave[i];
		}
	if (Ptr == Temp)
		strcpy(Temp, "x #None");
	else
	{
		*Ptr++ = ' ';
		*Ptr++ = '#';
		GetButtonsText(*(uint32_t*) ActiveMenuEntry->Target, Ptr);
	}
	snprintf(Value, 256, "%s = %s\n", ActiveMenuEntry->PersistentName, Temp);
}

// -- Custom actions --

static void ActionExit(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	// Please ensure that the Main Menu itself does not have entries of type
	// KIND_OPTION. The on-demand writing of settings to storage will not
	// occur after quit(), since it acts after the action function returns.
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

static void NullLeftFunction(struct Menu* ActiveMenu, struct MenuEntry* ActiveMenuEntry)
{
}

static void NullRightFunction(struct Menu* ActiveMenu, struct MenuEntry* ActiveMenuEntry)
{
}

static enum OpenDingux_Buttons GrabButton(struct Menu* ActiveMenu, char* Text)
{
	MenuFunction DisplayBackgroundFunction = ActiveMenu->DisplayBackgroundFunction;
	if (DisplayBackgroundFunction == NULL) DisplayBackgroundFunction = DefaultDisplayBackgroundFunction;
	enum OpenDingux_Buttons Buttons;
	// Wait for the buttons that triggered the action to be released.
	while (GetPressedOpenDinguxButtons() != 0)
	{
		(*DisplayBackgroundFunction)(ActiveMenu);
		ReGBA_VideoFlip();
		usleep(5000); // for platforms that don't sync their flips
	}
	// Wait until a button is pressed.
	while ((Buttons = GetPressedOpenDinguxButtons()) == 0)
	{
		(*DisplayBackgroundFunction)(ActiveMenu);
		PrintStringOutline(Text, COLOR_ACTIVE_TEXT, COLOR_ACTIVE_OUTLINE, OutputSurface->pixels, OutputSurface->pitch, 0, 0, GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT, CENTER, MIDDLE);
		ReGBA_VideoFlip();
		usleep(5000); // for platforms that don't sync their flips
	}
	// Accumulate buttons until they're all released.
	enum OpenDingux_Buttons ButtonTotal = Buttons;
	while ((Buttons = GetPressedOpenDinguxButtons()) != 0)
	{
		ButtonTotal |= Buttons;
		(*DisplayBackgroundFunction)(ActiveMenu);
		PrintStringOutline(Text, COLOR_ACTIVE_TEXT, COLOR_ACTIVE_OUTLINE, OutputSurface->pixels, OutputSurface->pitch, 0, 0, GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT, CENTER, MIDDLE);
		ReGBA_VideoFlip();
		usleep(5000); // for platforms that don't sync their flips
	}
	return ButtonTotal;
}

static enum OpenDingux_Buttons GrabButtons(struct Menu* ActiveMenu, char* Text)
{
	MenuFunction DisplayBackgroundFunction = ActiveMenu->DisplayBackgroundFunction;
	if (DisplayBackgroundFunction == NULL) DisplayBackgroundFunction = DefaultDisplayBackgroundFunction;
	enum OpenDingux_Buttons Buttons;
	// Wait for the buttons that triggered the action to be released.
	while (GetPressedOpenDinguxButtons() != 0)
	{
		(*DisplayBackgroundFunction)(ActiveMenu);
		ReGBA_VideoFlip();
		usleep(5000); // for platforms that don't sync their flips
	}
	// Wait until a button is pressed.
	while ((Buttons = GetPressedOpenDinguxButtons()) == 0)
	{
		(*DisplayBackgroundFunction)(ActiveMenu);
		PrintStringOutline(Text, COLOR_ACTIVE_TEXT, COLOR_ACTIVE_OUTLINE, OutputSurface->pixels, OutputSurface->pitch, 0, 0, GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT, CENTER, MIDDLE);
		ReGBA_VideoFlip();
		usleep(5000); // for platforms that don't sync their flips
	}
	// Accumulate buttons until they're all released.
	enum OpenDingux_Buttons ButtonTotal = Buttons;
	while ((Buttons = GetPressedOpenDinguxButtons()) != 0)
	{
		// a) If the old buttons are a strict subset of the new buttons,
		//    add the new buttons.
		if ((Buttons | ButtonTotal) == Buttons)
			ButtonTotal |= Buttons;
		// b) If the new buttons are a strict subset of the old buttons,
		//    do nothing. (The user is releasing the buttons to return.)
		else if ((Buttons | ButtonTotal) == ButtonTotal)
			;
		// c) If the new buttons are on another path, replace the buttons
		//    completely, for example, R+X turning into R+Y.
		else
			ButtonTotal = Buttons;
		(*DisplayBackgroundFunction)(ActiveMenu);
		PrintStringOutline(Text, COLOR_ACTIVE_TEXT, COLOR_ACTIVE_OUTLINE, OutputSurface->pixels, OutputSurface->pitch, 0, 0, GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT, CENTER, MIDDLE);
		ReGBA_VideoFlip();
		usleep(5000); // for platforms that don't sync their flips
	}
	return ButtonTotal;
}

void ShowErrorScreen(const char* Format, ...)
{
	char* line = malloc(82);
	va_list args;
	int linelen;

	va_start(args, Format);
	if ((linelen = vsnprintf(line, 82, Format, args)) >= 82)
	{
		va_end(args);
		va_start(args, Format);
		free(line);
		line = malloc(linelen + 1);
		vsnprintf(line, linelen + 1, Format, args);
	}
	va_end(args);
	GrabButton(&ErrorScreen, line);
	free(line);
}

static enum OpenDingux_Buttons GrabYesOrNo(struct Menu* ActiveMenu, char* Text)
{
	return GrabButtons(ActiveMenu, Text) == OPENDINGUX_BUTTON_FACE_RIGHT;
}

static void ActionSetMapping(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	char Text[256];
	bool Valid;
	sprintf(Text, "Setting binding for %s\nCurrently %s\n"
		"Press the new button or two at once to leave the binding alone.",
		(*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Name,
		GetButtonText(*(uint32_t*) (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Target, &Valid));

	enum OpenDingux_Buttons ButtonTotal = GrabButton(*ActiveMenu, Text);
	// If there's more than one button, change nothing.
	uint_fast8_t BitCount = 0, i;
	for (i = 0; i < OPENDINGUX_BUTTON_COUNT; i++)
		if ((ButtonTotal & (1 << i)) != 0)
			BitCount++;
	if (BitCount == 1)
		*(uint32_t*) (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Target = ButtonTotal;
}

static void ActionSetOrClearMapping(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	char Text[256];
	bool Valid;
	sprintf(Text, "Setting binding for %s\nCurrently %s\n"
		"Press the new button or two at once to clear the binding.",
		(*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Name,
		GetButtonText(*(uint32_t*) (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Target, &Valid));

	enum OpenDingux_Buttons ButtonTotal = GrabButton(*ActiveMenu, Text);
	// If there's more than one button, clear the mapping.
	uint_fast8_t BitCount = 0, i;
	for (i = 0; i < OPENDINGUX_BUTTON_COUNT; i++)
		if ((ButtonTotal & (1 << i)) != 0)
			BitCount++;
	*(uint32_t*) (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Target = (BitCount == 1)
		? ButtonTotal
		: 0;
}

static void ActionSetHotkey(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	char Text[256];
	char Current[256];
	GetButtonsText(*(uint32_t*) (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Target, Current);
	sprintf(Text, "Setting hotkey binding for %s\nCurrently %s\n"
		"Press the new buttons or B to leave the hotkey binding alone.",
		(*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Name,
		Current);

	enum OpenDingux_Buttons ButtonTotal = GrabButtons(*ActiveMenu, Text);
	if (ButtonTotal != OPENDINGUX_BUTTON_FACE_DOWN)
		*(uint32_t*) (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Target = ButtonTotal;
}

static void ActionSetOrClearHotkey(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	char Text[256];
	char Current[256];
	GetButtonsText(*(uint32_t*) (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Target, Current);
	sprintf(Text, "Setting hotkey binding for %s\nCurrently %s\n"
		"Press the new buttons or B to clear the hotkey binding.",
		(*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Name,
		Current);

	enum OpenDingux_Buttons ButtonTotal = GrabButtons(*ActiveMenu, Text);
	*(uint32_t*) (*ActiveMenu)->Entries[*ActiveMenuEntryIndex]->Target = (ButtonTotal == OPENDINGUX_BUTTON_FACE_DOWN)
		? 0
		: ButtonTotal;
}

static void SavedStateSelectionLeft(struct Menu* ActiveMenu, struct MenuEntry* ActiveMenuEntry)
{
	DefaultLeftFunction(ActiveMenu, ActiveMenuEntry);
	SavedStateUpdatePreview(ActiveMenu);
}

static void SavedStateSelectionRight(struct Menu* ActiveMenu, struct MenuEntry* ActiveMenuEntry)
{
	DefaultRightFunction(ActiveMenu, ActiveMenuEntry);
	SavedStateUpdatePreview(ActiveMenu);
}

static void ActionSavedStateRead(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	switch (load_state(SelectedState))
	{
		case 0:
			*ActiveMenu = NULL;
			break;

		case 1:
			if (errno != 0)
				ShowErrorScreen("Reading saved state #%" PRIu32 " failed:\n%s", SelectedState + 1, strerror(errno));
			else
				ShowErrorScreen("Reading saved state #%" PRIu32 " failed:\nIncomplete file", SelectedState + 1);
			break;

		case 2:
			ShowErrorScreen("Reading saved state #%" PRIu32 " failed:\nFile format invalid", SelectedState + 1);
			break;
	}
}

static void ActionSavedStateWrite(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	// 1. If the file already exists, ask the user if s/he wants to overwrite it.
	char SavedStateFilename[MAX_PATH + 1];
	if (!ReGBA_GetSavedStateFilename(SavedStateFilename, CurrentGamePath, SelectedState))
	{
		ReGBA_Trace("W: Failed to get the name of saved state #%d for '%s'", SelectedState, CurrentGamePath);
		ShowErrorScreen("Preparing to write saved state #%" PRIu32 " failed:\nPath too long", SelectedState + 1);
		return;
	}
	FILE_TAG_TYPE dummy;
	FILE_OPEN(dummy, SavedStateFilename, READ);
	if (FILE_CHECK_VALID(dummy))
	{
		FILE_CLOSE(dummy);
		char Temp[1024];
		sprintf(Temp, "Do you want to overwrite saved state #%" PRIu32 "?\n[A] = Yes  Others = No", SelectedState + 1);
		if (!GrabYesOrNo(*ActiveMenu, Temp))
			return;
	}
	
	// 2. If the file didn't exist or the user wanted to overwrite it, save.
	uint32_t ret = save_state(SelectedState, MainMenu.UserData /* preserved screenshot */);
	if (ret != 1)
	{
		if (errno != 0)
			ShowErrorScreen("Writing saved state #%" PRIu32 " failed:\n%s", SelectedState + 1, strerror(errno));
		else
			ShowErrorScreen("Writing saved state #%" PRIu32 " failed:\nUnknown error", SelectedState + 1);
	}
	SavedStateUpdatePreview(*ActiveMenu);
}

static void ActionSavedStateDelete(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	// 1. Ask the user if s/he wants to delete this saved state.
	char SavedStateFilename[MAX_PATH + 1];
	if (!ReGBA_GetSavedStateFilename(SavedStateFilename, CurrentGamePath, SelectedState))
	{
		ReGBA_Trace("W: Failed to get the name of saved state #%d for '%s'", SelectedState, CurrentGamePath);
		ShowErrorScreen("Preparing to delete saved state #%" PRIu32 " failed:\nPath too long", SelectedState + 1);
		return;
	}
	char Temp[1024];
	sprintf(Temp, "Do you want to delete saved state #%" PRIu32 "?\n[A] = Yes  Others = No", SelectedState + 1);
	if (!GrabYesOrNo(*ActiveMenu, Temp))
		return;
	
	// 2. If the user wants to, delete the saved state.
	if (remove(SavedStateFilename) != 0)
	{
		ShowErrorScreen("Deleting saved state #%" PRIu32 " failed:\n%s", SelectedState + 1, strerror(errno));
	}
	SavedStateUpdatePreview(*ActiveMenu);
}

static void ActionShowVersion(struct Menu** ActiveMenu, uint32_t* ActiveMenuEntryIndex)
{
	char Text[1024];
#ifdef GIT_VERSION_STRING
	sprintf(Text, "ReGBA version %s\nNebuleon/ReGBA commit %s", REGBA_VERSION_STRING, GIT_VERSION_STRING);
#else
	sprintf(Text, "ReGBA version %s", REGBA_VERSION_STRING);
#endif
	GrabButtons(*ActiveMenu, Text);
}

// -- Strut --

static bool CanNeverFocusFunction(struct Menu* ActiveMenu, struct MenuEntry* ActiveMenuEntry)
{
	return false;
}

static struct MenuEntry Strut = {
	.Kind = KIND_CUSTOM, .Name = "",
	.CanFocusFunction = CanNeverFocusFunction
};

// -- Debug > Native code stats --

static struct MenuEntry NativeCodeMenu_ROPeak = {
	ENTRY_DISPLAY("Read-only bytes at peak", &Stats.TranslationBytesPeak[TRANSLATION_REGION_READONLY], TYPE_UINT64)
};

static struct MenuEntry NativeCodeMenu_RWPeak = {
	ENTRY_DISPLAY("Writable bytes at peak", &Stats.TranslationBytesPeak[TRANSLATION_REGION_WRITABLE], TYPE_UINT64)
};

static struct MenuEntry NativeCodeMenu_ROFlushed = {
	ENTRY_DISPLAY("Read-only bytes flushed", &Stats.TranslationBytesFlushed[TRANSLATION_REGION_READONLY], TYPE_UINT64)
};

static struct MenuEntry NativeCodeMenu_RWFlushed = {
	ENTRY_DISPLAY("Writable bytes flushed", &Stats.TranslationBytesFlushed[TRANSLATION_REGION_WRITABLE], TYPE_UINT64)
};

static struct Menu NativeCodeMenu = {
	.Parent = &DebugMenu, .Title = "Native code statistics",
	.Entries = { &NativeCodeMenu_ROPeak, &NativeCodeMenu_RWPeak, &NativeCodeMenu_ROFlushed, &NativeCodeMenu_RWFlushed, NULL }
};

static struct MenuEntry DebugMenu_NativeCode = {
	ENTRY_SUBMENU("Native code statistics...", &NativeCodeMenu)
};

// -- Debug > Metadata stats --

static struct MenuEntry MetadataMenu_ROFull = {
	ENTRY_DISPLAY("Read-only area full", &Stats.TranslationFlushCount[TRANSLATION_REGION_READONLY][FLUSH_REASON_FULL_CACHE], TYPE_UINT64)
};

static struct MenuEntry MetadataMenu_RWFull = {
	ENTRY_DISPLAY("Writable area full", &Stats.TranslationFlushCount[TRANSLATION_REGION_WRITABLE][FLUSH_REASON_FULL_CACHE], TYPE_UINT64)
};

static struct MenuEntry MetadataMenu_BIOSLastTag = {
	ENTRY_DISPLAY("BIOS tags full", &Stats.MetadataClearCount[METADATA_AREA_BIOS][CLEAR_REASON_LAST_TAG], TYPE_UINT64)
};

static struct MenuEntry MetadataMenu_EWRAMLastTag = {
	ENTRY_DISPLAY("EWRAM tags full", &Stats.MetadataClearCount[METADATA_AREA_EWRAM][CLEAR_REASON_LAST_TAG], TYPE_UINT64)
};

static struct MenuEntry MetadataMenu_IWRAMLastTag = {
	ENTRY_DISPLAY("IWRAM tags full", &Stats.MetadataClearCount[METADATA_AREA_IWRAM][CLEAR_REASON_LAST_TAG], TYPE_UINT64)
};

static struct MenuEntry MetadataMenu_VRAMLastTag = {
	ENTRY_DISPLAY("VRAM tags full", &Stats.MetadataClearCount[METADATA_AREA_VRAM][CLEAR_REASON_LAST_TAG], TYPE_UINT64)
};

static struct MenuEntry MetadataMenu_PartialClears = {
	ENTRY_DISPLAY("Partial clears", &Stats.PartialFlushCount, TYPE_UINT64)
};

static struct Menu MetadataMenu = {
	.Parent = &DebugMenu, .Title = "Metadata clear statistics",
	.Entries = { &MetadataMenu_ROFull, &MetadataMenu_RWFull, &MetadataMenu_BIOSLastTag, &MetadataMenu_EWRAMLastTag, &MetadataMenu_IWRAMLastTag, &MetadataMenu_VRAMLastTag, &MetadataMenu_PartialClears, NULL }
};

static struct MenuEntry DebugMenu_Metadata = {
	ENTRY_SUBMENU("Metadata clear statistics...", &MetadataMenu)
};

// -- Debug > Execution stats --

static struct MenuEntry ExecutionMenu_SoundUnderruns = {
	ENTRY_DISPLAY("Sound buffer underruns", &Stats.SoundBufferUnderrunCount, TYPE_UINT64)
};

static struct MenuEntry ExecutionMenu_FramesEmulated = {
	ENTRY_DISPLAY("Frames emulated", &Stats.TotalEmulatedFrames, TYPE_UINT64)
};

static struct MenuEntry ExecutionMenu_FramesRendered = {
	ENTRY_DISPLAY("Frames rendered", &Stats.TotalRenderedFrames, TYPE_UINT64)
};

#ifdef PERFORMANCE_IMPACTING_STATISTICS
static struct MenuEntry ExecutionMenu_ARMOps = {
	ENTRY_DISPLAY("ARM opcodes decoded", &Stats.ARMOpcodesDecoded, TYPE_UINT64)
};

static struct MenuEntry ExecutionMenu_ThumbOps = {
	ENTRY_DISPLAY("Thumb opcodes decoded", &Stats.ThumbOpcodesDecoded, TYPE_UINT64)
};

static struct MenuEntry ExecutionMenu_MemAccessors = {
	ENTRY_DISPLAY("Memory accessors patched", &Stats.WrongAddressLineCount, TYPE_UINT32)
};
#endif

static struct Menu ExecutionMenu = {
	.Parent = &DebugMenu, .Title = "Execution statistics",
	.Entries = { &ExecutionMenu_SoundUnderruns, &ExecutionMenu_FramesEmulated, &ExecutionMenu_FramesRendered
#ifdef PERFORMANCE_IMPACTING_STATISTICS
	, &ExecutionMenu_ARMOps, &ExecutionMenu_ThumbOps, &ExecutionMenu_MemAccessors
#endif
	, NULL }
};

static struct MenuEntry DebugMenu_Execution = {
	ENTRY_SUBMENU("Execution statistics...", &ExecutionMenu)
};

// -- Debug > Code reuse stats --

#ifdef PERFORMANCE_IMPACTING_STATISTICS
static struct MenuEntry ReuseMenu_OpsRecompiled = {
	ENTRY_DISPLAY("Opcodes recompiled", &Stats.OpcodeRecompilationCount, TYPE_UINT64)
};

static struct MenuEntry ReuseMenu_BlocksRecompiled = {
	ENTRY_DISPLAY("Blocks recompiled", &Stats.BlockRecompilationCount, TYPE_UINT64)
};

static struct MenuEntry ReuseMenu_OpsReused = {
	ENTRY_DISPLAY("Opcodes reused", &Stats.OpcodeReuseCount, TYPE_UINT64)
};

static struct MenuEntry ReuseMenu_BlocksReused = {
	ENTRY_DISPLAY("Blocks reused", &Stats.BlockReuseCount, TYPE_UINT64)
};

static struct Menu ReuseMenu = {
	.Parent = &DebugMenu, .Title = "Code reuse statistics",
	.Entries = { &ReuseMenu_OpsRecompiled, &ReuseMenu_BlocksRecompiled, &ReuseMenu_OpsReused, &ReuseMenu_BlocksReused, NULL }
};

static struct MenuEntry DebugMenu_Reuse = {
	ENTRY_SUBMENU("Code reuse statistics...", &ReuseMenu)
};
#endif

static struct MenuEntry ROMInfoMenu_GameName = {
	ENTRY_DISPLAY("game_name =", gamepak_title, TYPE_STRING)
};

static struct MenuEntry ROMInfoMenu_GameCode = {
	ENTRY_DISPLAY("game_code =", gamepak_code, TYPE_STRING)
};

static struct MenuEntry ROMInfoMenu_VendorCode = {
	ENTRY_DISPLAY("vender_code =", gamepak_maker, TYPE_STRING)
};

static struct Menu ROMInfoMenu = {
	.Parent = &DebugMenu, .Title = "ROM information",
	.Entries = { &ROMInfoMenu_GameName, &ROMInfoMenu_GameCode, &ROMInfoMenu_VendorCode, NULL }
};

static struct MenuEntry DebugMenu_ROMInfo = {
	ENTRY_SUBMENU("ROM information...", &ROMInfoMenu)
};

static struct MenuEntry DebugMenu_VersionInfo = {
	.Kind = KIND_CUSTOM, .Name = "ReGBA version information...",
	.ButtonEnterFunction = &ActionShowVersion
};

// -- Debug --

static struct Menu DebugMenu = {
	.Parent = &MainMenu, .Title = "Performance and debugging",
	.Entries = { &DebugMenu_NativeCode, &DebugMenu_Metadata, &DebugMenu_Execution
#ifdef PERFORMANCE_IMPACTING_STATISTICS
	, &DebugMenu_Reuse
#endif
	, &Strut, &DebugMenu_ROMInfo, &Strut, &DebugMenu_VersionInfo, NULL }
};

// -- Display Settings --

static struct MenuEntry PerGameDisplayMenu_BootSource = {
	ENTRY_OPTION("boot_from", "Boot from", &PerGameBootFromBIOS),
	.ChoiceCount = 3, .Choices = { { "No override", "" }, { "Cartridge ROM", "cartridge" }, { "GBA BIOS", "gba_bios" } }
};
static struct MenuEntry DisplayMenu_BootSource = {
	ENTRY_OPTION("boot_from", "Boot from", &BootFromBIOS),
	.ChoiceCount = 2, .Choices = { { "Cartridge ROM", "cartridge" }, { "GBA BIOS", "gba_bios" } }
};

static struct MenuEntry PerGameDisplayMenu_FPSCounter = {
	ENTRY_OPTION("fps_counter", "FPS counter", &PerGameShowFPS),
	.ChoiceCount = 3, .Choices = { { "No override", "" }, { "Hide", "hide" }, { "Show", "show" } }
};
static struct MenuEntry DisplayMenu_FPSCounter = {
	ENTRY_OPTION("fps_counter", "FPS counter", &ShowFPS),
	.ChoiceCount = 2, .Choices = { { "Hide", "hide" }, { "Show", "show" } }
};

static struct MenuEntry PerGameDisplayMenu_ScaleMode = {
	ENTRY_OPTION("image_size", "Image scaling", &PerGameScaleMode),
	.ChoiceCount = 9, .Choices = { { "No override", "" }, { "Aspect, fast", "aspect" }, { "Full, fast", "fullscreen" }, { "Aspect, bilinear", "aspect_bilinear" }, { "Full, bilinear", "fullscreen_bilinear" }, { "Aspect, sub-pixel", "aspect_subpixel" }, { "Full, sub-pixel", "fullscreen_subpixel" }, { "None", "original" }, { "Hardware", "hardware" } }
};
static struct MenuEntry DisplayMenu_ScaleMode = {
	ENTRY_OPTION("image_size", "Image scaling", &ScaleMode),
	.ChoiceCount = 8, .Choices = { { "Aspect, fast", "aspect" }, { "Full, fast", "fullscreen" }, { "Aspect, bilinear", "aspect_bilinear" }, { "Full, bilinear", "fullscreen_bilinear" }, { "Aspect, sub-pixel", "aspect_subpixel" }, { "Full, sub-pixel", "fullscreen_subpixel" }, { "None", "original" }, { "Hardware", "hardware" } }
};

static struct MenuEntry PerGameDisplayMenu_Frameskip = {
	ENTRY_OPTION("frameskip", "Frame skipping", &PerGameUserFrameskip),
	.ChoiceCount = 6, .Choices = { { "No override", "" }, { "Automatic", "auto" }, { "0 (~60 FPS)", "0" }, { "1 (~30 FPS)", "1" }, { "2 (~20 FPS)", "2" }, { "3 (~15 FPS)", "3" } }
};
static struct MenuEntry DisplayMenu_Frameskip = {
	ENTRY_OPTION("frameskip", "Frame skipping", &UserFrameskip),
	.ChoiceCount = 5, .Choices = { { "Automatic", "auto" }, { "0 (~60 FPS)", "0" }, { "1 (~30 FPS)", "1" }, { "2 (~20 FPS)", "2" }, { "3 (~15 FPS)", "3" } }
};

static struct MenuEntry PerGameDisplayMenu_FastForwardTarget = {
	ENTRY_OPTION("fast_forward_target", "Fast-forward target", &PerGameFastForwardTarget),
	.ChoiceCount = 6, .Choices = { { "No override", "" }, { "2x (~120 FPS)", "2" }, { "3x (~180 FPS)", "3" }, { "4x (~240 FPS)", "4" }, { "5x (~300 FPS)", "5" }, { "6x (~360 FPS)", "6" } }
};
static struct MenuEntry DisplayMenu_FastForwardTarget = {
	ENTRY_OPTION("fast_forward_target", "Fast-forward target", &FastForwardTarget),
	.ChoiceCount = 5, .Choices = { { "2x (~120 FPS)", "2" }, { "3x (~180 FPS)", "3" }, { "4x (~240 FPS)", "4" }, { "5x (~300 FPS)", "5" }, { "6x (~360 FPS)", "6" } }
};

static struct Menu PerGameDisplayMenu = {
	.Parent = &PerGameMainMenu, .Title = "Display settings",
	MENU_PER_GAME,
	.AlternateVersion = &DisplayMenu,
	.Entries = { &PerGameDisplayMenu_BootSource, &PerGameDisplayMenu_FPSCounter, &PerGameDisplayMenu_ScaleMode, &PerGameDisplayMenu_Frameskip, &PerGameDisplayMenu_FastForwardTarget, NULL }
};
static struct Menu DisplayMenu = {
	.Parent = &MainMenu, .Title = "Display settings",
	.AlternateVersion = &PerGameDisplayMenu,
	.Entries = { &DisplayMenu_BootSource, &DisplayMenu_FPSCounter, &DisplayMenu_ScaleMode, &DisplayMenu_Frameskip, &DisplayMenu_FastForwardTarget, NULL }
};

// -- Input Settings --
static struct MenuEntry PerGameInputMenu_A = {
	ENTRY_OPTION("gba_a", "GBA A", &PerGameKeypadRemapping[0]),
	ENTRY_OPTIONAL_MAPPING
};
static struct MenuEntry InputMenu_A = {
	ENTRY_OPTION("gba_a", "GBA A", &KeypadRemapping[0]),
	ENTRY_MANDATORY_MAPPING
};

static struct MenuEntry PerGameInputMenu_B = {
	ENTRY_OPTION("gba_b", "GBA B", &PerGameKeypadRemapping[1]),
	ENTRY_OPTIONAL_MAPPING
};
static struct MenuEntry InputMenu_B = {
	ENTRY_OPTION("gba_b", "GBA B", &KeypadRemapping[1]),
	ENTRY_MANDATORY_MAPPING
};

static struct MenuEntry PerGameInputMenu_Start = {
	ENTRY_OPTION("gba_start", "GBA Start", &PerGameKeypadRemapping[3]),
	ENTRY_OPTIONAL_MAPPING
};
static struct MenuEntry InputMenu_Start = {
	ENTRY_OPTION("gba_start", "GBA Start", &KeypadRemapping[3]),
	ENTRY_MANDATORY_MAPPING
};

static struct MenuEntry PerGameInputMenu_Select = {
	ENTRY_OPTION("gba_select", "GBA Select", &PerGameKeypadRemapping[2]),
	ENTRY_OPTIONAL_MAPPING
};
static struct MenuEntry InputMenu_Select = {
	ENTRY_OPTION("gba_select", "GBA Select", &KeypadRemapping[2]),
	ENTRY_MANDATORY_MAPPING
};

static struct MenuEntry PerGameInputMenu_L = {
	ENTRY_OPTION("gba_l", "GBA L", &PerGameKeypadRemapping[9]),
	ENTRY_OPTIONAL_MAPPING
};
static struct MenuEntry InputMenu_L = {
	ENTRY_OPTION("gba_l", "GBA L", &KeypadRemapping[9]),
	ENTRY_MANDATORY_MAPPING
};

static struct MenuEntry PerGameInputMenu_R = {
	ENTRY_OPTION("gba_r", "GBA R", &PerGameKeypadRemapping[8]),
	ENTRY_OPTIONAL_MAPPING
};
static struct MenuEntry InputMenu_R = {
	ENTRY_OPTION("gba_r", "GBA R", &KeypadRemapping[8]),
	ENTRY_MANDATORY_MAPPING
};

static struct MenuEntry PerGameInputMenu_RapidA = {
	ENTRY_OPTION("rapid_a", "Rapid-fire A", &PerGameKeypadRemapping[10]),
	ENTRY_OPTIONAL_MAPPING
};
static struct MenuEntry InputMenu_RapidA = {
	ENTRY_OPTION("rapid_a", "Rapid-fire A", &KeypadRemapping[10]),
	ENTRY_OPTIONAL_MAPPING
};

static struct MenuEntry PerGameInputMenu_RapidB = {
	ENTRY_OPTION("rapid_b", "Rapid-fire B", &PerGameKeypadRemapping[11]),
	ENTRY_OPTIONAL_MAPPING
};
static struct MenuEntry InputMenu_RapidB = {
	ENTRY_OPTION("rapid_b", "Rapid-fire B", &KeypadRemapping[11]),
	ENTRY_OPTIONAL_MAPPING
};

#ifdef GCW_ZERO
static struct MenuEntry PerGameInputMenu_AnalogSensitivity = {
	ENTRY_OPTION("analog_sensitivity", "Analog sensitivity", &PerGameAnalogSensitivity),
	.ChoiceCount = 6, .Choices = { { "No override", "" }, { "Very low", "lowest" }, { "Low", "low" }, { "Medium", "medium" }, { "High", "high" }, { "Very high", "highest" } }
};
static struct MenuEntry InputMenu_AnalogSensitivity = {
	ENTRY_OPTION("analog_sensitivity", "Analog sensitivity", &AnalogSensitivity),
	.ChoiceCount = 5, .Choices = { { "Very low", "lowest" }, { "Low", "low" }, { "Medium", "medium" }, { "High", "high" }, { "Very high", "highest" } }
};

static struct MenuEntry PerGameInputMenu_AnalogAction = {
	ENTRY_OPTION("analog_action", "Analog in-game binding", &PerGameAnalogAction),
	.ChoiceCount = 3, .Choices = { { "No override", "" }, { "None", "none" }, { "GBA D-pad", "dpad" } }
};
static struct MenuEntry InputMenu_AnalogAction = {
	ENTRY_OPTION("analog_action", "Analog in-game binding", &AnalogAction),
	.ChoiceCount = 2, .Choices = { { "None", "none" }, { "GBA D-pad", "dpad" } }
};
#endif

static struct Menu PerGameInputMenu = {
	.Parent = &PerGameMainMenu, .Title = "Input settings",
	MENU_PER_GAME,
	.AlternateVersion = &InputMenu,
	.Entries = { &PerGameInputMenu_A, &PerGameInputMenu_B, &PerGameInputMenu_Start, &PerGameInputMenu_Select, &PerGameInputMenu_L, &PerGameInputMenu_R, &PerGameInputMenu_RapidA, &PerGameInputMenu_RapidB
#ifdef GCW_ZERO
	, &Strut, &PerGameInputMenu_AnalogSensitivity, &PerGameInputMenu_AnalogAction
#endif
	, NULL }
};
static struct Menu InputMenu = {
	.Parent = &MainMenu, .Title = "Input settings",
	.AlternateVersion = &PerGameInputMenu,
	.Entries = { &InputMenu_A, &InputMenu_B, &InputMenu_Start, &InputMenu_Select, &InputMenu_L, &InputMenu_R, &InputMenu_RapidA, &InputMenu_RapidB
#ifdef GCW_ZERO
	, &Strut, &InputMenu_AnalogSensitivity, &InputMenu_AnalogAction
#endif
	, NULL }
};

// -- Hotkeys --

static struct MenuEntry PerGameHotkeyMenu_FastForward = {
	ENTRY_OPTION("hotkey_fast_forward", "Fast-forward while held", &PerGameHotkeys[0]),
	ENTRY_OPTIONAL_HOTKEY
};
static struct MenuEntry HotkeyMenu_FastForward = {
	ENTRY_OPTION("hotkey_fast_forward", "Fast-forward while held", &Hotkeys[0]),
	ENTRY_OPTIONAL_HOTKEY
};

#if !defined GCW_ZERO
static struct MenuEntry HotkeyMenu_Menu = {
	ENTRY_OPTION("hotkey_menu", "Menu", &Hotkeys[1]),
	ENTRY_MANDATORY_HOTKEY
};
#endif

static struct MenuEntry PerGameHotkeyMenu_FastForwardToggle = {
	ENTRY_OPTION("hotkey_fast_forward_toggle", "Fast-forward toggle", &PerGameHotkeys[2]),
	ENTRY_OPTIONAL_HOTKEY
};
static struct MenuEntry HotkeyMenu_FastForwardToggle = {
	ENTRY_OPTION("hotkey_fast_forward_toggle", "Fast-forward toggle", &Hotkeys[2]),
	ENTRY_OPTIONAL_HOTKEY
};

static struct MenuEntry PerGameHotkeyMenu_QuickLoadState = {
	ENTRY_OPTION("hotkey_quick_load_state", "Quick load state #1", &PerGameHotkeys[3]),
	ENTRY_OPTIONAL_HOTKEY
};
static struct MenuEntry HotkeyMenu_QuickLoadState = {
	ENTRY_OPTION("hotkey_quick_load_state", "Quick load state #1", &Hotkeys[3]),
	ENTRY_OPTIONAL_HOTKEY
};

static struct MenuEntry PerGameHotkeyMenu_QuickSaveState = {
	ENTRY_OPTION("hotkey_quick_save_state", "Quick save state #1", &PerGameHotkeys[4]),
	ENTRY_OPTIONAL_HOTKEY
};
static struct MenuEntry HotkeyMenu_QuickSaveState = {
	ENTRY_OPTION("hotkey_quick_save_state", "Quick save state #1", &Hotkeys[4]),
	ENTRY_OPTIONAL_HOTKEY
};

static struct Menu PerGameHotkeyMenu = {
	.Parent = &PerGameMainMenu, .Title = "Hotkeys",
	MENU_PER_GAME,
	.AlternateVersion = &HotkeyMenu,
	.Entries = { &Strut, &PerGameHotkeyMenu_FastForward, &PerGameHotkeyMenu_FastForwardToggle, &PerGameHotkeyMenu_QuickLoadState, &PerGameHotkeyMenu_QuickSaveState, NULL }
};
static struct Menu HotkeyMenu = {
	.Parent = &MainMenu, .Title = "Hotkeys",
	.AlternateVersion = &PerGameHotkeyMenu,
	.Entries = {
#if !defined GCW_ZERO
		&HotkeyMenu_Menu,
#else
		&Strut,
#endif
		&HotkeyMenu_FastForward, &HotkeyMenu_FastForwardToggle, &HotkeyMenu_QuickLoadState, &HotkeyMenu_QuickSaveState, NULL
	}
};

// -- Saved States --

static struct MenuEntry SavedStateMenu_SelectedState = {
	.Kind = KIND_CUSTOM, .Name = "Save slot #", .PersistentName = "",
	.Target = &SelectedState,
	.ChoiceCount = 100,
	.ButtonLeftFunction = SavedStateSelectionLeft, .ButtonRightFunction = SavedStateSelectionRight,
	.DisplayValueFunction = SavedStateSelectionDisplayValue
};

static struct MenuEntry SavedStateMenu_Read = {
	.Kind = KIND_CUSTOM, .Name = "Load from selected slot",
	.ButtonEnterFunction = ActionSavedStateRead
};

static struct MenuEntry SavedStateMenu_Write = {
	.Kind = KIND_CUSTOM, .Name = "Save to selected slot",
	.ButtonEnterFunction = ActionSavedStateWrite
};

static struct MenuEntry SavedStateMenu_Delete = {
	.Kind = KIND_CUSTOM, .Name = "Delete selected state",
	.ButtonEnterFunction = ActionSavedStateDelete
};

static struct Menu SavedStateMenu = {
	.Parent = &MainMenu, .Title = "Saved states",
	.InitFunction = SavedStateMenuInit, .EndFunction = SavedStateMenuEnd,
	.DisplayDataFunction = SavedStateMenuDisplayData,
	.Entries = { &SavedStateMenu_SelectedState, &Strut, &SavedStateMenu_Read, &SavedStateMenu_Write, &SavedStateMenu_Delete, NULL }
};

// -- Main Menu --

static struct MenuEntry PerGameMainMenu_Display = {
	ENTRY_SUBMENU("Display settings...", &PerGameDisplayMenu)
};
static struct MenuEntry MainMenu_Display = {
	ENTRY_SUBMENU("Display settings...", &DisplayMenu)
};

static struct MenuEntry PerGameMainMenu_Input = {
	ENTRY_SUBMENU("Input settings...", &PerGameInputMenu)
};
static struct MenuEntry MainMenu_Input = {
	ENTRY_SUBMENU("Input settings...", &InputMenu)
};

static struct MenuEntry PerGameMainMenu_Hotkey = {
	ENTRY_SUBMENU("Hotkeys...", &PerGameHotkeyMenu)
};
static struct MenuEntry MainMenu_Hotkey = {
	ENTRY_SUBMENU("Hotkeys...", &HotkeyMenu)
};

static struct MenuEntry MainMenu_SavedStates = {
	ENTRY_SUBMENU("Saved states...", &SavedStateMenu)
};

static struct MenuEntry MainMenu_Debug = {
	ENTRY_SUBMENU("Performance and debugging...", &DebugMenu)
};

static struct MenuEntry MainMenu_Reset = {
	.Kind = KIND_CUSTOM, .Name = "Reset the game",
	.ButtonEnterFunction = &ActionReset
};

static struct MenuEntry MainMenu_Return = {
	.Kind = KIND_CUSTOM, .Name = "Return to the game",
	.ButtonEnterFunction = &ActionReturn
};

static struct MenuEntry MainMenu_Exit = {
	.Kind = KIND_CUSTOM, .Name = "Exit",
	.ButtonEnterFunction = &ActionExit
};

static struct Menu PerGameMainMenu = {
	.Parent = NULL, .Title = "ReGBA Main Menu",
	MENU_PER_GAME,
	.AlternateVersion = &MainMenu,
	.Entries = { &PerGameMainMenu_Display, &PerGameMainMenu_Input, &PerGameMainMenu_Hotkey, &Strut, &Strut, &Strut, &Strut, &Strut, &Strut, &MainMenu_Reset, &MainMenu_Return, &MainMenu_Exit, NULL }
};
struct Menu MainMenu = {
	.Parent = NULL, .Title = "ReGBA Main Menu",
	.AlternateVersion = &PerGameMainMenu,
	.Entries = { &MainMenu_Display, &MainMenu_Input, &MainMenu_Hotkey, &Strut, &MainMenu_SavedStates, &Strut, &Strut, &MainMenu_Debug, &Strut, &MainMenu_Reset, &MainMenu_Return, &MainMenu_Exit, NULL }
};

/* Do not make this the active menu */
static struct Menu ErrorScreen = {
	.Parent = &MainMenu, .Title = "Error",
	.DisplayBackgroundFunction = DisplayErrorBackgroundFunction,
	.Entries = { NULL }
};

u32 ReGBA_Menu(enum ReGBA_MenuEntryReason EntryReason)
{
	SDL_PauseAudio(SDL_ENABLE);
	MainMenu.UserData = copy_screen();
	ScaleModeUnapplied();

	// Avoid entering the menu with menu keys pressed (namely the one bound to
	// the per-game option switching key, Select).
	while (ReGBA_GetPressedButtons() != 0)
	{
		usleep(5000);
	}

	SetMenuResolution();

	struct Menu *ActiveMenu = &MainMenu, *PreviousMenu = ActiveMenu;
	if (MainMenu.InitFunction != NULL)
	{
		(*(MainMenu.InitFunction))(&ActiveMenu);
		while (PreviousMenu != ActiveMenu)
		{
			if (PreviousMenu->EndFunction != NULL)
				(*(PreviousMenu->EndFunction))(PreviousMenu);
			PreviousMenu = ActiveMenu;
			if (ActiveMenu != NULL && ActiveMenu->InitFunction != NULL)
				(*(ActiveMenu->InitFunction))(&ActiveMenu);
		}
	}

	void* PreviousGlobal = ReGBA_PreserveMenuSettings(&MainMenu);
	void* PreviousPerGame = ReGBA_PreserveMenuSettings(MainMenu.AlternateVersion);

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

		ReGBA_VideoFlip();
		
		// Wait. (This is for platforms on which flips don't wait for vertical
		// sync.)
		usleep(5000);

		// Get input.
		enum GUI_Action Action = GetGUIAction();
		
		switch (Action)
		{
			case GUI_ACTION_ENTER:
				if (ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex] != NULL)
				{
					MenuModifyFunction ButtonEnterFunction = ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]->ButtonEnterFunction;
					if (ButtonEnterFunction == NULL) ButtonEnterFunction = DefaultEnterFunction;
					(*ButtonEnterFunction)(&ActiveMenu, &ActiveMenu->ActiveEntryIndex);
					break;
				}
				// otherwise, no entry has the focus, so ENTER acts like LEAVE
				// (fall through)

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
				if (ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex] != NULL)
				{
					MenuEntryFunction ButtonLeftFunction = ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]->ButtonLeftFunction;
					if (ButtonLeftFunction == NULL) ButtonLeftFunction = DefaultLeftFunction;
					(*ButtonLeftFunction)(ActiveMenu, ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]);
				}
				break;

			case GUI_ACTION_RIGHT:
				if (ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex] != NULL)
				{
					MenuEntryFunction ButtonRightFunction = ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]->ButtonRightFunction;
					if (ButtonRightFunction == NULL) ButtonRightFunction = DefaultRightFunction;
					(*ButtonRightFunction)(ActiveMenu, ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]);
				}
				break;

			case GUI_ACTION_ALTERNATE:
				if (IsGameLoaded && ActiveMenu->AlternateVersion != NULL)
					ActiveMenu = ActiveMenu->AlternateVersion;
				break;

			default:
				break;
		}

		// Possibly finalise this menu and activate and initialise a new one.
		while (ActiveMenu != PreviousMenu)
		{
			if (PreviousMenu->EndFunction != NULL)
				(*(PreviousMenu->EndFunction))(PreviousMenu);

			// Save settings if they have changed.
			void* Global = ReGBA_PreserveMenuSettings(&MainMenu);

			if (!ReGBA_AreMenuSettingsEqual(&MainMenu, PreviousGlobal, Global))
			{
				ReGBA_SaveSettings("global_config", false);
			}
			free(PreviousGlobal);
			PreviousGlobal = Global;

			void* PerGame = ReGBA_PreserveMenuSettings(MainMenu.AlternateVersion);
			if (!ReGBA_AreMenuSettingsEqual(MainMenu.AlternateVersion, PreviousPerGame, PerGame) && IsGameLoaded)
			{
				char FileNameNoExt[MAX_PATH + 1];
				GetFileNameNoExtension(FileNameNoExt, CurrentGamePath);
				ReGBA_SaveSettings(FileNameNoExt, true);
			}
			free(PreviousPerGame);
			PreviousPerGame = PerGame;

			// Keep moving down until a menu entry can be focused, if
			// the first one can't be.
			if (ActiveMenu != NULL && ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex] != NULL)
			{
				uint32_t Sentinel = ActiveMenu->ActiveEntryIndex;
				MenuEntryCanFocusFunction CanFocusFunction = ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]->CanFocusFunction;
				if (CanFocusFunction == NULL) CanFocusFunction = DefaultCanFocusFunction;
				while (!(*CanFocusFunction)(ActiveMenu, ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]))
				{
					MoveDown(ActiveMenu, &ActiveMenu->ActiveEntryIndex);
					if (ActiveMenu->ActiveEntryIndex == Sentinel)
					{
						// If we went through all of them, we cannot focus anything.
						// Place the focus on the NULL entry.
						ActiveMenu->ActiveEntryIndex = FindNullEntry(ActiveMenu);
						break;
					}
					CanFocusFunction = ActiveMenu->Entries[ActiveMenu->ActiveEntryIndex]->CanFocusFunction;
					if (CanFocusFunction == NULL) CanFocusFunction = DefaultCanFocusFunction;
				}
			}

			PreviousMenu = ActiveMenu;
			if (ActiveMenu != NULL && ActiveMenu->InitFunction != NULL)
				(*(ActiveMenu->InitFunction))(&ActiveMenu);
		}
	}

	free(PreviousGlobal);
	free(PreviousPerGame);

	// Avoid leaving the menu with GBA keys pressed (namely the one bound to
	// the native exit button, B).
	while (ReGBA_GetPressedButtons() != 0)
	{
		usleep(5000);
	}

	SetGameResolution();

	SDL_PauseAudio(SDL_DISABLE);
	StatsStopFPS();
	timespec Now;
	clock_gettime(CLOCK_MONOTONIC, &Now);
	Stats.LastFPSCalculationTime = Now;
	if (MainMenu.UserData != NULL)
		free(MainMenu.UserData);
	return 0;
}
