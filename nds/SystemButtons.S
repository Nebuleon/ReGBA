	.set noreorder

#include "pspstub.s"

	STUB_START "SystemButtons",0x40090000,0x000B0005
	STUB_FUNC  0x494C24C0,initSystemButtons
	STUB_FUNC  0xC3E44941,readSystemButtons
	STUB_FUNC  0x40DC34E4,readHomeButton
	STUB_FUNC  0x1BED91EE,readVolumeButtons
	STUB_FUNC  0x4E7C27BF,readVolUpButton
	STUB_FUNC  0x8F7FA8C3,readVolDownButton
	STUB_FUNC  0x52DF22A1,readNoteButton
	STUB_FUNC  0xE6884A40,readScreenButton
	STUB_FUNC  0x32A5C599,readHoldSwitch
	STUB_FUNC  0x3337F3E1,readWLANSwitch
	STUB_FUNC  0x8DDE496C,readMainVolume
	STUB_END
