/*
   vplay.c - plays and records 
             CREATIVE LABS VOICE-files, Microsoft WAVE-files and raw data

   Autor:    Michael Beck - beck@informatik.hu-berlin.de
*/

#include "jz4740.h"

#include <stdio.h>
//#include <malloc.h>
//#include <unistd.h>
//#include <stdlib.h>
#include <string.h>
//#include <fcntl.h>
//#include <sys/ioctl.h>
#include "fs_api.h"
#include <sys/soundcard.h>
#include "fmtheaders.h"
//#include <netinet/in.h>
#include "pcm.h"

#define DEFAULT_DSP_SPEED 	48000

#define RECORD	0
#define PLAY	1
//#define AUDIO "/dev/dsp"

#define min(a,b) 		((a) <= (b) ? (a) : (b))
#define d_printf(x)        if (verbose_mode) fprintf x

#define VOC_FMT			0
#define WAVE_FMT		1
#define RAW_DATA		2
#define SND_FMT			3
/* global data */

int timelimit = 0, dsp_speed = DEFAULT_DSP_SPEED, dsp_stereo = 0;
int samplesize   = 8;
int quiet_mode   = 0;
int verbose_mode = 0;
int convert      = 0;
int record_type  = VOC_FMT;
u_long count;
int audio, abuf_size, zbuf_size;
int direction, omode;
unsigned char *audiobuf, *zerobuf;
unsigned char audiobuf1[0x1000 * 16];
extern int pcm_ioctl(unsigned int cmd, unsigned long arg);
char *command;
int vocminor, vocmajor;		/* .VOC version */
FS_FILE *myfile;

/* defaults for playing raw data */
;
struct {
  int timelimit, dsp_speed, dsp_stereo, samplesize;
} raw_info = { 0, DEFAULT_DSP_SPEED, 0, 8 };

/* needed prototypes */

void record_play (char *name,int dir);
void start_voc (int fd, u_long count);
void start_wave (int fd, u_long count);
void start_snd (int fd, u_long count);
void end_voc (int fd);
void end_wave_raw (int fd);
void end_snd (int fd);
u_long calc_count ();

struct fmt_record {
  void (*start) (int fd, u_long count);
  void (*end) (int fd);
  char *what;
} fmt_rec_table[] = {
  { start_voc,  end_voc,      "VOC" },
  { start_wave, end_wave_raw, "WAVE" },
  { NULL,       end_wave_raw, "raw data" },
  { start_snd,  end_snd,      "SND" }
}; 

extern int IS_WRITE_PCM;
/*
 test, if it is a .VOC file and return >=0 if ok (this is the length of rest)
                                       < 0 if not 
*/
int test_vocfile(void *buffer)
{
  VocHeader *vp = buffer;
  if (!strcmp((const char *)vp->magic, MAGIC_STRING) ) {
    vocminor = vp->version & 0xFF;
    vocmajor = vp->version / 256;
    if (vp->version != (0x1233 - vp->coded_ver) )
      return -2;				/* coded version mismatch */
    return vp->headerlen - sizeof(VocHeader);	/* 0 mostly */
  }
  return -1;					/* magic string fail */
}

/*
 test, if it's a .WAV file, 0 if ok (and set the speed, stereo etc.)
                          < 0 if not
*/
int test_wavefile(void *buffer)
{
  WaveHeader *wp = buffer;
 
  if (wp->main_chunk == RIFF && wp->chunk_type == WAVE &&
      wp->sub_chunk == FMT && wp->data_chunk == DATA) {
    if (wp->format != PCM_CODE) {
      fprintf (stderr, "%s: can't play not PCM-coded WAVE-files\n", command);
      exit (-1);
    }
    if (wp->modus > 2) {
      fprintf (stderr, "%s: can't play WAVE-files with %d tracks\n",
               command, wp->modus);
      exit (-1);
    }
    dsp_stereo = (wp->modus == WAVE_STEREO) ? 1 : 0;
    samplesize = wp->bit_p_spl;
    dsp_speed = wp->sample_fq; 
    count = wp->data_length;
    return 0;
  }
  return -1;
}

/*
* test, if it's a .SND file, 0 if ok (and set the speed, stereo etc.)
*                          < 0 if not
*/
int test_sndfile(void *buffer, int fd)
{
  long infolen;
  char *info;
  SndHeader *snd = buffer;

  if (snd->magic == SND_MAGIC)
    convert = 0;
  else{
    if( htonl(snd->magic) == SND_MAGIC){
      convert = 1;
      snd->dataLocation = htonl(snd->dataLocation);
      snd->dataSize = htonl(snd->dataSize);
      snd->dataFormat = htonl(snd->dataFormat);
      snd->samplingRate = htonl(snd->samplingRate);
      snd->channelCount = htonl(snd->channelCount);
    }
    else{
      /* No SoundFile */
      return(-1);
    }     
  }     
  switch (snd->dataFormat){
  case SND_FORMAT_LINEAR_8:
    samplesize = 8;
    break;
  case SND_FORMAT_LINEAR_16:
    samplesize = 16;
    break;
  default:
    fprintf (stderr, "%s: Unsupported SND_FORMAT\n", command);
    exit (-1);
  }
  
  dsp_stereo = (snd->channelCount == 2) ? 1 : 0;
  dsp_speed = snd->samplingRate; 
  count = snd->dataSize;
  
  /* read Info-Strings */
  infolen = snd->dataLocation - sizeof(SndHeader);
  info = (char *) malloc(infolen);
  read(fd, info, infolen);
  if(!quiet_mode)
    fprintf(stderr, "SoundFile Info: %s\n", info);
  free(info);

  return 0;
}

 
/*
  writing zeros from the zerobuf to simulate silence,
  perhaps it's enough to use a long var instead of zerobuf ?
*/
void write_zeros (unsigned x)
{
  unsigned l;
 
  while (x) {
    l = min (x, zbuf_size);
    if (write (audio, (char *)zerobuf, l) != l) {
//      perror (AUDIO);
      exit (-1);
    }
    x -= l;
  }
} 

/* if need a SYNC, 
   (is needed if we plan to change speed, stereo ... during output)
*/
void 
sync_dsp(void)
{
#if 0
  if (ioctl (audio, SNDCTL_DSP_SYNC, NULL) < 0) {
    perror(AUDIO);
    exit (-1);
  }
#endif
}

/* setting the speed for output */
void set_dsp_speed (int *dsp_speed)
{

    if (ioctl(audio, SNDCTL_DSP_SPEED, dsp_speed) < 0) {
// if (pcm_ioctl(PCM_SET_SAMPLE_RATE, 16000) < 0) {
    fprintf (stderr, "%s: unable to set audio speed\n", command);
//    perror (AUDIO);
    exit (-1);
  }
}

/* if to_mono: 
   compress 8 bit stereo data 2:1, needed if we want play 'one track';
   this is usefull, if you habe SB 1.0 - 2.0 (I have 1.0) and want
   hear the sample (in Mono)
   if to_8:
   compress 16 bit to 8 by using hi-byte; wave-files use signed words,
   so we need to convert it in "unsigned" sample (0x80 is now zero)

   WARNING: this procedure can't compress 16 bit stereo to 16 bit mono,
            because if you have a 16 (or 12) bit card you should have
            stereo (or I'm wrong ?? )
   */
unsigned long 
one_channel(unsigned char *buf,unsigned long l, char to_mono, char to_8)
{
  register unsigned char *w  = buf;
  register unsigned char *w2 = buf;
  char ofs = 0;
  unsigned long incr = 0; 
  unsigned long c, ret;
  printf("on channel---\n");
  if (to_mono) 
    ++incr;
  if (to_8) 
  {
    ++incr; 
	++w2; 
	ofs = 128; 
  }
  ret = c = l >> incr;
  incr = incr << 1;
 printf("two channel---\n");
  while (c--) 
  {
    *w++ = *w2 + ofs; 
	w2 += incr;
  }
printf("two2 channel---\n");
  return ret;
}

/*
  ok, let's play a .voc file
*/ 
void vplay (int fd, int ofs, char *name)
{
  int l, real_l;
  BlockType *bp;
  Voice_data *vd;
  Ext_Block *eb;
  u_long nextblock, in_buffer;
  u_char *data = audiobuf;
  char was_extended = 0, output = 0;
  u_short *sp, repeat = 0;
  u_long silence;
  int filepos = 0;
  char one_chn = 0;

#define COUNT(x)	nextblock -= x; in_buffer -=x ;data += x

  /* first SYNC the dsp */
  sync_dsp();
 
  if (!quiet_mode) 
    fprintf (stderr, "Playing Creative Labs Voice file ...\n");

  /* first we waste the rest of header, ugly but we don't need seek */
  while (ofs > abuf_size) {	
    read (fd, (char *)audiobuf, abuf_size);
    ofs -= abuf_size;
  }
  if (ofs)
    read (fd, audiobuf, ofs);

  /* .voc files are 8 bit (now) */
  samplesize = VOC_SAMPLESIZE;
  ioctl(audio, SNDCTL_DSP_SAMPLESIZE, &samplesize);
  if (samplesize != VOC_SAMPLESIZE) {
    fprintf(stderr, "%s: unable to set 8 bit sample size!\n", command);
    exit (-1);
  }
  /* and there are MONO by default */
  dsp_stereo = MODE_MONO;
  ioctl(audio, SNDCTL_DSP_STEREO, &dsp_stereo);

  in_buffer = nextblock = 0;
  while (1) {
    Fill_the_buffer:		/* need this for repeat */
    if ( in_buffer < 32 ) {
      /* move the rest of buffer to pos 0 and fill the audiobuf up */
      if (in_buffer)
        memcpy ((char *)audiobuf, data, in_buffer);
      data = audiobuf;
      if ((l = read (fd, (char *)audiobuf + in_buffer, abuf_size - in_buffer) ) > 0) 
        in_buffer += l;
      else if (! in_buffer) {
	/* the file is truncated, so simulate 'Terminator' 
           and reduce the datablock for save landing */
        nextblock = audiobuf[0] = 0;
        if (l == -1) {
          perror (name);
          exit (-1);
        }
      }
    }
    while (! nextblock) { 	/* this is a new block */
      bp = (BlockType *)data; COUNT(sizeof (BlockType));
      nextblock = DATALEN(bp);
      if (output && !quiet_mode)
        fprintf (stderr, "\n");	/* write /n after ASCII-out */
      output = 0;
      switch (bp->type) {
        case 0:
          d_printf ((stderr, "Terminator\n"));
          return;		/* VOC-file stop */
        case 1:
          vd = (Voice_data *)data; COUNT(sizeof(Voice_data));
          /* we need a SYNC, before we can set new SPEED, STEREO ... */
          sync_dsp();

          if (! was_extended) {
            dsp_speed = (int)(vd->tc);
            dsp_speed = 1000000 / (256 - dsp_speed); 
            d_printf ((stderr, "Voice data %d Hz\n", dsp_speed));
            if (vd->pack) {	/* /dev/dsp can't it */
              fprintf (stderr, "%s: can't play packed .voc files\n", command);
              return;
            }
            if (dsp_stereo) {	/* if we are in Stereo-Mode, switch back */
              dsp_stereo = MODE_MONO;
              ioctl(audio, SNDCTL_DSP_STEREO, &dsp_stereo); 
            }
          }
          else {		/* there was extended block */
            if (one_chn)	/* if one Stereo fails, why test another ? */
              dsp_stereo = MODE_MONO;
            else if (dsp_stereo) {	/* want Stereo */
              /* shit, my MACRO dosn't work here */
#ifdef SOUND_VERSION
              if (ioctl(audio, SNDCTL_DSP_STEREO, &dsp_stereo) < 0) {
#else
              if (dsp_stereo != ioctl(audio, SNDCTL_DSP_STEREO, dsp_stereo)) { 
#endif
                dsp_stereo = MODE_MONO; 
                fprintf (stderr, "%s: can't play in Stereo; playing only one channel\n",
                         command);
                one_chn = 1;
              }
            }
            was_extended = 0;
          }
          set_dsp_speed (&dsp_speed);
          break;
        case 2:			/* nothing to do, pure data */
          d_printf ((stderr, "Voice continuation\n"));
          break;
        case 3:			/* a silence block, no data, only a count */
          sp = (u_short *)data; COUNT(sizeof(u_short));
          dsp_speed = (int)(*data); COUNT(1);
          dsp_speed = 1000000 / (256 - dsp_speed);
          sync_dsp();
          set_dsp_speed (&dsp_speed);
          silence = ( ((u_long)*sp) * 1000) / dsp_speed; 
          d_printf ((stderr, "Silence for %ld ms\n", silence));
          write_zeros (*sp);
          break;
        case 4:			/* a marker for syncronisation, no effect */
          sp = (u_short *)data; COUNT(sizeof(u_short));
          d_printf ((stderr, "Marker %d\n", *sp)); 
          break;
        case 5:			/* ASCII text, we copy to stderr */
          output = 1;
          d_printf ((stderr, "ASCII - text :\n"));
          break; 
        case 6:			/* repeat marker, says repeatcount */
          /* my specs don't say it: maybe this can be recursive, but
             I don't think somebody use it */
          repeat = *(u_short *)data; COUNT(sizeof(u_short));
          d_printf ((stderr, "Repeat loop %d times\n", repeat));
          if (filepos >= 0)	/* if < 0, one seek fails, why test another */
            if ( (filepos = lseek (fd, 0, 1)) < 0 ) {
              fprintf(stderr, "%s: can't play loops; %s isn't seekable\n", 
                      command, name);
              repeat = 0;
            }
            else
              filepos -= in_buffer;	/* set filepos after repeat */
          else
            repeat = 0;
          break;
        case 7:			/* ok, lets repeat that be rewinding tape */
          if (repeat) {
            if (repeat != 0xFFFF) {
              d_printf ((stderr, "Repeat loop %d\n", repeat));
              --repeat;
            }
            else
              d_printf ((stderr, "Neverending loop\n"));
            lseek (fd, filepos, 0);
            in_buffer = 0;		/* clear the buffer */
            goto Fill_the_buffer;
          }
          else
            d_printf ((stderr, "End repeat loop\n"));
          break;
        case 8:			/* the extension to play Stereo, I have SB 1.0 :-( */
          was_extended = 1;
          eb = (Ext_Block *)data; COUNT(sizeof(Ext_Block));
          dsp_speed = (int)(eb->tc);
          dsp_speed = 256000000L / (65536 - dsp_speed);
          dsp_stereo = eb->mode;
          if (dsp_stereo == MODE_STEREO) 
            dsp_speed = dsp_speed >> 1;
          if (eb->pack) {     /* /dev/dsp can't it */
            fprintf (stderr, "%s: can't play packed .voc files\n", command);
            return;
          }
          d_printf ((stderr, "Extended block %s %d Hz\n", 
				(eb->mode ? "Stereo" : "Mono"), dsp_speed));
          break;
        default:
          fprintf (stderr, "%s: unknown blocktype %d. terminate.\n", 
                   command, bp->type);
          return;
      } 		/* switch (bp->type) */
    }			/* while (! nextblock)  */
    /* put nextblock data bytes to dsp */
    l = min (in_buffer, nextblock);
    if (l) {  
      if (output && !quiet_mode)
        write (2, data, l);	/* to stderr */
      else {
        real_l = one_chn ? one_channel(data, l, one_chn, 0) : l;
        if (write (audio, data, real_l) != real_l) {
//          perror (AUDIO);
          exit(-1);
        }
      }
      COUNT(l);
    }
  }			/* while(1) */
}
/* that was a big one, perhaps somebody split it :-) */

/* setting the globals for playing raw data */
void init_raw_data(void)
{
  timelimit  = raw_info.timelimit;
  dsp_speed  = raw_info.dsp_speed;
  dsp_stereo = raw_info.dsp_stereo;
  samplesize = raw_info.samplesize;
}

/* calculate the data count to read from/to dsp */
u_long calc_count(void)
{
  u_long count;

  if (!timelimit)
    count = 0x7fffffff;
  else {
    count = timelimit * dsp_speed;
    if (dsp_stereo)
      count *= 2;
    if (samplesize != 8)
      count *= 2;
  }
  return count;
}

/* write a .VOC-header */ 
void start_voc(int fd, u_long cnt)
{
  VocHeader  vh;
  BlockType  bt;
  Voice_data vd;
  Ext_Block  eb;
 
  strcpy((char *)vh.magic,MAGIC_STRING);
  vh.headerlen = sizeof(VocHeader);
  vh.version = ACTUAL_VERSION;
  vh.coded_ver = 0x1233 - ACTUAL_VERSION;

  write (fd, &vh, sizeof(VocHeader));

  if (dsp_stereo) {
    /* write a extended block */
    bt.type = 8;
    bt.datalen = 4;
    bt.datalen_m = bt.datalen_h = 0;
    write (fd, &bt, sizeof(BlockType));
    eb.tc = (u_short)(65536 - 256000000L / (dsp_speed << 1));
    eb.pack = 0;
    eb.mode = 1;
    write (fd, &eb, sizeof(Ext_Block));
  }
  bt.type = 1;
  cnt += sizeof(Voice_data);	/* Voice_data block follows */
  bt.datalen   = (u_char)  (cnt & 0xFF);
  bt.datalen_m = (u_char)( (cnt & 0xFF00) >> 8 );
  bt.datalen_h = (u_char)( (cnt & 0xFF0000) >> 16 );
  write (fd, &bt, sizeof(BlockType));
  vd.tc = (u_char)(256 - (1000000 / dsp_speed) );
  vd.pack = 0;
  write (fd, &vd, sizeof(Voice_data) );
} 

/* write a WAVE-header */
void start_wave(int fd, u_long cnt)
{
  WaveHeader wh;

  wh.main_chunk = RIFF;
  wh.length     = cnt + sizeof(WaveHeader) - 8; 
  wh.chunk_type = WAVE;
  wh.sub_chunk  = FMT;
  wh.sc_len     = 16;
  wh.format     = PCM_CODE;
  wh.modus      = dsp_stereo ? 2 : 1;
  wh.sample_fq  = dsp_speed;
  wh.byte_p_spl = ((samplesize == 8) ? 1 : 2) * (dsp_stereo ? 2 : 1);
  wh.byte_p_sec = dsp_speed * wh.modus * wh.byte_p_spl;
  wh.bit_p_spl  = samplesize;
  wh.data_chunk = DATA;
  wh.data_length= cnt;
  write (fd, &wh, sizeof(WaveHeader));
}

/* closing .VOC */
void end_voc(int fd)
{
  char dummy = 0;		/* Write a Terminator */
  write (fd, &dummy, 1);
  if (fd != 1)
    close (fd);
}

void end_wave_raw(int fd)
{				/* only close output */
  if (fd != 1)
    close (fd);
}

void start_snd (int fd, u_long count)
{
  SndHeader snd;
  char *sndinfo = "Recorded by vrec\000";

  snd.magic = SND_MAGIC;
  snd.dataLocation = sizeof(SndHeader) + strlen(sndinfo);
  snd.dataSize = count;
  switch(samplesize){
  case 8:
    snd.dataFormat = SND_FORMAT_LINEAR_8;
    break;
  case 16:
    snd.dataFormat = SND_FORMAT_LINEAR_16;
    break;
  default:
    fprintf(stderr,"%d bit: unsupported sample size for NeXt sound file!\n", 
	    samplesize);
    exit (-1);
  }
  snd.samplingRate = dsp_speed;
  snd.channelCount = dsp_stereo ? 2 : 1;

  write(fd, &snd, sizeof(SndHeader));
  write(fd, sndinfo, strlen(sndinfo));
}

void end_snd (int fd)
{
  if (fd != 1)
    close (fd);
}


/* playing/recording raw data, this proc handels WAVE files and
   recording .VOCs (as one block) */ 
unsigned char recram[1024 * 1024 * 6];
void recplay (int fd, int loaded, u_long count, int rtype, char *name)
{
  int l= 0, real_l= 0;
  u_long c;
  char one_chn = 0;
  char to_8 = 0;
  u_long recCount = 0;
  abuf_size = 1024 * 8;
  sync_dsp();
    if (direction == PLAY) {
	  if(rtype == WAVE_FMT){
		   pcm_ioctl(PCM_SET_SAMPLE_RATE,dsp_speed);
//		   pcm_ioctl(PCM_SET_SAMPLE_RATE, 22050);
		   printf("Speed= %d\n",REG_ICDC_CDCCR2);
		   
		  if(dsp_stereo==0)
			  pcm_ioctl(PCM_SET_CHANNEL, 1);
          else
			  pcm_ioctl(PCM_SET_CHANNEL, 2);
		  
		  if(samplesize == 8 )
			  pcm_ioctl(PCM_SET_FORMAT, AFMT_U8);
		  else
			  pcm_ioctl(PCM_SET_FORMAT,  AFMT_S16_LE);
	  } else {
		  pcm_ioctl(PCM_SET_SAMPLE_RATE,48000);
	  }
  }

 if (!quiet_mode) {
	 printf ("%s %s : ", 
		  (direction == PLAY) ? "Playing" : "Recording",
		  fmt_rec_table[rtype].what);
	 if (samplesize != 8)
		 printf( "%d bit, ", samplesize);
	 printf ( "Speed %d Hz ", dsp_speed);
	 printf ( "%d bits ", samplesize);
	 printf ( "%s ...\n", dsp_stereo ? "Stereo" : "Mono");
 }
 
//    count = 0x1000 * 2500;
    printf("count= %lx\n", count);
  if (direction == PLAY) {
	   abuf_size = 1470*2*4;//0x1000;// * 2;
	  while (count) {
		  c = count;
		  if (c > abuf_size)
			  	  c = abuf_size;
          
//          udelay(10000);
		  if ( FS_FRead(audiobuf,1,c,myfile) > 0 ) {
			  l += c; loaded = 0;	/* correct the count; ugly but ... */
			  
			  int flag= 0;
			  while(!flag)
			  {
			    flag= pcm_write(audiobuf,c);       
			  }
//			  if (pcm_write(audiobuf,c+1) == -1) {
//				  return -1;
//			  }
			  count -= c;
		  }
		  else{
			  if (l == -1) { 
//				  perror (name);
//				  exit (-1);
			  }
			  count = 0;	/* Stop */
		  }
		  
		  printf("count= %lx\n", count);
	  }			/* while (count) */
	  printf("Play over\n");

#if 0	  
	  count = 0x1000 * 2500;
    printf("count= %lx\n", count);
    pcm_ioctl(PCM_SET_SAMPLE_RATE,dsp_speed);
	  
	  abuf_size = 0x1000;// * 2;
	  while (count) {
		  c = count;
		  if (c > abuf_size)
			  	  c = abuf_size;
          
//          udelay(10000);
		  if ( FS_FRead(audiobuf,1,c,myfile) > 0 ) {
			  l += c; loaded = 0;	/* correct the count; ugly but ... */
			  
			  int flag= 0;
			  while(!flag)
			  {
			    flag= pcm_write(audiobuf,c);       
			  }
//			  if (pcm_write(audiobuf,c+1) == -1) {
//				  return -1;
//			  }
			  count -= c;
		  }
		  else{
			  if (l == -1) { 
				  perror (name);
				  exit (-1);
			  }
			  count = 0;	/* Stop */
		  }
		  
		  printf("count= %lx\n", count);
	  }			/* while (count) */
	  printf("Play over\n");
#endif	  
  } else {		/* we are recording */
	 abuf_size = 1024 * 4 * 2;
	 while (count) {
		 c = count;
		 if (c > abuf_size)
		 	 c = abuf_size;
		 #if 0
		 //if ((l = pcm_read ((char *)audiobuf, c)) > 0) {
		 if ((l = pcm_read ((char *)(recram + recCount), c)) > 0) {
//	 	 printf("read daata--%x-%x-%x-%x!\n",recram[recCount-4],recram[recCount-3],recram[recCount-2],recram[recCount-1]);
			 recCount += l;
			 if(recCount > 1 * 1024 * 1024 )
				 break;
            #if 0
			 if (FS_FWrite (audiobuf,1,l,myfile) != l) {
				 printf("can not write to File %s!!",fd);
			
			 }
			 #endif
			 count -= l;
			 
		 }
		 #endif

		 printf("record audio start\r\n");
		 
		 l = pcm_read((char *)recram, 1024 * 1024);
		 recCount = l;
		 
		 count = 0; 
		 if (l == -1) {
			 printf("Error write to File %s!!",fd);

		 }
	 }			/* While count */
	 IS_WRITE_PCM=1;
	 pcm_init();
	 l=0;
	 printf("Start palying voice ...\n");
	 pcm_write(recram,recCount);
	 printf("Finish palying voice!\n");

    }
    
    
    
}

/*
  let's play or record it (record_type says VOC/WAVE/raw)
*/

void record_play(char *name,int dir)
{
  int fd, ofs;
  audiobuf = (unsigned char*) audiobuf1;

  if(dir==1)
	  direction = PLAY;
  else
	  direction = RECORD; 
  if (direction == PLAY) {
 	  myfile = FS_FOpen(name,"r");
	  printf("file open %x\r\n",myfile); 
	  if (myfile) {
		  /* read SND-header */
		  FS_FRead(audiobuf,1,sizeof(WaveHeader),myfile);
		  /* read bytes for WAVE-header */

		  if (test_wavefile (audiobuf) >= 0)
		  {
			  recplay (myfile,0, count, WAVE_FMT, name);
		  }
		  else {
			  /* should be raw data */
			  init_raw_data();
			  count = calc_count();
			  recplay (fd, sizeof(WaveHeader), count, RAW_DATA, name);
		  }
	  }
	  
	  if (myfile != 0)
		  FS_FClose(myfile);   
  }
  else {		/* recording ... */
	  if (!name) {
		  fd = 1;
		  name = "stdout";
	  }
	  else {
		  //	  myfile = FS_FOpen(name,"wb");
		  //		  if (!myfile) {
		  //			  printf("can not open %s",name);
		  //	  }
	  }
	  count = calc_count() & 0xFFFFFFFE;
	  /* WAVE-file should be even (I'm not sure), but wasting one byte
	     isn't a problem (this can only be in 8 bit mono) */
	  // if (fmt_rec_table[record_type].start)
	  //	  fmt_rec_table[record_type].start(fd, count);
	  recplay (myfile, sizeof(WaveHeader), count, record_type, name);
	  //fmt_rec_table[record_type].end(fd);
  }
} 

