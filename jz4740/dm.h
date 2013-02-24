#ifndef __DM_H
#define __DM_H
//#define DRIVER_MANAGER 1
#if (DM==1)
struct dm_jz4740_t{
	unsigned char name;
	int (*init)(void);
	int (*poweron)(void);
	int (*poweroff)(void);
	int (*convert)(void);
	int (*preconvert)(void);
};
#endif
#endif	/* __CONFIG_H */
