	.set noreorder

#include "pspstub.s"

	STUB_START "homehook",0x40090000,0x00020005
	STUB_FUNC  0xDDC19F6B,initHomeButton
	STUB_FUNC  0x40DC34E4,readHomeButton
	STUB_END
