/******************************************************************************

	SystemButtons.prx

******************************************************************************/

#ifndef SYSTEMBUTTONS_PRX_H
#define SYSTEMBUTTONS_PRX_H

#ifdef __cplusplus
extern "C" {
#endif

void initSystemButtons(int devkit_version);
unsigned int readSystemButtons(void);
unsigned int readHomeButton(void);
unsigned int readVolumeButtons(void);
unsigned int readVolUpButton(void);
unsigned int readVolDownButton(void);
unsigned int readNoteButton(void);
unsigned int readScreenButton(void);
unsigned int readHoldSwitch(void);
unsigned int readWLANSwitch(void);
int readMainVolume(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEMBUTTONS_PRX_H */
