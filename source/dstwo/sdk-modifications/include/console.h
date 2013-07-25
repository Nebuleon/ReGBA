#ifndef __CONSOLE_H__
#define __CONSOLE_H__
#include "ds2io.h"

extern int ConsoleInit(unsigned short front_color, unsigned short background_color, enum SCREEN_ID screen, unsigned int buf_size);

extern int cprintf(const char *format, ...);

#endif //__CONSOLE_H__
