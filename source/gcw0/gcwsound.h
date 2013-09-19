#ifndef GCWSOUND_H_
#define GCWSOUND_H_

void init_sdlaudio();

extern int audio_discardflag;

#ifdef SOUND_TO_FILE
extern FILE* WaveFile;
#endif

#define AUDIO_OUTPUT_BUFFER_SIZE 1476

// OUTPUT_SOUND_FREQUENCY should be a power-of-2 fraction of SOUND_FREQUENCY;
// if not, gcwsound.c's feed_buffer() needs to resample the output.
#define OUTPUT_SOUND_FREQUENCY 44100

#define OUTPUT_FREQUENCY_DIVISOR ((int) (SOUND_FREQUENCY) / (OUTPUT_SOUND_FREQUENCY))

#endif /* GCWSOUND_H_ */
