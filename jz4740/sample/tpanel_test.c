/*  JZ4740 LYRA board touch panel test sample.
 *
 *  Copyright (C) 2006 - 2007 Ingenic Semiconductor Inc.
 *
 *  Author: <zyliu@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
//#include <bsp.h>
//#include <jz4740.h>
//#include <key.h>

//extern PFN_KEYHANDLE TPanelUpKey;
//extern PFN_KEYHANDLE TPanelDownKey;

/*void TPanelUpKeyTest(int key)
{
	printf("UpKeyTest 0x%x\r\n",key);
}
void TPanelDownKeyTest(int key)
{
	printf("DownKeyTest 0x%x\r\n",key);
}
*/

void TPanelTest()
{
    int i;  
//	TPanelUpKey = TPanelUpKeyTest;
//	TPanelDownKey = TPanelDownKeyTest;
	TPanelInit();
}

