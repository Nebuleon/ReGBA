extern int IS_WRITE_PCM;
void replay_test(char *fname)
{
  IS_WRITE_PCM=1;
  pcm_init();
//  record_play("tiankong.wav",IS_WRITE_PCM);
//  record_play("startup.wav",IS_WRITE_PCM);
//  record_play("ring.wav",IS_WRITE_PCM);
  while(1)
  {
    record_play(fname,IS_WRITE_PCM);
  }
  
}

void record_replay_test ()
{
  IS_WRITE_PCM=0;
  pcm_init();
  record_play("test.wav",IS_WRITE_PCM);

}
