#ifndef __DS2_CPU_H__
#define __DS2_CPU_H__

#ifdef __cplusplus
extern "C" {
#endif

//exception handle


//cache operationr
//invalidate instruction cache
extern void __icache_invalidate_all(void);

//invalidate data cache
extern void __dcache_invalidate_all(void);

//data cache writeback
extern void __dcache_writeback_all(void);

//data cache writeback and invalidate
extern void _dcache_wback_inv(unsigned long addr, unsigned long size);


//interruption operation
//clear CPU's interrupt state and enable global interrupt
extern void sti(void);

//disable global interrupt
extern void cli(void);

//disable global interrupt and store the global interrupt state
//return: interrupt state
extern unsigned int spin_lock_irqsave(void);

//restore global interrupt state
extern void spin_unlock_irqrestore(unsigned int val);


//CPU frequence
//There are 14 levels, 0 to 13, 13 level have the highest clock frequence
extern int ds2_setCPUclocklevel(unsigned int num);

//print colock frequence CPU
extern void printf_clock(void);

//delay n us
extern void udelay(unsigned int usec);

//delay n ms
extern void mdelay(unsigned int msec);

#ifdef __cplusplus
}
#endif

#endif //__DS2_CPU_H__
