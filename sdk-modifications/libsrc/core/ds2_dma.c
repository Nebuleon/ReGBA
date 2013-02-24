#include "ds2_dma.h"



//register a DMA transfer request
//ch: channel id request, there are 6 channles, 
//irq_handler: the DMA interruption handle
//arg: argument to the handle
//mode: DMA mode, such as port width, address increased/fixed, and so on
//type: DMA request type
extern int dma_request(int ch, void (*irq_handler)(unsigned int), unsigned int arg,
     unsigned int mode, unsigned int type);

//start DMA transfer, must request a DMA first
//ch: channel id
//srcAddr: DMA source address
//dstAddr: DMA destination address
//count: DMA transfer count, the total bytes due the mode in dma_request
extern void dma_start(int ch, unsigned int srcAddr, unsigned int dstAddr,
         unsigned int count);


int _dmaCopy(int ch, void *dest, void *src, unsigned int size, unsigned int flags){
  int test = 0;
  if(!(test = dma_request(ch, 0, 0,
                        //increment dest addr
                        DMAC_DCMD_DAI | flags,
                        //auto request type
                        DMAC_DRSR_RS_AUTO)))
  {
    dma_start(ch, (unsigned int)src, (unsigned int)dest, size);
  }
  return test;
}
