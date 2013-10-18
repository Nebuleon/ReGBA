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

uint_fast8_t FastForwardValue = 0;

uint_fast8_t FastForwardControl = 0;

static SDL_Joystick* Joystick;

static bool JoystickInitialised = false;

// Mandatory remapping for OpenmDingux keys. Each OpenmDingux key maps to a
// key on the keyboard, but not all keys on the keyboard map to these.
// They are not in GBA bitfield order in this array.
uint32_t OpenDinguxKeys[12] = {
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
};

// These must be OpenDingux buttons at the bit suitable for the ReGBA_Buttons
// enumeration.
enum OpenDingux_Buttons KeypadRemapping[13] = {
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
	OPENDINGUX_BUTTON_FACE_UP,    // ReGBA Menu
};

// The menu keys, in decreasing order of priority when two or more are
// pressed. For example, when the user keeps a direction pressed but also
// presses A, start ignoring the direction.
enum OpenDingux_Buttons MenuKeys[6] = {
	OPENDINGUX_BUTTON_FACE_RIGHT,  // Select/Enter button
	OPENDINGUX_BUTTON_FACE_DOWN,   // Cancel/Leave button
	OPENDINGUX_BUTTON_DOWN,        // Menu navigation
	OPENDINGUX_BUTTON_UP,
	OPENDINGUX_BUTTON_RIGHT,
	OPENDINGUX_BUTTON_LEFT,
};

// In the same order as MenuKeys above. Maps the OpenDingux buttons to their
// corresponding GUI action.
enum GUI_Action MenuKeysToGUI[6] = {
	GUI_ACTION_ENTER,
	GUI_ACTION_LEAVE,
	GUI_ACTION_DOWN,
	GUI_ACTION_UP,
	GUI_ACTION_RIGHT,
	GUI_ACTION_LEFT,
};

static enum OpenDingux_Buttons LastButtons = 0;

static void UpdateOpenDinguxButtons()
{
	SDL_Event ev;

	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				uint_fast8_t i;
				for (i = 0; i < sizeof(OpenDinguxKeys) / sizeof(OpenDinguxKeys[0]); i++)
				{
					if (ev.key.keysym.sym == OpenDinguxKeys[i])
					{
						if (ev.type == SDL_KEYDOWN)
							LastButtons |= 1 << (uint_fast16_t) i;
						else
							LastButtons &= ~(1 << (uint_fast16_t) i);
						break;
					}
				}
				break;
			}
			default:
				break;
		}
	}
}

void ProcessSpecialKeys()
{
	// none for now
}

enum ReGBA_Buttons ReGBA_GetPressedButtons()
{
	uint_fast8_t i;
	enum ReGBA_Buttons Result = 0;

	UpdateOpenDinguxButtons();
	ProcessSpecialKeys();
	for (i = 0; i < 13; i++)
	{
		if (LastButtons & KeypadRemapping[i])
		{
			Result |= 1 << (uint_fast16_t) i;
		}
	}
	return Result;
}

enum OpenDingux_Buttons GetPressedOpenDinguxButtons()
{
	UpdateOpenDinguxButtons();
	return LastButtons;
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
	enum OpenDingux_Buttons EffectiveButtons = LastButtons;
	// Incorporate analog nub input. (GCW Zero, implemented as 0 on the
	// Dingoo A320.)
	int16_t X = GetHorizontalAxisValue(), Y = GetVerticalAxisValue();
	     if (X < -32700) EffectiveButtons |= OPENDINGUX_BUTTON_LEFT;
	else if (X >  32700) EffectiveButtons |= OPENDINGUX_BUTTON_RIGHT;
	     if (Y < -32700) EffectiveButtons |= OPENDINGUX_BUTTON_UP;
	else if (Y >  32700) EffectiveButtons |= OPENDINGUX_BUTTON_DOWN;
	// Now get the currently-held button with the highest priority in MenuKeys.
	for (i = 0; i < 6; i++)
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
