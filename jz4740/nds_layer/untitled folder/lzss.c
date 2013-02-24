/*
 Simple LZSS implementation v1.0 by Jeremy Collake
 jeremy@collakesoftware.com
 http://www.collakesoftware.com 
  
 synopsis: 
 This is a very simple implementation of the LZSS 
 compression algorithm. I wrote it some time ago and
 have had it lying around, not going to much good use.
 So, I figured I'd make it public in hopes that someone 
 may be able to learn from it. Unfortunatly, my comments
 are non-existent.
 
 This implementation uses a 4095 (12 bit) window, with
 maximum phrase sizes of 17 bytes, and a minimum of 2 bytes,
 to allow storage within a nibble. Therefore, codewords are a 
 nice and easy 16bits. Lazy-evaluation is performed when 
 selecting the best phrase to encode. Control bits are 
 stored conveniently in groups of 8. The end of stream marker
 is indicated by a null index.

 Please remember that this code is by no means optimal, nor
 is it indended to be used for any data compression purposes.

*/
#include <bsp.h>
#include <jz4740.h>
#include <mipsregs.h>
#include "common.h"

#define CONTROL_LITERAL 0
#define CONTROL_CODEWORD 1

#define MAX_LENGTH 17
#define MAX_WINDOWSIZE 4095
#define ITERATIONS_BEFORE_CALLBACK 64

unsigned long DecompressData(unsigned char *src, unsigned char *dest);

unsigned long DecompressData(unsigned char *src, unsigned char *dest) {
	unsigned char control;
	unsigned int phrase_index,control_count=0;
	int phrase_length=0;
	unsigned char *dest_start=dest,*temp;
	unsigned short codeword=0;	
    unsigned char *dest_end;
    phrase_length = (*(src+1) <<0) + (*(src+2) <<8) + (*(src+3) <<16);
    //printf("phrase_length = %x\n",phrase_length);

    dest_end = dest_start + phrase_length;
    phrase_length = 0;
    src += 4;


	control=*src;
	src++;
	while(1) {				
		if((control>>7)==CONTROL_LITERAL) {						
			*dest=*src;
			dest++;
			src++;
		}		
		else { 								
			codeword=*src;			
			codeword<<=8;			
			codeword|=*(src+1);			
			phrase_index=codeword &0xfff;
			//if(!phrase_index) break;
			temp=dest-phrase_index -1;									
			for(phrase_length=((codeword>>12)+3);
				phrase_length>0;
				phrase_length--,temp++,dest++) {
					*dest=*temp;
			}
			src+=2;
		}
		control=control<<1;
		control_count++;
		if(control_count>=8) {
			control=*src++;
			//*src++;
			control_count=0;
		}
        if (dest >= dest_end )
        {
            break;
        }
	}
	return (unsigned long)(dest-dest_start)-1;
}
