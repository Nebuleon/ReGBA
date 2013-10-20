#ifndef GCWSOUND_H_
#define GCWSOUND_H_

void init_sdlaudio();

// Modified by the audio thread to match VideoFastForwarded after it finds
// that the video thread has skipped a frame. The audio thread must deal
// with the condition as soon as possible.
extern volatile unsigned int AudioFastForwarded;

#define AUDIO_OUTPUT_BUFFER_SIZE 1476

// OUTPUT_SOUND_FREQUENCY should be a power-of-2 fraction of SOUND_FREQUENCY;
// if not, gcwsound.c's feed_buffer() needs to resample the output.
#define OUTPUT_SOUND_FREQUENCY 44100

#define OUTPUT_FREQUENCY_DIVISOR ((int) (SOUND_FREQUENCY) / (OUTPUT_SOUND_FREQUENCY))

#endif /* GCWSOUND_H_ */
