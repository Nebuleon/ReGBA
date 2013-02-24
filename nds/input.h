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

#ifndef INPUT_H
#define INPUT_H

#define MAX_GAMEPAD_CONFIG_MAP 16

typedef enum
{
  BUTTON_L = 0x200,
  BUTTON_R = 0x100,
  BUTTON_DOWN = 0x80,
  BUTTON_UP = 0x40,
  BUTTON_LEFT = 0x20,
  BUTTON_RIGHT = 0x10,
  BUTTON_START = 0x08,
  BUTTON_SELECT = 0x04,
  BUTTON_B = 0x02,
  BUTTON_A = 0x01,
  BUTTON_NONE = 0x00
} input_buttons_type;

typedef enum
{
  BUTTON_ID_A   = 0x01,
  BUTTON_ID_B   = 0x02,
  BUTTON_ID_SELECT  = 0x04,
  BUTTON_ID_START   = 0x08,
  BUTTON_ID_RIGHT   = 0x10,
  BUTTON_ID_LEFT    = 0x20,
  BUTTON_ID_UP      = 0x40,
  BUTTON_ID_DOWN    = 0x80,
  BUTTON_ID_R       = 0x100,
  BUTTON_ID_L       = 0x200,
  BUTTON_ID_X       = 0x400,
  BUTTON_ID_Y       = 0x800,
  BUTTON_ID_TOUCH   = 0x1000,
  BUTTON_ID_LID     = 0x2000,
  BUTTON_ID_FA      = 0x4000,
  BUTTON_ID_FB      = 0x8000,
  BUTTON_ID_NONE    = 0
} input_buttons_id_type;

#define  KEY_ID_0   KEY_A       //!< Keypad A button.
#define  KEY_ID_1   KEY_B       //!< Keypad B button.
#define  KEY_ID_2   KEY_SELECT  //!< Keypad SELECT button.
#define  KEY_ID_3   KEY_START   //!< Keypad START button.
#define  KEY_ID_4   KEY_RIGHT   //!< Keypad RIGHT button.
#define  KEY_ID_5   KEY_LEFT    //!< Keypad LEFT button.
#define  KEY_ID_6   KEY_UP      //!< Keypad UP button.
#define  KEY_ID_7   KEY_DOWN    //!< Keypad DOWN button.
#define  KEY_ID_8   KEY_R       //!< Right shoulder button.
#define  KEY_ID_9   KEY_L       //!< Left shoulder button.
#define  KEY_ID_10  KEY_X       //!< Keypad X button.
#define  KEY_ID_11  KEY_Y       //!< Keypad Y button.
#define  KEY_ID_12  KEY_TOUCH   //!< Touchscreen pendown.
#define  KEY_ID_13  KEY_LID     //!< Lid state.

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

typedef struct _touch_screen {
  u32 x;
  u32 y;
} TOUCH_SCREEN;


void init_input();
u32 update_input();
gui_action_type get_gui_input();
void input_read_mem_savestate();
void input_write_mem_savestate();
void button_up_wait();

extern u32 tilt_sensor_x;
extern u32 tilt_sensor_y;
extern u32 sensorR;
extern u32 gamepad_config_map[MAX_GAMEPAD_CONFIG_MAP];
extern u32 gamepad_config_home;
extern TOUCH_SCREEN touch;

#endif

