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

#ifndef __GCW_INPUT_H__
#define __GCW_INPUT_H__

// 0 if not fast-forwarding.
// Otherwise, the amount of frames to skip per second.
// 30 amounts to targetting 200% real-time;
// 40 amounts to targetting 300% real-time;
// 45 amounts to targetting 400% real-time;
// 48 amounts to targetting 500% real-time;
// 50 amounts to targetting 600% real-time.
extern uint_fast8_t FastForwardValue;

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

#endif // !defined __GCW_INPUT_H__
