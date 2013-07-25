#ifndef _DS2_CPUCLOCK_H__
#define _DS2_CPUCLOCK_H__


#define CPU_MAX_LEVEL_EX 18

#ifdef __cplusplus
extern "C" {
#endif

extern int ds2_getCPUClock(void);
extern int ds2_setCPULevel(unsigned int level);
extern void udelayOC(unsigned int usec);
extern void mdelayOC(unsigned int msec);

//#define ds2_setCPUclocklevel ds2_setCPULevel

#ifdef __cplusplus
}
#endif

#endif //__DS2_CPUCLOCK_H__
