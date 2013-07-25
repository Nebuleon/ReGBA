#include "ds2_timer.h"
#include "ds2io.h"

#define KEY_REPEAT_TIME		200		//5 times/second
#define KEY_REPEAT_TIME_N	(KEY_REPEAT_TIME*1000000/SYSTIME_UNIT)

#define INPUT_REPEAT_TIME	100
#define INPUT_REPEAT_TIME_N (INPUT_REPEAT_TIME*1000000/SYSTIME_UNIT)

static unsigned int _last_key = 0;
static unsigned int _last_key_timestamp = 0;

/*
*   Function: only detect the key pressed or not
*/
unsigned int getKey(void)
{
	struct key_buf input;
	unsigned int new_key, hold_key, ret_key, timestamp;
	int flag;

	ds2_getrawInput(&input);
	timestamp = getSysTime();
	flag = ((timestamp - _last_key_timestamp) >= KEY_REPEAT_TIME_N) ? 1 : 0;

	input.key &= 0x3FFF;
	new_key = (_last_key ^ input.key) & input.key;
	hold_key = _last_key & input.key;

	ret_key = 0;
	if(hold_key) {
		if(flag)
		{
			ret_key = hold_key;
			_last_key_timestamp = timestamp;
		}
	}

	if(new_key)
	{
		ret_key |= new_key;
		_last_key_timestamp = timestamp;

		_last_key = input.key;
	}

	return ret_key;
}

/*
*   Function: can get the detail information about key pressed, hold, or release
*/
unsigned int getKey1(void)
{
	struct key_buf input;
	unsigned int new_key, hold_key, ret_key, timestamp;
	int flag;

	ds2_getrawInput(&input);
	timestamp = getSysTime();
	flag = ((timestamp - _last_key_timestamp) >= KEY_REPEAT_TIME_N) ? 1 : 0;

	input.key &= 0x3FFF;
	new_key = _last_key ^ input.key;
	hold_key = _last_key & input.key;
	//ret_key[31:16] = last key;
	//ret_key[15:0]	= new key;
	
	ret_key = 0;
	if(hold_key) {
		if(flag)
		{
			ret_key = (hold_key << 16) | hold_key;
			_last_key_timestamp = timestamp;
		}
	}

	if(new_key)
	{
		unsigned int tmp_key;

		//have new key pressed
		if(new_key & input.key)
		{
			ret_key |= new_key & input.key;
			_last_key_timestamp = timestamp;
		}
		else
		{
			ret_key |= (new_key & ~input.key) << 16;
		}

		_last_key = input.key;
	}

	return ret_key;
}

static unsigned int _last_input = 0;
static unsigned int _last_input_timestamp = 0;

/*
*   Function: only detect the key pressed or not and touch position
*/
unsigned int getInput(struct key_buf *input) {
    struct key_buf rawin;
    unsigned int timestamp, new, hold;
    int flag, ret;

	ds2_getrawInput(&rawin);
	timestamp = getSysTime();
	flag = ((timestamp - _last_input_timestamp) >= INPUT_REPEAT_TIME_N) ? 1 : 0;
    
    rawin.key &= 0x3FFF;
    new = (rawin.key ^ _last_input) & rawin.key;
    hold = rawin.key & _last_input;
    
    ret = 0;
    if(hold && flag)
    {
        ret = hold;
        input->x = rawin.x;
        input->y = rawin.y;
        
        _last_input_timestamp = timestamp;
    }
    
    if(new)
    {
        ret |= new;
        input->x = rawin.x;
        input->y = rawin.y;
        
        _last_input_timestamp = timestamp;
        _last_input = rawin.key;
    }
    
    input->key = ret;
    return ret > 0;
}





