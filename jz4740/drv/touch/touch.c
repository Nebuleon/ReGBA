#include <jztouch.h>
#if (TOUCH == 1)
#include <ak4182.c>
#elif (TOUCH == 2)
#include <sadc.c>
#endif
