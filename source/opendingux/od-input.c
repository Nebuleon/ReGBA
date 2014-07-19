/* Per-platform code - ReGBA on OpenDingux
 *
 * Copyright (C) 2013 Paul Cercueil and Dingoonity user Nebuleon
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

uint_fast8_t FastForwardFrameskip = 0;

uint32_t PerGameFastForwardTarget = 0;
uint32_t FastForwardTarget = 4; // 6x by default

uint32_t PerGameAnalogSensitivity = 0;
uint32_t AnalogSensitivity = 0; // require 32256/32767 of the axis by default

uint32_t PerGameAnalogAction = 0;
uint32_t AnalogAction = 0;

uint_fast8_t FastForwardFrameskipControl = 0;

static SDL_Joystick* Joystick;

static bool JoystickInitialised = false;

// Mandatory remapping for OpenmDingux keys. Each OpenmDingux key maps to a
// key on the keyboard, but not all keys on the keyboard map to these.
// They are not in GBA bitfield order in this array.
uint32_t OpenDinguxKeys[OPENDINGUX_BUTTON_COUNT] = {
	SDLK_TAB,        // L
	SDLK_BACKSPACE,  // R
	SDLK_DOWN,       // Down
	SDLK_UP,         // Up
	SDLK_LEFT,       // Left
	SDLK_RIGHT,      // Right
	SDLK_RETURN,     // Start
	SDLK_ESCAPE,     // Select
	SDLK_LALT,       // Lower face button (B)
	SDLK_LCTRL,      // Right face button (A)
	SDLK_LSHIFT,     // Left face button (GCW X, A320 Y)
	SDLK_SPACE,      // Upper face button (GCW Y, A320 X)
	0,
	0,
	0,
	0,
	SDLK_HOME,       // GCW: Quick flick of Power
};

// These must be OpenDingux buttons at the bit suitable for the ReGBA_Buttons
// enumeration.
const enum OpenDingux_Buttons DefaultKeypadRemapping[12] = {
	OPENDINGUX_BUTTON_FACE_RIGHT, // GBA A
	OPENDINGUX_BUTTON_FACE_DOWN,  // GBA B
	OPENDINGUX_BUTTON_SELECT,     // GBA Select
	OPENDINGUX_BUTTON_START,      // GBA Start
	OPENDINGUX_BUTTON_RIGHT,      // GBA D-pad Right
	OPENDINGUX_BUTTON_LEFT,       // GBA D-pad Left
	OPENDINGUX_BUTTON_UP,         // GBA D-pad Up
	OPENDINGUX_BUTTON_DOWN,       // GBA D-pad Down
	OPENDINGUX_BUTTON_R,          // GBA R trigger
	OPENDINGUX_BUTTON_L,          // GBA L trigger
	0,                            // ReGBA rapid-fire A
	0,                            // ReGBA rapid-fire B
};

// These must be OpenDingux buttons at the bit suitable for the ReGBA_Buttons
// enumeration.
enum OpenDingux_Buttons PerGameKeypadRemapping[12] = {
	0, // GBA A
	0, // GBA B
	0, // GBA Select
	0, // GBA Start
	0, // GBA D-pad Right
	0, // GBA D-pad Left
	0, // GBA D-pad Up
	0, // GBA D-pad Down
	0, // GBA R trigger
	0, // GBA L trigger
	0, // ReGBA rapid-fire A
	0, // ReGBA rapid-fire B
};
enum OpenDingux_Buttons KeypadRemapping[12] = {
	OPENDINGUX_BUTTON_FACE_RIGHT, // GBA A
	OPENDINGUX_BUTTON_FACE_DOWN,  // GBA B
	OPENDINGUX_BUTTON_SELECT,     // GBA Select
	OPENDINGUX_BUTTON_START,      // GBA Start
	OPENDINGUX_BUTTON_RIGHT,      // GBA D-pad Right
	OPENDINGUX_BUTTON_LEFT,       // GBA D-pad Left
	OPENDINGUX_BUTTON_UP,         // GBA D-pad Up
	OPENDINGUX_BUTTON_DOWN,       // GBA D-pad Down
	OPENDINGUX_BUTTON_R,          // GBA R trigger
	OPENDINGUX_BUTTON_L,          // GBA L trigger
	0,                            // ReGBA rapid-fire A
	0,                            // ReGBA rapid-fire B
};

enum OpenDingux_Buttons PerGameHotkeys[5] = {
	0, // Fast-forward while held
	0, // Menu
	0, // Fast-forward toggle
	0, // Quick load state #1
	0, // Quick save state #1
};
enum OpenDingux_Buttons Hotkeys[5] = {
	0,                            // Fast-forward while held
	OPENDINGUX_BUTTON_FACE_UP,    // Menu
	0,                            // Fast-forward toggle
	0,                            // Quick load state #1
	0,                            // Quick save state #1
};

// The menu keys, in decreasing order of priority when two or more are
// pressed. For example, when the user keeps a direction pressed but also
// presses A, start ignoring the direction.
enum OpenDingux_Buttons MenuKeys[7] = {
	OPENDINGUX_BUTTON_FACE_RIGHT,                     // Select/Enter button
	OPENDINGUX_BUTTON_FACE_DOWN,                      // Cancel/Leave button
	OPENDINGUX_BUTTON_DOWN  | OPENDINGUX_ANALOG_DOWN, // Menu navigation
	OPENDINGUX_BUTTON_UP    | OPENDINGUX_ANALOG_UP,
	OPENDINGUX_BUTTON_RIGHT | OPENDINGUX_ANALOG_RIGHT,
	OPENDINGUX_BUTTON_LEFT  | OPENDINGUX_ANALOG_LEFT,
	OPENDINGUX_BUTTON_SELECT,
};

// In the same order as MenuKeys above. Maps the OpenDingux buttons to their
// corresponding GUI action.
enum GUI_Action MenuKeysToGUI[7] = {
	GUI_ACTION_ENTER,
	GUI_ACTION_LEAVE,
	GUI_ACTION_DOWN,
	GUI_ACTION_UP,
	GUI_ACTION_RIGHT,
	GUI_ACTION_LEFT,
	GUI_ACTION_ALTERNATE,
};

// CurButtons allows only one state change per button relative to LastButtons
// during a frame.
// FutureButtons alows any number of state changes per button.
static enum OpenDingux_Buttons LastButtons = 0, CurButtons = 0, FutureButtons = 0;

static void UpdateOpenDinguxButtons()
{
	SDL_Event ev;
	uint_fast8_t i;

	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
			case SDL_KEYDOWN:
				for (i = 0; i < sizeof(OpenDinguxKeys) / sizeof(OpenDinguxKeys[0]); i++)
					if (ev.key.keysym.sym == OpenDinguxKeys[i])
					{
						FutureButtons |= 1 << i;
						if ((LastButtons & (1 << i)) == (CurButtons & (1 << i)))
							CurButtons |= 1 << i;
						break;
					}
				break;
			case SDL_KEYUP:
				for (i = 0; i < sizeof(OpenDinguxKeys) / sizeof(OpenDinguxKeys[0]); i++)
					if (ev.key.keysym.sym == OpenDinguxKeys[i])
					{
						FutureButtons &= ~(1 << i);
						if ((LastButtons & (1 << i)) == (CurButtons & (1 << i)))
							CurButtons &= ~(1 << i);
						break;
					}
				break;
			default:
				break;
		}
	}

	CurButtons &= ~(OPENDINGUX_ANALOG_LEFT | OPENDINGUX_ANALOG_RIGHT
	               | OPENDINGUX_ANALOG_UP | OPENDINGUX_ANALOG_DOWN);
	int16_t X = GetHorizontalAxisValue(), Y = GetVerticalAxisValue(),
	        Threshold = (4 - ResolveSetting(AnalogSensitivity, PerGameAnalogSensitivity)) * 7808 + 1024;
	if (X > Threshold)       CurButtons |= OPENDINGUX_ANALOG_RIGHT;
	else if (X < -Threshold) CurButtons |= OPENDINGUX_ANALOG_LEFT;
	if (Y > Threshold)       CurButtons |= OPENDINGUX_ANALOG_DOWN;
	else if (Y < -Threshold) CurButtons |= OPENDINGUX_ANALOG_UP;
}

static bool IsFastForwardToggled = false;
static bool WasFastForwardToggleHeld = false;
static bool WasLoadStateHeld = false;
static bool WasSaveStateHeld = false;

void ProcessSpecialKeys()
{
	enum OpenDingux_Buttons ButtonCopy = LastButtons;

	enum OpenDingux_Buttons FastForwardToggle = ResolveButtons(Hotkeys[2], PerGameHotkeys[2]);
	bool IsFastForwardToggleHeld = FastForwardToggle != 0 && (FastForwardToggle & LastButtons) == FastForwardToggle;
	if (!WasFastForwardToggleHeld && IsFastForwardToggleHeld)
		IsFastForwardToggled = !IsFastForwardToggled;
	WasFastForwardToggleHeld = IsFastForwardToggleHeld;

	// Resolve fast-forwarding. It is activated if it's either toggled by the
	// Toggle hotkey, or the While Held key is held down.
	enum OpenDingux_Buttons FastForwardWhileHeld = ResolveButtons(Hotkeys[0], PerGameHotkeys[0]);
	bool IsFastForwardWhileHeld = FastForwardWhileHeld != 0 && (FastForwardWhileHeld & ButtonCopy) == FastForwardWhileHeld;
	FastForwardFrameskip = (IsFastForwardToggled || IsFastForwardWhileHeld)
		? ResolveSetting(FastForwardTarget, PerGameFastForwardTarget) + 1
		: 0;

	enum OpenDingux_Buttons LoadState = ResolveButtons(Hotkeys[3], PerGameHotkeys[3]);
	bool IsLoadStateHeld = LoadState != 0 && (LoadState & ButtonCopy) == LoadState;
	if (!WasLoadStateHeld && IsLoadStateHeld)
	{
		SetMenuResolution();
		switch (load_state(0))
		{
			case 1:
				if (errno != 0)
					ShowErrorScreen("Reading saved state #1 failed:\n%s", strerror(errno));
				else
					ShowErrorScreen("Reading saved state #1 failed:\nIncomplete file");
				break;

			case 2:
				ShowErrorScreen("Reading saved state #1 failed:\nFile format invalid");
				break;
		}
		SetGameResolution();
	}
	WasLoadStateHeld = IsLoadStateHeld;
	
	enum OpenDingux_Buttons SaveState = ResolveButtons(Hotkeys[4], PerGameHotkeys[4]);
	bool IsSaveStateHeld = SaveState != 0 && (SaveState & ButtonCopy) == SaveState;
	if (!WasSaveStateHeld && IsSaveStateHeld)
	{
		void* Screenshot = copy_screen();
		SetMenuResolution();
		if (Screenshot == NULL)
			ShowErrorScreen("Gathering the screenshot for saved state #1 failed: Memory allocation error");
		else
		{
			uint32_t ret = save_state(0, Screenshot /* preserved screenshot */);
			free(Screenshot);
			if (ret != 1)
			{
				if (errno != 0)
					ShowErrorScreen("Writing saved state #1 failed:\n%s", strerror(errno));
				else
					ShowErrorScreen("Writing saved state #1 failed:\nUnknown error");
			}
		}
		SetGameResolution();
	}
	WasSaveStateHeld = IsSaveStateHeld;
}

enum ReGBA_Buttons ReGBA_GetPressedButtons()
{
	uint_fast8_t i;
	enum ReGBA_Buttons Result = 0;

	UpdateOpenDinguxButtons();

	LastButtons = CurButtons;
	CurButtons = FutureButtons;

	ProcessSpecialKeys();
	for (i = 0; i < 12; i++)
	{
		if (LastButtons & ResolveButtons(KeypadRemapping[i], PerGameKeypadRemapping[i]))
		{
			Result |= 1 << (uint_fast16_t) i;
		}
	}
	if (ResolveSetting(AnalogAction, PerGameAnalogAction) == 1)
	{
		if (LastButtons & OPENDINGUX_ANALOG_LEFT)  Result |= REGBA_BUTTON_LEFT;
		if (LastButtons & OPENDINGUX_ANALOG_RIGHT) Result |= REGBA_BUTTON_RIGHT;
		if (LastButtons & OPENDINGUX_ANALOG_UP)    Result |= REGBA_BUTTON_UP;
		if (LastButtons & OPENDINGUX_ANALOG_DOWN)  Result |= REGBA_BUTTON_DOWN;
	}

	if ((Result & REGBA_BUTTON_LEFT) && (Result & REGBA_BUTTON_RIGHT))
		Result &= ~REGBA_BUTTON_LEFT;
	if ((Result & REGBA_BUTTON_UP) && (Result & REGBA_BUTTON_DOWN))
		Result &= ~REGBA_BUTTON_UP;

	if (
#if defined GCW_ZERO
	// Unified emulator menu buttons: Start+Select
		((LastButtons & (OPENDINGUX_BUTTON_START | OPENDINGUX_BUTTON_SELECT)) == (OPENDINGUX_BUTTON_START | OPENDINGUX_BUTTON_SELECT))
#else
	// The ReGBA Menu key should be pressed if ONLY the hotkey bound to it
	// is pressed on the native device.
	// This is not in ProcessSpecialKeys because REGBA_BUTTON_MENU needs to
	// be returned by ReGBA_GetPressedButtons.
		LastButtons == Hotkeys[1]
#endif
	 || (LastButtons & OPENDINGUX_BUTTON_MENU))
		Result |= REGBA_BUTTON_MENU;

	return Result;
}

bool IsImpossibleHotkey(enum OpenDingux_Buttons Hotkey)
{
	if ((Hotkey & (OPENDINGUX_BUTTON_LEFT | OPENDINGUX_BUTTON_RIGHT)) == (OPENDINGUX_BUTTON_LEFT | OPENDINGUX_BUTTON_RIGHT))
		return true;
	if ((Hotkey & (OPENDINGUX_BUTTON_UP | OPENDINGUX_BUTTON_DOWN)) == (OPENDINGUX_BUTTON_UP | OPENDINGUX_BUTTON_DOWN))
		return true;
#if defined DINGOO_A320
	if (Hotkey & (OPENDINGUX_ANALOG_LEFT | OPENDINGUX_ANALOG_RIGHT | OPENDINGUX_ANALOG_UP | OPENDINGUX_ANALOG_DOWN))
		return true;
#elif defined GCW_ZERO
	if ((Hotkey & (OPENDINGUX_ANALOG_LEFT | OPENDINGUX_ANALOG_RIGHT)) == (OPENDINGUX_ANALOG_LEFT | OPENDINGUX_ANALOG_RIGHT))
		return true;
	if ((Hotkey & (OPENDINGUX_ANALOG_UP | OPENDINGUX_ANALOG_DOWN)) == (OPENDINGUX_ANALOG_UP | OPENDINGUX_ANALOG_DOWN))
		return true;
#endif
	return false;
}

enum OpenDingux_Buttons GetPressedOpenDinguxButtons()
{
	UpdateOpenDinguxButtons();

	LastButtons = CurButtons;
	CurButtons = FutureButtons;

	return LastButtons & ~OPENDINGUX_BUTTON_MENU;
}

static void EnsureJoystick()
{
	if (!JoystickInitialised)
	{
		JoystickInitialised = true;
		Joystick = SDL_JoystickOpen(0);
		if (Joystick == NULL)
		{
			ReGBA_Trace("I: Joystick #0 could not be opened");
		}
	}
}

int16_t GetHorizontalAxisValue()
{
	EnsureJoystick();
	if (Joystick != NULL)
		return SDL_JoystickGetAxis(Joystick, 0);
	else
		return 0;
}

int16_t GetVerticalAxisValue()
{
	EnsureJoystick();
	if (Joystick != NULL)
		return SDL_JoystickGetAxis(Joystick, 1);
	else
		return 0;
}

enum GUI_ActionRepeatState
{
  BUTTON_NOT_HELD,
  BUTTON_HELD_INITIAL,
  BUTTON_HELD_REPEAT
};

// Nanoseconds to wait before repeating GUI actions the first time.
static const uint64_t BUTTON_REPEAT_START    = 500000000;
// Nanoseconds to wait before repeating GUI actions subsequent times.
static const uint64_t BUTTON_REPEAT_CONTINUE = 100000000;

static enum GUI_ActionRepeatState ActionRepeatState = BUTTON_NOT_HELD;
static struct timespec LastActionRepeat;
static enum GUI_Action LastAction = GUI_ACTION_NONE;

enum GUI_Action GetGUIAction()
{
	uint_fast8_t i;
	enum GUI_Action Result = GUI_ACTION_NONE;

	UpdateOpenDinguxButtons();

	LastButtons = CurButtons;
	CurButtons = FutureButtons;

	enum OpenDingux_Buttons EffectiveButtons = LastButtons;
	// Now get the currently-held button with the highest priority in MenuKeys.
	for (i = 0; i < sizeof(MenuKeys) / sizeof(MenuKeys[0]); i++)
	{
		if ((EffectiveButtons & MenuKeys[i]) != 0)
		{
			Result = MenuKeysToGUI[i];
			break;
		}
	}

	struct timespec Now;
	clock_gettime(CLOCK_MONOTONIC, &Now);
	if (Result == GUI_ACTION_NONE || LastAction != Result || ActionRepeatState == BUTTON_NOT_HELD)
	{
		LastAction = Result;
		LastActionRepeat = Now;
		if (Result != GUI_ACTION_NONE)
			ActionRepeatState = BUTTON_HELD_INITIAL;
		else
			ActionRepeatState = BUTTON_NOT_HELD;
		return Result;
	}

	// We are certain of the following here:
	// Result != GUI_ACTION_NONE && LastAction == Result
	// We need to check how much time has passed since the last repeat.
	struct timespec Difference = TimeDifference(LastActionRepeat, Now);
	uint64_t DiffNanos = (uint64_t) Difference.tv_sec * 1000000000 + Difference.tv_nsec;
	uint64_t RequiredNanos = (ActionRepeatState == BUTTON_HELD_INITIAL)
		? BUTTON_REPEAT_START
		: BUTTON_REPEAT_CONTINUE;

	if (DiffNanos < RequiredNanos)
		return GUI_ACTION_NONE;

	// Here we repeat the action.
	LastActionRepeat = Now;
	ActionRepeatState = BUTTON_HELD_REPEAT;
	return Result;
}
