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
    u32 ret= 0;

    updateInput(&inputdata);
#if 0
    if( inputdata.key & KEY_A )
        ret |= KEY_ID_0;
            
    if( inputdata.key & KEY_B )
        ret |= KEY_ID_1;
       
    if( inputdata.key & KEY_X )
        ret |= KEY_ID_2;
        
    if( inputdata.key & KEY_Y )
        ret |= KEY_ID_3;
        
    if( inputdata.key & KEY_SELECT )
        ret |= KEY_ID_4;
        
    if( inputdata.key & KEY_START )
        ret |= KEY_ID_5;
        
    if( inputdata.key & KEY_RIGHT )
        ret |= KEY_ID_6;
        
    if( inputdata.key & KEY_LEFT )
        ret |= KEY_ID_7;
        
    if( inputdata.key & KEY_UP )
        ret |= KEY_ID_8;
        
    if( inputdata.key & KEY_DOWN )
        ret |= KEY_ID_9;
        
    if( inputdata.key & KEY_R )
        ret |= KEY_ID_10;
        
    if( inputdata.key & KEY_L )
        ret |= KEY_ID_11;

    if( inputdata.key & KEY_TOUCH )
        ret |= KEY_ID_12;

    if( inputdata.key & KEY_LID )
        ret |= KEY_ID_13;
#endif
    touch.x = inputdata.x;
    touch.y = inputdata.y;
	return ret = inputdata.key;
}
#endif

void button_up_wait()
{
    while(readkey())
    {
        OSTimeDly(2);
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
gui_action_type get_gui_input()
{
  gui_action_type new_button = CURSOR_NONE;
  u32 new_buttons;
  u32 row_buttons;

//  OSTimeDly(OS_TICKS_PER_SEC/50);       //delay 20ms

//  if (quit_flag == 1)
//    quit(0);

  row_buttons = readkey();

  new_buttons = (last_buttons ^ row_buttons) & row_buttons;
  last_buttons = row_buttons;

  if(new_buttons & KEY_ID_5)    //KEY_LEFT
    new_button = CURSOR_LEFT;

  if(new_buttons & KEY_ID_4)    //KEY_RIGHT
    new_button = CURSOR_RIGHT;

  if(new_buttons & KEY_ID_6)    //KEY_UP
    new_button = CURSOR_UP;

  if(new_buttons & KEY_ID_7)    //KEY_DOWN
    new_button = CURSOR_DOWN;

  if(new_buttons & KEY_ID_0)    //A
    new_button = button_circle;

  if(new_buttons & KEY_ID_1)    //B
    new_button = CURSOR_BACK;

  if(new_buttons & KEY_ID_11)   //Y
    new_button = button_cross;

  if(new_buttons & KEY_ID_8)    //R
    new_button = CURSOR_RTRIGGER;

  if(new_buttons & KEY_ID_9)    //L
    new_button = CURSOR_LTRIGGER;

  if(new_buttons & KEY_ID_12)   //Touch
    new_button = CURSOR_TOUCH;

  if(new_button != CURSOR_NONE)
  {
    button_repeat_timestamp = OSTimeGet();                  //Get system ticks
    button_repeat_state = BUTTON_HELD_INITIAL;
    button_repeat = new_buttons;
    cursor_repeat = new_button;
  }
  else
  {
    if(row_buttons & button_repeat)
    {
      u32 new_ticks;
      new_ticks = OSTimeGet();

      if(button_repeat_state == BUTTON_HELD_INITIAL)
      {
        if((new_ticks - button_repeat_timestamp) >
         BUTTON_REPEAT_START)
        {
          new_button = cursor_repeat;
          button_repeat_timestamp = new_ticks;
          button_repeat_state = BUTTON_HELD_REPEAT;
        }
      }

      if(button_repeat_state == BUTTON_HELD_REPEAT)
      {
        if((new_ticks - button_repeat_timestamp) >
         BUTTON_REPEAT_CONTINUE)
        {
          new_button = cursor_repeat;
          button_repeat_timestamp = new_ticks;
        }
      }
    }
  }

  return new_button;
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
