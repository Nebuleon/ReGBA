#ifndef GCWSOUND_H_
#define GCWSOUND_H_

void init_sdlaudio();

extern int audio_discardflag;

#ifdef SOUND_TO_FILE
extern FILE* WaveFile;
#endif

#define AUDIO_OUTPUT_BUFFER_SIZE 1476

#endif /* GCWSOUND_H_ */
