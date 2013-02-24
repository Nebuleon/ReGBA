#include <bsp.h>
#include <jz4740.h>
#include <key.h>

extern PFN_KEYHANDLE UpKey;
extern PFN_KEYHANDLE DownKey;

void UpKeyTest(int key)
{
//	printf("UpKeyTest 0x%x\r\n",key);
    __asm__ __volatile__("syscall 0\n\t");
}
void DownKeyTest(int key)
{
//	printf("DownKeyTest 0x%x\r\n",key);
}


void KeyTest()
{
    int i;  
	UpKey = UpKeyTest;
	DownKey = DownKeyTest;
	KeyInit();
	
}

