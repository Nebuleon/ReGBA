#ifndef __KEY_H__
#define __KEY_H__

typedef void (*PFN_KEYHANDLE)(unsigned int key);

#define KEY_PIN (32*3 + 0)
#define KEY_NUM 6
#define KEY_MASK 0x03f;

#endif //__KEY_H__
