/* Per-platform code - gpSP on GCW Zero
 *
 * Copyright (C) 2013 Dingoonity/GBATemp user Nebuleon
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

#ifndef __OD_INPUT_H__
#define __OD_INPUT_H__

#define OPENDINGUX_BUTTON_COUNT 12

// These must be in the order defined in OpenDinguxKeys in od-input.c.
enum OpenDingux_Buttons {
	OPENDINGUX_BUTTON_L          = 0x0001,
	OPENDINGUX_BUTTON_R          = 0x0002,
	OPENDINGUX_BUTTON_DOWN       = 0x0004,
	OPENDINGUX_BUTTON_UP         = 0x0008,
	OPENDINGUX_BUTTON_LEFT       = 0x0010,
	OPENDINGUX_BUTTON_RIGHT      = 0x0020,
	OPENDINGUX_BUTTON_START      = 0x0040,
	OPENDINGUX_BUTTON_SELECT     = 0x0080,
	OPENDINGUX_BUTTON_FACE_DOWN  = 0x0100,
	OPENDINGUX_BUTTON_FACE_RIGHT = 0x0200,
	OPENDINGUX_BUTTON_FACE_LEFT  = 0x0400,
	OPENDINGUX_BUTTON_FACE_UP    = 0x0800,
};

enum GUI_Action {
	GUI_ACTION_NONE,
	GUI_ACTION_UP,
	GUI_ACTION_DOWN,
	GUI_ACTION_LEFT,
	GUI_ACTION_RIGHT,
	GUI_ACTION_ENTER,
	GUI_ACTION_LEAVE,
};

// 0 if not fast-forwarding.
// Otherwise, the amount of frames to skip per second.
// 30 amounts to targetting 200% real-time;
// 40 amounts to targetting 300% real-time;
// 45 amounts to targetting 400% real-time;
// 48 amounts to targetting 500% real-time;
// 50 amounts to targetting 600% real-time.
extern uint_fast8_t FastForwardValue;

// The number of times to target while fast-forwarding. (UI option)
// 0 means 2x, 1 means 3x, ... 4 means 6x.
extern uint32_t FastForwardTarget;

// Incremented by FastForwardValue every frame. If it's over 60,
// a frame is skipped, 60 is removed, then FastForwardValue is added again.
extern uint_fast8_t FastForwardControl;

// Incremented by the video thread when it decides to skip a frame due to
// fast forwarding.
extern volatile uint_fast8_t VideoFastForwarded;

// Modified by the audio thread to match VideoFastForwarded after it finds
// that the video thread has skipped a frame. The audio thread must deal
// with the condition as soon as possible.
extern volatile uint_fast8_t AudioFastForwarded;

/*
 * Gets the current value of the analog horizontal axis.
 * Returns:
 *   A value between -32768 (left) and 32767 (right).
 */
extern int16_t GetHorizontalAxisValue();

/*
 * Gets the current value of the analog vertical axis.
 * Returns:
 *   A value between -32768 (up) and 32767 (down).
 */
extern int16_t GetVerticalAxisValue();

/*
 * Reads the buttons pressed at the time of the function call on the input
 * mechanism most appropriate for the port being compiled, and returns a GUI
 * action according to a priority order and the buttons being pressed.
 * Returns:
 *   A single GUI action according to the priority order, or GUI_ACTION_NONE
 *   if either the user is pressing no buttons or a repeat interval has not
 *   yet passed since the last automatic repeat of the same button.
 * Output assertions:
 *   a) The return value is a valid member of 'enum GUI_Action'.
 *   b) The priority order is Enter (sub-menu), Leave, Down, Up, Right, Left.
 *      For example, if the user had pressed Down, and now presses Enter+Down,
 *      Down's repeating is stopped while Enter is held.
 *      If the user had pressed Enter, and now presses Enter+Down, Down is
 *      ignored until Enter is released.
 */
extern enum GUI_Action GetGUIAction();

#if defined GCW_ZERO
#  define LEFT_FACE_BUTTON_NAME "X"
#  define TOP_FACE_BUTTON_NAME "Y"
#elif defined DINGOO_A320
#  define LEFT_FACE_BUTTON_NAME "Y"
#  define TOP_FACE_BUTTON_NAME "X"
#endif

extern const enum OpenDingux_Buttons DefaultKeypadRemapping[13];

// The OpenDingux_Buttons corresponding to each GBA or ReGBA button.
// [0] = GBA A
// GBA B
// GBA Select
// GBA Start
// GBA D-pad Right
// GBA D-pad Left
// GBA D-pad Up
// GBA D-pad Down
// GBA R trigger
// GBA L trigger
// ReGBA rapid-fire A
// ReGBA rapid-fire B
// [12] = ReGBA Menu
extern enum OpenDingux_Buttons KeypadRemapping[13];

// The OpenDingux_Buttons (as bitfields) for hotkeys.
// [0] = Fast-forward
extern enum OpenDingux_Buttons Hotkeys[1];

/*
 * Returns the OpenDingux buttons that are currently pressed.
 */
extern enum OpenDingux_Buttons GetPressedOpenDinguxButtons();

#endif // !defined __OD_INPUT_H__
