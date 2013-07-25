#ifndef __KEY_H__
#define __KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

#define KEY_PRESS(key, key_id)	(!(key & (key_id<<16)) && (key & key_id))
#define KEY_HOLD(key, key_id)	((key & (key_id<<16)) && (key & key_id))
#define KEY_RELEASE(key, key_id)	((key & (key_id<<16)) && !(key & key_id))

/*
*   Function: only detect the key pressed or not
*/
unsigned int getKey(void);

/*
*   Function: can get the detail information about key pressed, hold, or release
*/
unsigned int getKey1(void);

/*
*   Function: detect the key pressed or not and touched position
*/
extern unsigned int getInput(struct key_buf *input);

#ifdef __cplusplus
}
#endif

#endif //__KEY_H__
