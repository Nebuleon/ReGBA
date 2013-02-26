/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
 * Copyright (C) 2007 ????? <?????>
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

#include "common.h"

TOUCH_SCREEN touch;

enum PspCtrlButtons
{
	/** Select button. */
	PSP_CTRL_SELECT     = 0x000001,
	/** Start button. */
	PSP_CTRL_START      = 0x000008,
	/** Up D-Pad button. */
	PSP_CTRL_UP         = 0x000010,
	/** Right D-Pad button. */
	PSP_CTRL_RIGHT      = 0x000020,
	/** Down D-Pad button. */
	PSP_CTRL_DOWN      	= 0x000040,
	/** Left D-Pad button. */
	PSP_CTRL_LEFT      	= 0x000080,
	/** Left trigger. */
	PSP_CTRL_LTRIGGER   = 0x000100,
	/** Right trigger. */
	PSP_CTRL_RTRIGGER   = 0x000200,
	/** Triangle button. */
	PSP_CTRL_TRIANGLE   = 0x001000,
	/** Circle button. */
	PSP_CTRL_CIRCLE     = 0x002000,
	/** Cross button. */
	PSP_CTRL_CROSS      = 0x004000,
	/** Square button. */
	PSP_CTRL_SQUARE     = 0x008000,
	/** Home button. */
	PSP_CTRL_HOME       = 0x010000,
	/** Hold button. */
	PSP_CTRL_HOLD       = 0x020000,
	/** Music Note button. */
	PSP_CTRL_NOTE       = 0x800000,
};

#ifndef NDS_LAYER
u32 readkey()
{
    unsigned char key;
	u32 ret;
	
	if(!serial_getc_noblock(&key))
	    return 0;
	
	switch(key){
		case 'i': ret= PSP_CTRL_UP;
					break;
		case 'k': ret= PSP_CTRL_DOWN;
					break;
		case 'j': ret= PSP_CTRL_LEFT;
					break;
		case 'l': ret= PSP_CTRL_RIGHT;
					break;
		case 'q': ret= PSP_CTRL_CIRCLE;
					break;
		case 'w': ret= PSP_CTRL_CROSS;
					break;
		case 'a': ret= PSP_CTRL_TRIANGLE;
					break;
		case 's': ret= PSP_CTRL_SQUARE;
					break;
		case 'z': ret= PSP_CTRL_SELECT;
					break;
		case 'x': ret= PSP_CTRL_START;
					break;
		case 'e': ret= PSP_CTRL_LTRIGGER;
					break;
		case 'r': ret= PSP_CTRL_RTRIGGER;
					break;
		default: ret= 0;
					break;
	}
	
	return ret;
}
#else
u32 readkey()
{
    struct key_buf inputdata;

    ds2_getrawInput(&inputdata);

    touch.x = inputdata.x;
    touch.y = inputdata.y;
	return inputdata.key;
}
#endif

/*--------------------------------------------------------
	Wait any key in [key_list] pressed
	if key_list == NULL, return at any key pressed
--------------------------------------------------------*/
unsigned int wait_Anykey_press(unsigned int key_list)
{
	unsigned int key;

	while(1)
	{
		key = getKey();
		if(key) {
			if(0 == key_list) break;
			else if(key & key_list) break;
		}
	}

	return key;
}

/*--------------------------------------------------------
	Wait all key in [key_list] released
	if key_list == NULL, return at all key released
--------------------------------------------------------*/
void wait_Allkey_release(unsigned int key_list)
{
	unsigned int key;
    struct key_buf inputdata;

	while(1)
	{
		ds2_getrawInput(&inputdata);
		key = inputdata.key;

		if(0 == key) break;
		else if(!key_list) continue;
		else if(0 == (key_list & key)) break;
	}
}

// Special thanks to psp298 for the analog->dpad code!
void trigger_key(u32 key);

void trigger_key(u32 key)
{
  u32 p1_cnt = io_registers[REG_P1CNT];
//  u32 p1;

  if((p1_cnt >> 14) & 0x01)
  {
    u32 key_intersection = (p1_cnt & key) & 0x3FF;

    if(p1_cnt >> 15)
    {
      if(key_intersection == (p1_cnt & 0x3FF))
        raise_interrupt(IRQ_KEYPAD);
    }
    else
    {
      if(key_intersection)
        raise_interrupt(IRQ_KEYPAD);
    }
  }
}

u32 key = 0;
TOUCH_SCREEN last_touch= {0, 0};

#define PSP_ALL_BUTTON_MASK 0x1FFFF

u32 gamepad_config_map[MAX_GAMEPAD_CONFIG_MAP];
u32 gamepad_config_home= BUTTON_ID_X;

u32 last_buttons = 0;
//u64 button_repeat_timestamp;
u32 button_repeat_timestamp;

typedef enum
{
  BUTTON_NOT_HELD,
  BUTTON_HELD_INITIAL,
  BUTTON_HELD_REPEAT
} button_repeat_state_type;

button_repeat_state_type button_repeat_state = BUTTON_NOT_HELD;
u32 button_repeat = 0;
gui_action_type cursor_repeat = CURSOR_NONE;

u32 tilt_sensor_x;
u32 tilt_sensor_y;
u32 sensorR;

static u32 button_circle;
static u32 button_cross;

//#define BUTTON_REPEAT_START    200000
//#define BUTTON_REPEAT_CONTINUE 50000
#define BUTTON_REPEAT_START    (200)    // Not sure what this 200 is
				// Just avoiding a compiler error [Neb]
#define BUTTON_REPEAT_CONTINUE (0)                        //之后重复间隔 0

// GUI输入处理
gui_action_type get_gui_input(void)
{
	unsigned int key;
	gui_action_type	ret;

	key = getKey();

	if (key & KEY_LID)
	{
		ds2_setSupend();
		struct key_buf inputdata;
		do {
			ds2_getrawInput(&inputdata);
			mdelay(1);
		} while (inputdata.key & KEY_LID);
		ds2_wakeup();
		// In the menu, the lower screen's backlight needs to be on,
		// and it is on right away after resuming from suspend.
		// mdelay(100); // needed to avoid ds2_setBacklight crashing
		// ds2_setBacklight(3);
	}

	switch(key)
	{
		case KEY_UP:
			ret = CURSOR_UP;
			break;
		case KEY_DOWN:
			ret = CURSOR_DOWN;
			break;
		case KEY_LEFT:
			ret = CURSOR_LEFT;
			break;
		case KEY_RIGHT:
			ret = CURSOR_RIGHT;
			break;
		case KEY_L:
			ret = CURSOR_LTRIGGER;
			break;
		case KEY_R:
			ret = CURSOR_RTRIGGER;
			break;
		case KEY_A:
			ret = CURSOR_SELECT;
			break;
		case KEY_B:
			ret = CURSOR_BACK;
			break;
		case KEY_X:
			ret = CURSOR_EXIT;
			break;
		case KEY_TOUCH:
			ret = CURSOR_TOUCH;
			break;
		default:
			ret = CURSOR_NONE;
			break;
	}

	return ret;
}

u32 button_input_to_gba[] =
{
    BUTTON_A,
    BUTTON_B,
    BUTTON_SELECT,
    BUTTON_START,
    BUTTON_RIGHT,
    BUTTON_LEFT,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_R,
    BUTTON_L,
    BUTTON_ID_FA,
    BUTTON_ID_FB,
    BUTTON_ID_NONE
};

u32 rapidfire_flag = 1;

// 仿真过程输入
u32 update_input()
{
    u32 buttons;
    u32 non_repeat_buttons;
    u32 i;
    u32 new_key = 0;

    buttons = readkey();
    non_repeat_buttons = (last_buttons ^ buttons) & buttons;
    last_buttons = buttons;

    if(non_repeat_buttons & gamepad_config_home)   //Call main menu
    {
        u16 screen_copy[GBA_SCREEN_BUFF_SIZE];
        copy_screen(screen_copy);
        u32 ret_val = menu(screen_copy);

        return ret_val;
    }

    for(i = 0; i < 13; i++)
    {
        if( buttons & gamepad_config_map[i] )
            new_key |= button_input_to_gba[i];
    }

    if(new_key & BUTTON_ID_FA)  //Rapid fire A
    {
        if(rapidfire_flag)
            new_key |= BUTTON_A;
        else
            new_key &= ~BUTTON_A;

        rapidfire_flag ^= 1;
    }
    
    if(new_key & BUTTON_ID_FB) //Rapid fire B
    {
        if(rapidfire_flag)
            new_key |= BUTTON_B;
        else
            new_key &= ~BUTTON_B;

        rapidfire_flag ^= 1;
    }

    if((new_key | key) != key)
        trigger_key(new_key);

    key = new_key;
    io_registers[REG_P1] = (~key) & 0x3FF;

    return 0;
}


#if 0
void init_input()
{
  int button_swap;
  sceCtrlSetSamplingCycle(0);
  sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
  initHomeButton(sceKernelDevkitVersion());
  tilt_sensor_x = 0x800;
  tilt_sensor_y = 0x800;
  sceUtilityGetSystemParamInt(9, &button_swap);
  if (button_swap == 0)
  {
    button_circle = CURSOR_SELECT;
    button_cross  = CURSOR_EXIT;
  }
  else
  {
    button_circle = CURSOR_EXIT;
    button_cross  = CURSOR_SELECT;
  }
}
#endif

void init_input()
{
    button_circle = CURSOR_SELECT;
    button_cross  = CURSOR_EXIT;
}

// type = READ / WRITE_MEM
#define input_savestate_body(type)                                            \
{                                                                             \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, key);                            \
}                                                                             \

void input_read_mem_savestate()
input_savestate_body(READ_MEM);

void input_write_mem_savestate()
input_savestate_body(WRITE_MEM);
