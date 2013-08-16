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

u32 tilt_sensor_x;
u32 tilt_sensor_y;

TOUCH_SCREEN last_touch= {0, 0};

#define PSP_ALL_BUTTON_MASK 0x1FFFF
u32 button_repeat = 0;
u32 sensorR;

static u32 button_circle;
static u32 button_cross;

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
