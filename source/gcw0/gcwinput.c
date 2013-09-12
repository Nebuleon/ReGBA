/* Per-platform code - gpSP on GCW Zero
 *
 * Copyright (C) 2013 Paul Cercueil
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

// Mandatory remapping for GCW Zero keys. Each GCW Zero key maps to a key on
// the keyboard, but not all keys on the keyboard map to these.
// They are not in GBA bitfield order in this array.
uint32_t GCWZeroKeys[12] = {
	SDLK_TAB,        // L
	SDLK_BACKSPACE,  // R
	SDLK_DOWN,       // Down
	SDLK_UP,         // Up
	SDLK_LEFT,       // Left
	SDLK_RIGHT,      // Right
	SDLK_RETURN,     // Start
	SDLK_ESCAPE,     // Select
	SDLK_LALT,       // B
	SDLK_LCTRL,      // A
	SDLK_LSHIFT,     // X
	SDLK_SPACE,      // Y
};

// These must be in the order defined above for the GCW Zero keyboard-to-
// button mappings.
enum GCWZero_Buttons {
	GCW_ZERO_BUTTON_L      = 0x0001,
	GCW_ZERO_BUTTON_R      = 0x0002,
	GCW_ZERO_BUTTON_DOWN   = 0x0004,
	GCW_ZERO_BUTTON_UP     = 0x0008,
	GCW_ZERO_BUTTON_LEFT   = 0x0010,
	GCW_ZERO_BUTTON_RIGHT  = 0x0020,
	GCW_ZERO_BUTTON_START  = 0x0040,
	GCW_ZERO_BUTTON_SELECT = 0x0080,
	GCW_ZERO_BUTTON_B      = 0x0100,
	GCW_ZERO_BUTTON_A      = 0x0200,
	GCW_ZERO_BUTTON_X      = 0x0400,
	GCW_ZERO_BUTTON_Y      = 0x0800,
};

// These must be GCW Zero buttons at the bit suitable for the ReGBA_Buttons
// enumeration.
uint32_t KeypadRemapping[13] = {
	GCW_ZERO_BUTTON_A,       // GBA A
	GCW_ZERO_BUTTON_B,       // GBA B
	GCW_ZERO_BUTTON_SELECT,  // GBA Select
	GCW_ZERO_BUTTON_START,   // GBA Start
	GCW_ZERO_BUTTON_RIGHT,   // GBA D-pad Right
	GCW_ZERO_BUTTON_LEFT,    // GBA D-pad Left
	GCW_ZERO_BUTTON_UP,      // GBA D-pad Up
	GCW_ZERO_BUTTON_DOWN,    // GBA D-pad Down
	GCW_ZERO_BUTTON_R,       // GBA R trigger
	GCW_ZERO_BUTTON_L,       // GBA L trigger
	GCW_ZERO_BUTTON_Y,       // ReGBA rapid-fire A
	GCW_ZERO_BUTTON_X,       // ReGBA rapid-fire B
	0                        // ReGBA Menu
};

static enum GCWZero_Buttons LastButtons = 0;

static void UpdateGCWZeroButtons()
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
				for (i = 0; i < sizeof(GCWZeroKeys) / sizeof(GCWZeroKeys[0]); i++)
				{
					if (ev.key.keysym.sym == GCWZeroKeys[i])
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

	// Before a menu gets implemented, X+Y held together are equivalent to an
	// exit command.
	if ((LastButtons & (GCW_ZERO_BUTTON_X | GCW_ZERO_BUTTON_Y)) == (GCW_ZERO_BUTTON_X | GCW_ZERO_BUTTON_Y))
		quit();
}

enum ReGBA_Buttons ReGBA_GetPressedButtons()
{
	uint_fast8_t i;
	enum ReGBA_Buttons Result = 0;

	UpdateGCWZeroButtons();
	for (i = 0; i < 13; i++)
	{
		if (LastButtons & KeypadRemapping[i])
		{
			Result |= 1 << (uint_fast16_t) i;
		}
	}
	return Result;
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
