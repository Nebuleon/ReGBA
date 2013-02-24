#include <bsp.h>
#include <jz4740.h>
#define CACHE_MAX_NUM 32
#define SECTOR_SIZE 512
typedef struct {
	unsigned short  CacheState;
	unsigned short  UseCount;
	unsigned short  CacheChange;
	unsigned short  CacheReserve;	
	unsigned int BlockId;
    unsigned char   aBlockData[SECTOR_SIZE];
} SSFDC__LB_CACHE;


#define FREE_CACHE 0
#define PREWRITE_CACHE 2
#define OFTEN_USE_CACHE 3
#if 0
#define dprintf(x,...)  printf(x)
#else
#define dprintf(x,...)
#endif

static SSFDC__LB_CACHE ssfdc_cache[CACHE_MAX_NUM]  __attribute__ ((aligned (4)));
static unsigned short Cur_CacheCount = 0;


#define DMA_ENABLE 0

#if DMA_ENABLE
#define DMA_CHANNEL 4
#define PHYSADDR(x) ((x) & 0x1fffffff)
#else
#define lb_memcpy memcpy
#endif

#if DMA_ENABLE
static void lb_memcpy(void *target,void* source,unsigned int len)
{
    int ch = DMA_CHANNEL;
	if(((unsigned int)source < 0xa0000000) && len)
		dma_cache_wback_inv((unsigned long)source, len);
    if(((unsigned int)target < 0xa0000000) && len)
		dma_cache_wback_inv((unsigned long)target, len);
	
	
	REG_DMAC_DSAR(ch) = PHYSADDR((unsigned long)source);       
	REG_DMAC_DTAR(ch) = PHYSADDR((unsigned long)target);       
	REG_DMAC_DTCR(ch) = len / 32;	            	    
	REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_AUTO;	        	
	REG_DMAC_DCMD(ch) = DMAC_DCMD_SAI| DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32|DMAC_DCMD_DS_32BYTE;
	REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
	while (	REG_DMAC_DTCR(ch) );
}
#endif
	
static void _NAND_LB_InitCache()
{
	int i,ret = 0;
	SSFDC__LB_CACHE *pCache = ssfdc_cache;
	i = 0;
	while(1)
	{
		if(i >= CACHE_MAX_NUM)
		{
			ret = -1;
			break;
		}
		pCache->CacheState = FREE_CACHE;
        pCache->UseCount = 0;
	    pCache->CacheChange = 0;
        pCache++;
		i++;
	}
	Cur_CacheCount = 0;
	
}

static  SSFDC__LB_CACHE * _NAND_LB_GetFreeCache()
{
	int i,ret = 0;
	SSFDC__LB_CACHE *pCacheInfo = &ssfdc_cache[Cur_CacheCount];
	while(1)
	{
        if(ret >= CACHE_MAX_NUM)
			return 0;
		if(pCacheInfo >= &ssfdc_cache[CACHE_MAX_NUM])
		{
			pCacheInfo = ssfdc_cache;
			Cur_CacheCount = 0;
		}
		
		if(pCacheInfo->CacheState == FREE_CACHE)
		{
           	return pCacheInfo;
		}
		pCacheInfo++;
		Cur_CacheCount++;
		ret++;
	}
	return 0;
}
#define _NAND_LB_Write(pCache) ssfdc_nftl_write(pCache->BlockId,pCache->aBlockData,1)

static void _NAND_LB_CloseCACHES(unsigned int sectorstart,unsigned int sectorend){
  unsigned int i;
  SSFDC__LB_CACHE *pCache = ssfdc_cache;
  for( i = 0;i < CACHE_MAX_NUM;i++){
    if((pCache->CacheState != FREE_CACHE) && (pCache->BlockId >= sectorstart) && (pCache->BlockId < sectorend)){
      pCache->CacheChange = 0;
      pCache->CacheState = FREE_CACHE;			    
      pCache->UseCount = 0;
    }
    pCache++;
    
  }
    
}

static void _NAND_LB_FLUSHCACHES(unsigned int sectorstart,unsigned int sectorend){
  unsigned int i;
  SSFDC__LB_CACHE *pCache = ssfdc_cache;
  for( i = 0;i < CACHE_MAX_NUM;i++){
    if((pCache->CacheState != FREE_CACHE) && (pCache->BlockId >= sectorstart) && (pCache->BlockId < sectorend)){
		if(pCache->CacheChange)
			_NAND_LB_Write(pCache);
		pCache->CacheChange = 0;
		pCache->CacheState = FREE_CACHE;			    
		pCache->UseCount = 0;
    }
    pCache++;
    
  }
    
}
inline static int Get_NAND_CacheFreeCount()
{
	SSFDC__LB_CACHE *pCache = ssfdc_cache;
	SSFDC__LB_CACHE *pEndCache = &ssfdc_cache[CACHE_MAX_NUM];
	unsigned int count = 0;
	while(pCache < pEndCache)
	{
		if(pCache->CacheState == FREE_CACHE)
			count++;
		pCache++;	
	}
	return count;
	
}
static unsigned int _NAND_LB_PreWiteToNand(SSFDC__LB_CACHE *pCache,unsigned short *count,unsigned int update)
{
	SSFDC__LB_CACHE *pWriteCache;
	//SSFDC__LB_CACHE *pOldCache = pCache;
	SSFDC__LB_CACHE *pEndCache = &ssfdc_cache[CACHE_MAX_NUM];
	unsigned int sector = -1;
	unsigned int flag;
	while(1)
	{
		sector = -1;
		flag = 0;
		pWriteCache = ssfdc_cache;
		while(pWriteCache < pEndCache)
		{
			if(pWriteCache->CacheState == update) //PREWRITE_CACHE
			{
				if(pWriteCache->BlockId < sector)
				{
						sector = pWriteCache->BlockId;
						pCache = pWriteCache;
				}
			}else
					flag++;
			pWriteCache++;
		}
		
		if(flag < CACHE_MAX_NUM)
		{	
			if(pCache->CacheChange)
			{
				_NAND_LB_Write(pCache);
				pCache->CacheChange = 0;
	    }
			pCache->CacheState = FREE_CACHE;			    
			pCache->UseCount = 0;
			(*count)++;
	 }else
	 	   break;		
	}
}
static void _NAND_LB_OftenToNand(SSFDC__LB_CACHE *pCache,unsigned short *count,unsigned int update)
{
	//printf("%s!\n",__FUNCTION__);
	SSFDC__LB_CACHE *pWriteCache = pCache;
	SSFDC__LB_CACHE *pOldCache = pCache;
	SSFDC__LB_CACHE *pEndCache = &ssfdc_cache[CACHE_MAX_NUM];
	
	while(pCache)
	{
		
		if(pCache->CacheState == OFTEN_USE_CACHE)
		{
            if(pWriteCache->CacheState != OFTEN_USE_CACHE)
				pWriteCache = pCache;
			else if(pWriteCache->UseCount > pCache->UseCount)
			{
				pWriteCache = pCache;
			}
			
		}
		pCache++;
		if(pCache >= pEndCache)
			break;
	}
	if(pWriteCache->CacheState == OFTEN_USE_CACHE)
	{
        (*count)++;
        if(pWriteCache->CacheChange)
			_NAND_LB_Write(pWriteCache);
		//if(update == 1)
		pWriteCache->CacheState = FREE_CACHE;			    
			
		pWriteCache->UseCount = 0;
		pWriteCache->CacheChange = 0;
		if(update != -1)
			update--;
		if(update != 0)
			_NAND_LB_OftenToNand(pOldCache,count,update);
		
	}
}

static int _NAND_LB_FreeCache(unsigned int update)
{
	unsigned short freecount = 0,totalfree = 0;
    
	freecount = 0;
	
	_NAND_LB_PreWiteToNand(ssfdc_cache,&freecount,PREWRITE_CACHE);
    
	totalfree += freecount;
	//printf("free count = %d\n",freecount);
	if(freecount == 0)
	{
		freecount = 0;	
		_NAND_LB_PreWiteToNand(ssfdc_cache,&freecount,OFTEN_USE_CACHE);
		totalfree += freecount;
		update = 0;
	}	
	if(update)
	{	
		if(Get_NAND_CacheFreeCount() < CACHE_MAX_NUM * 1 / 4) // because fat is 4 sector
		{
			//printf("!!!!!!!!!!!!!!!!11\n");
			freecount = 0;	
			_NAND_LB_PreWiteToNand(ssfdc_cache,&freecount,OFTEN_USE_CACHE);
			totalfree += freecount;
		}
  }
  
	//printf("Free = %d\r\n",totalfree);
	return totalfree;
}
static int _NAND_LB_GetFromCache(unsigned int Sector, void *pBuffer) {

	SSFDC__LB_CACHE *pCache = &ssfdc_cache[Cur_CacheCount];
	SSFDC__LB_CACHE *pUseCache = 0;
	unsigned short i;
	//dprintf("sector = %x pBuffer = %x\n",Sector,pBuffer);
	if(pCache >= &ssfdc_cache[CACHE_MAX_NUM])
		pCache = ssfdc_cache;
	i = 0;
	while (1) {
		
      if(pCache->CacheState != FREE_CACHE)
	  {
		  
		  if (Sector == pCache->BlockId)  {
              //dprintf("Cache is use = %d\r\n",pCache->BlockId);
			  
			  pUseCache = pCache;
              
			  pCache->UseCount++;
			  if(pCache->UseCount == 0)
				  pCache->UseCount = -1;
			  
			  pCache->CacheState = OFTEN_USE_CACHE;
			  
		  }
	  }
	  pCache--;
	  if(pCache < ssfdc_cache)
		  pCache = &ssfdc_cache[CACHE_MAX_NUM - 1];
	  
      i++;
	  if (i >= CACHE_MAX_NUM) {
		  break;  /* Sector not in cache */
      }
	}
	if (pUseCache) {
	  //dprintf("From Cache %d\r\n",Sector);
		
      lb_memcpy(pBuffer, pUseCache->aBlockData, SECTOR_SIZE);
	  return 0;
	}
	return -1;  
}

static void _NAND_LB_ClearCache() {

	unsigned short freecount = 0;
	// dprintf("Clear Cache\r\n");
	
	_NAND_LB_PreWiteToNand(ssfdc_cache,&freecount,PREWRITE_CACHE);
	_NAND_LB_PreWiteToNand(ssfdc_cache,&freecount,OFTEN_USE_CACHE);
}
static void _NAND_LB_CopyToCache(unsigned int Sector, void *pBuffer,unsigned short rw) {

	SSFDC__LB_CACHE *pCache = _NAND_LB_GetFreeCache();
//    dprintf("Copy to Cache = 0x%08x 0x%08x\r\n",pCache,ssfdc_cache);
	
	if(!pCache)
	{
		// dprintf("NAND Free!\r\n");
		_NAND_LB_FreeCache(rw);
		
		pCache = _NAND_LB_GetFreeCache();
	}
	pCache->BlockId = Sector;
	pCache->CacheState = PREWRITE_CACHE;
	pCache->UseCount = 0;
	pCache->CacheChange = rw;
	
	lb_memcpy(pCache->aBlockData,pBuffer,SECTOR_SIZE);
}


int _NAND_LB_UpdateInCache(unsigned int Sector, void *pBuffer) {
    short i,ret = 0;
	i =  Cur_CacheCount;
	if(Cur_CacheCount > CACHE_MAX_NUM)
		i = 0;
	while(1)
	{
        if(ret >= CACHE_MAX_NUM)
			return -1;
        if(ssfdc_cache[i].CacheState != FREE_CACHE)
		{
        
			if(ssfdc_cache[i].BlockId == Sector)
			{
				//printf("UpdateInCache = %d\r\n",Sector);
				ssfdc_cache[i].CacheState = OFTEN_USE_CACHE;
				ssfdc_cache[i].UseCount++;
        ssfdc_cache[i].CacheChange = 1;
				lb_memcpy(ssfdc_cache[i].aBlockData,pBuffer,SECTOR_SIZE);
				return 0;
				
			}
		}
		i--;
		if(i < 0)
			i = CACHE_MAX_NUM - 1;

		ret++;
	}
	return -1;
}


/*********************************************************************
*
*             Global functions
*
**********************************************************************

  Functions here are global, although their names indicate a local
  scope. They should not be called by user application.
*/


#define  _NAND_LB_Read(Sector,pBuffer)  ssfdc_nftl_read(Sector,pBuffer,1);

int NAND_LB_Init() {

//    dprintf("NAND Init\r\n");
	
	_NAND_LB_InitCache();	
	jz_nand_init();
	ssfdc_nftl_init();
	ssfdc_nftl_open();
	
	return -1;
}
int NAND_LB_FLASHCACHE() {

    //printf("nand flash!\r\n");
	_NAND_LB_ClearCache();
   ssfdc_nftl_flush_cache();
	return 0;
}
int NAND_LB_Deinit()
{
	NAND_LB_FLASHCACHE();
	
	ssfdc_nftl_release();
	
}
int NAND_LB_MultiRead(unsigned int Sector, void *pBuffer,unsigned int SectorCount) {
	//printf("NAND_LB_MultiRead = %x %x\r\n",Sector,pBuffer);
	
	_NAND_LB_FLUSHCACHES(Sector,Sector+SectorCount);
	#if 1
	return ssfdc_nftl_read(Sector,pBuffer,SectorCount);
	#else
	if(SectorCount > 1)
		return ssfdc_nftl_multiread(Sector,pBuffer,SectorCount);
	return ssfdc_nftl_read(Sector,pBuffer,SectorCount);
	#endif
}
	
int NAND_LB_Read(unsigned int Sector, void *pBuffer) {


    int x;
  //printf("LB_Read = %x %x\r\n",Sector,pBuffer);

	if(_NAND_LB_GetFromCache(Sector,pBuffer))
	{
       x =  _NAND_LB_Read(Sector,pBuffer);
        //printf("LB_Read = %x %x,ret = %d\r\n",Sector,pBuffer,x);
      	
      _NAND_LB_CopyToCache(Sector,pBuffer,0);
		return x;
	}
	return 512;
}

int NAND_LB_MultiWrite(unsigned int Sector, void *pBuffer,unsigned int SectorCount) {
  
	_NAND_LB_CloseCACHES(Sector,Sector + SectorCount);
  return ssfdc_nftl_write(Sector,pBuffer,SectorCount);
}


int NAND_LB_Write(unsigned int Sector, void *pBuffer) {
	int x;
	//printf("LB_Write = %x %x\r\n",Sector,pBuffer);
	
	if(_NAND_LB_UpdateInCache(Sector,pBuffer))
	{
		_NAND_LB_CopyToCache(Sector,pBuffer,1);
		return 512;
		
	}
	return 512;
}
