#include <bsp.h>
#include <jz4740.h>
#include <mipsregs.h>
//#include <jzfs_api.h>
#include "common.h"
#include"test.h"
#include "test_bmp.h"

#define BI_RGB (0)
#define BI_RLE8 (1)
#define BI_RLE4 (2)
#define BI_Bitfields (3)
//#define false 0
//#define true 1
//#define RGB15(r,g,b)  ((r)|((g)<<5)|((b)<<10))

typedef struct {
  u8 bfType[2];
  u32 bfSize;
  u16 bfReserved1;
  u16 bfReserved2;
  u32 bfOffset;
  u32 biSize;
  u32 biWidth;
  u32 biHeight;
  u16 biPlanes;
  u16 biBitCount;
  u32 biCopmression;
  u32 biSizeImage;
  u32 biXPixPerMeter;
  u32 biYPixPerMeter;
  u32 biClrUsed;
  u32 biCirImportant;
  u8 *pPalette;
  u8 *pBitmap;
  
  u32 DataWidth;
} TBMPHeader;
u16 kkk[(192*256*2)/2+512];
static char *BMP_LoadErrorStr;
static u16 GetVariable16bit(void *pb)
{
  u16 res;
  u8 *pb8=(u8*)pb;
  
  res=(u32)pb8[0] << 0;
  res+=(u32)pb8[1] << 8;
  
  return(res);
}

static u32 GetVariable32bit(void *pb)
{
  u32 res;
  u8 *pb8=(u8*)pb;
  
  res=(u32)pb8[0] << 0;
  res+=(u32)pb8[1] << 8;
  res+=(u32)pb8[2] << 16;
  res+=(u32)pb8[3] << 24;
  
  return(res);
}


int GetBMPHeader(u8 *pb,TBMPHeader *pBMPHeader)
{
  if(pb==NULL){
    BMP_LoadErrorStr="SourceData Null.";
    return(false);
  }
  if(pBMPHeader==NULL){
    BMP_LoadErrorStr="pBMPHeader Null.";
    return(false);
  }
  
  pBMPHeader->bfType[0]=pb[0];
  pBMPHeader->bfType[1]=pb[1];
  pBMPHeader->bfSize=GetVariable32bit(&pb[2]);
  pBMPHeader->bfReserved1=GetVariable16bit(&pb[6]);
  pBMPHeader->bfReserved2=GetVariable16bit(&pb[8]);
  pBMPHeader->bfOffset=GetVariable32bit(&pb[10]);
  pBMPHeader->biSize=GetVariable32bit(&pb[14+0]);
  pBMPHeader->biWidth=GetVariable32bit(&pb[14+4]);
  pBMPHeader->biHeight=GetVariable32bit(&pb[14+8]);
  pBMPHeader->biPlanes=GetVariable16bit(&pb[14+12]);
  pBMPHeader->biBitCount=GetVariable16bit(&pb[14+14]);
  pBMPHeader->biCopmression=GetVariable32bit(&pb[14+16]);
  pBMPHeader->biSizeImage=GetVariable32bit(&pb[14+20]);
  pBMPHeader->biXPixPerMeter=GetVariable32bit(&pb[14+24]);
  pBMPHeader->biYPixPerMeter=GetVariable32bit(&pb[14+28]);
  pBMPHeader->biClrUsed=GetVariable32bit(&pb[14+32]);
  pBMPHeader->biCirImportant=GetVariable32bit(&pb[14+36]);
  
  pBMPHeader->pPalette=&pb[14+40];
  pBMPHeader->pBitmap=&pb[pBMPHeader->bfOffset];
  
  pBMPHeader->DataWidth=0;
  
  if((pBMPHeader->bfType[0]!='B')||(pBMPHeader->bfType[1]!='M')){
    BMP_LoadErrorStr="Error MagicID!=BM";
    return(false);
  }
  
  if(pBMPHeader->biCopmression!=BI_RGB){
    BMP_LoadErrorStr="Error notsupport Compression";
    return(false);
  }
  
  if(pBMPHeader->biHeight>=0x80000000){
    BMP_LoadErrorStr="Error notsupport OS/2 format";
    return(false);
  }
  
  if(pBMPHeader->biPlanes!=1){
    BMP_LoadErrorStr="Error notsupport Planes!=1";
    return(false);
  }
  
  switch(pBMPHeader->biBitCount){
    case 1:
      BMP_LoadErrorStr="Error notsupport 1bitcolor.";
      return(false);
    case 4:
      BMP_LoadErrorStr="Error notsupport 4bitcolor.";
      return(false);
    case 8:
      pBMPHeader->DataWidth=pBMPHeader->biWidth*1;
      break;
    case 16:
      BMP_LoadErrorStr="Error notsupport 16bitcolor.";
      return(false);
    case 24:
      pBMPHeader->DataWidth=pBMPHeader->biWidth*3;
      break;
    case 32:
      BMP_LoadErrorStr="Error notsupport 32bitcolor.";
      return(false);
    default:
      BMP_LoadErrorStr="Error Unknown xxBitColor.";
      return(false);
  }
  
  if((pBMPHeader->DataWidth&3)!=0){
    pBMPHeader->DataWidth+=4-(pBMPHeader->DataWidth&3);
  }
  
  BMP_LoadErrorStr="";
  return(true);
}

//static char bmerrstr1[256],bmerrstr2[256];

int intLoadBM(u16 *pbm,const u32 bmw,const u32 bmh)
{
  
  u8 *bmdata;
  u32 bmsize;
   TBMPHeader BMPHeader;
u32 y,x;
  u32 gr=0,gg=0,gb=0;
bmdata=(u8*)&test_bmp[0];
  bmsize=sizeof(test_bmp);

 
  
  if(GetBMPHeader(bmdata,&BMPHeader)==false){ 
    return(false);
  }
  
  if((BMPHeader.biWidth==1)&&(BMPHeader.biHeight==1)){ 
    //free(bmdata); bmdata=NULL;
    return(false);
  }
  
  if((BMPHeader.biWidth<bmw)||(BMPHeader.biHeight<bmh)){ 
    return(false);
  }
  

  
#define Gravity(c,cg) { \
  c+=cg; \
  cg=c&7; \
  c=c>>3; \
  if((c&(~0x1f))!=0) c=(c<0) ? 0x00 : 0x1f; \
}

  for(y=0;y<bmh;y++){
    u8 *pSrcBM=&BMPHeader.pBitmap[(BMPHeader.biHeight-1-y)*BMPHeader.DataWidth];
    u16 *pDstBM=&pbm[y*bmw];
    
    switch(BMPHeader.biBitCount){
      case 8: {
        u8 *PaletteTable=BMPHeader.pPalette;
        for(x=0;x<bmw;x++){
          u8 *pal;
          u32 r,g,b;
          
          pal=&PaletteTable[*pSrcBM*4];
          pSrcBM+=1;
          
          b=pal[0];
          g=pal[1];
          r=pal[2];
          
          Gravity(b,gb);
          Gravity(g,gg);
          Gravity(r,gr);
          
          pDstBM[x]=RGB15(r,g,b) ;//| BIT(15);
        }
        break;
      }
      case 24: {
        for(x=0;x<bmw;x++){
          u32 r,g,b;
          
          b=pSrcBM[0];
          g=pSrcBM[1];
          r=pSrcBM[2];
          pSrcBM+=3;
          
          Gravity(b,gb);
          Gravity(g,gg);
          Gravity(r,gr);
          
          pDstBM[x]=RGB15(r,g,b) ;//| BIT(15);
        }
        break;
      }
    }
    
  }
  
#undef Gravity

  //free(bmdata); bmdata=NULL;
  
  return(true);
}
u32 get(void)
{
  u16 * pTitleABM=(u16*)&kkk[0];//(u16*)malloc(256*192*2); 
  
  if(intLoadBM(pTitleABM,256,192)==false){
  }
  return (u32)pTitleABM;
}
