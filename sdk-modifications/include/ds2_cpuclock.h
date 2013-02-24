#ifndef _DS2_CPUCLOCK_H__
#define _DS2_CPUCLOCK_H__


#define CPU_MAX_LEVEL_EX 18

#ifdef __cplusplus
extern "C" {
#endif

extern int ds2_getCPUClock(void);
extern int ds2_setCPULevel(unsigned int level);
extern void ds2_udelay(unsigned int usec);
extern void ds2_mdelay(unsigned int msec);


#ifdef __cplusplus
}
#endif

#endif //__DS2_CPUCLOCK_H__
