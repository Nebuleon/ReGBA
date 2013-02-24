//
// Copyright (c) Ingenic Semiconductor Co., Ltd. 2007.
//

#include "header.h"

#define ssfdc_memcpy	memcpy
#define inline			__inline

#define	SECTORS_PER_PAGE	4
#define DEFAULT_SECT_SIZE 0x200
#if __UNDER_UCOS__

#define	ALINGED(n)	__attribute__ ((aligned(n)))

static page_cache_t		g_read_page_cache	ALINGED(4);
static block_cache_t	g_write_cache	ALINGED(4);
#define malloc(x) alloc(x)
#define free(x) deAlloc(x)

#elif __UNDER_WINCE__

static page_cache_t		g_read_page_cache;
static block_cache_t	g_write_cache;

#endif

#define PAGE_PTR(data,x) (data + x * g_nand_page_size)


static unsigned int	g_start_phys_block = 0;
static unsigned int	g_start_phys_page = 0;
static unsigned int g_total_phys_block = 0;
static unsigned int g_total_virt_block = 0;

static unsigned int g_MaxBadBlocks = 0;

static unsigned int g_bad_block = 0;
static unsigned int g_free_phys_block = 0;
static unsigned int g_free_virt_block = 0;
static unsigned int g_used_block = 0;
static unsigned int g_cur_phy_block = 0;

static unsigned int g_min_lifetime = -1;

block_info_t		*g_pBlockInfo = 0;
static unsigned int	*g_pBlockLookup = 0;
static unsigned int	*g_pBlockInfoIndex = 0;

static unsigned char	*g_dirty_page;
static unsigned char	*g_cache_data; 
static unsigned char	*g_read_data;
static unsigned char	*g_ext_data;


static unsigned int g_nand_page_size = 0, g_nand_page_per_block = 0, g_nand_block_size = 0;

/////////Translate Physical Page Address
#define	NAND_READ_PAGE(a, b, c, d)		\
			jz_nand_read_page(a, (b) + g_start_phys_page, c, d)

#define	NAND_READ_PAGE_INFO(a, b, c)	\
			jz_nand_read_page_info(a, (b) + g_start_phys_page, c);

#define	NAND_READ_MULTIPAGE(a, b, c, d, e, f)	\
			jz_nand_multiread(a, (b) + g_start_phys_page, c, d, e, f);

#define	NAND_WRITE_PAGE_INFO(a, b, c)	\
			jz_nand_write_page_info(a, (b) + g_start_phys_page, c);

#define	NAND_WRITE_MULTIPAGE(a, b, c, d, e, f)	\
			jz_nand_multiwrite(a, (b) + g_start_phys_page, c, d, e, f);

#define	NAND_ERASE_BLOCK(a, b)	\
			jz_nand_erase_block(a, (b) + g_start_phys_block);

#define	NAND_SET_BLOCK_STATUS(a, b)	\
			jz_nand_set_block_status((a) + g_start_phys_block, b);

//------------------------------------------------------------------------------

static inline int ssfdc_block_info_map_bad_block(unsigned int phys_block)
{
	g_pBlockInfo[phys_block].tag |= SSFDC_BLOCK_TAG_BAD;
	g_bad_block ++;
	g_free_phys_block --;
	g_free_virt_block --;

	if (g_bad_block > g_MaxBadBlocks)
	{
		return (-EIO);
	}

	return (0);
}

//------------------------------------------------------------------------------

int ssfdc_block_lookup_map_entry(unsigned int virt_block, unsigned int phys_block)
{
	if (virt_block >= g_total_virt_block)
		return (-EINVAL);

	if (!(g_pBlockLookup[virt_block] & SSFDC_BLOCK_VALID_ENTRY))
	{
		g_pBlockLookup[virt_block] = phys_block;
		g_pBlockLookup[virt_block] |= SSFDC_BLOCK_VALID_ENTRY;
		g_pBlockInfo[phys_block].tag &= ~SSFDC_BLOCK_TAG_FREE;

		g_free_phys_block --;
		g_free_virt_block --;
		g_used_block ++;
		g_cur_phy_block = phys_block;

		return (0);
	}
	else
	{
		return (-EINVAL);
	}
}

//------------------------------------------------------------------------------

int ssfdc_block_lookup_unmap_entry(unsigned int virt_block)
{
	unsigned int	phys_block;

	if (g_pBlockLookup[virt_block] & SSFDC_BLOCK_VALID_ENTRY)
	{
		g_pBlockLookup[virt_block] &= ~SSFDC_BLOCK_VALID_ENTRY;
		phys_block = g_pBlockLookup[virt_block] & SSFDC_BLOCK_ADDR_MASK;
		g_pBlockInfo[phys_block].tag |= SSFDC_BLOCK_TAG_FREE;
		g_free_phys_block++;
		g_free_virt_block++;
		g_used_block--;

		return (0);
	}
	else
	{
		return (-EINVAL);
	}
}

//------------------------------------------------------------------------------

static int ssfdc_block_address_translate(unsigned int virt_block, unsigned int *phys_block) 
{	
	unsigned int	entry;

	entry = g_pBlockLookup[virt_block];
	if (!(entry & SSFDC_BLOCK_VALID_ENTRY))
		return (-EINVAL);
	*phys_block = entry & SSFDC_BLOCK_ADDR_MASK;

	return (0);
}
//------------------------------------------------------------------------------

unsigned int ssfdc_nftl_read(unsigned int	nSectorAddr,
							 unsigned char	*pBuffer,
							 unsigned int	nSectorNum)
{
	unsigned int	i, j;
	unsigned int	virt_page, phys_page;
	unsigned int	cvirt_page, cvirt_secotr, sector_num_in_page;
	unsigned int	virt_block, phys_block;
	nand_page_info_t	info;
	block_cache_t		*block_cache = &g_write_cache;
	page_cache_t		*page_cache = &g_read_page_cache;
	int					op_cache_flag;
  //printf("++++ %s ++++ %x \n",__FUNCTION__,pBuffer);
	for (i = 0; i < nSectorNum; nSectorAddr++, i ++)
	{
		virt_page = nSectorAddr / SECTORS_PER_PAGE;
		sector_num_in_page = nSectorAddr % SECTORS_PER_PAGE;
		virt_block = virt_page / g_nand_page_per_block;
		if (ssfdc_block_address_translate(virt_block, &phys_block) < 0)
		{
			memset(pBuffer, 0xFF, DEFAULT_SECT_SIZE);
		}
		else
		{
			cvirt_page = virt_page % g_nand_page_per_block;
			op_cache_flag = (virt_block == block_cache->virt_block) && (block_cache->in_use);

			if (op_cache_flag && (block_cache->dirty_page[cvirt_page] & (1 << sector_num_in_page)))
			{
				cvirt_secotr = cvirt_page * SECTORS_PER_PAGE + sector_num_in_page;
				ssfdc_memcpy(	pBuffer,
								&block_cache->data[cvirt_secotr * (DEFAULT_SECT_SIZE)],
								DEFAULT_SECT_SIZE);
			}
			else
			{
				if (op_cache_flag)
					phys_block = block_cache->old_phys_block;

				phys_page = phys_block * g_nand_page_per_block + cvirt_page;
				if (phys_page != page_cache->page_num)
				{
					
					NAND_READ_PAGE(0, phys_page, page_cache->data, &info);
					page_cache->page_num = phys_page;
					if (op_cache_flag)
					{
						for ( j = 0; j < SECTORS_PER_PAGE; j ++ )
						{
							if ((block_cache->dirty_page[cvirt_page] & (1 << j)) == 0)
							{
								if ((info.page_status & (1 << j)) == 0)
								{
									cvirt_secotr = cvirt_page * SECTORS_PER_PAGE + j;
									ssfdc_memcpy(&block_cache->data[cvirt_secotr * (DEFAULT_SECT_SIZE)],
												 page_cache->data + j * DEFAULT_SECT_SIZE,
												 DEFAULT_SECT_SIZE);
									block_cache->dirty_page[cvirt_page] |= (1 << j);
								}
							}
						}
					}
				}
				ssfdc_memcpy(	pBuffer,
								page_cache->data + sector_num_in_page * DEFAULT_SECT_SIZE,
								DEFAULT_SECT_SIZE);
			}
		}
		pBuffer += (DEFAULT_SECT_SIZE);
	}
	//printf("--- %s ---\n",__FUNCTION__);
	
	return (nSectorNum * DEFAULT_SECT_SIZE);
}

//------------------------------------------------------------------------------
#if 1
unsigned int ssfdc_nftl_multiread(unsigned int	nSectorAddr,
							 unsigned char	*pBuffer,
							 unsigned int	nSectorNum)
{
	unsigned int	i, j;
	unsigned int oldBuffer = (unsigned int) pBuffer;
	unsigned int	phys_page;
	unsigned int	start_virt_block,end_virt_block,start_page,end_page,start_sector,end_sector;
	unsigned int  phys_block,tmp,op_cache_flag;
	unsigned int run = 1;
	unsigned int endsector = 4;
	unsigned char dirtypage = 0;							
	nand_page_info_t	info;
	block_cache_t		*block_cache = &g_write_cache;
	page_cache_t		*page_cache = &g_read_page_cache;
	
	start_page = (nSectorAddr / 4 )% g_nand_page_per_block;
	end_page = ((nSectorAddr + nSectorNum) / 4) % g_nand_page_per_block; 
	
	start_sector = nSectorAddr % 4;
	end_sector =  (nSectorAddr + nSectorNum) % 4;

	start_virt_block = nSectorAddr / (g_nand_page_per_block  * 4);
	end_virt_block = (nSectorAddr + nSectorNum) / (g_nand_page_per_block * 4);
	if((end_page == 0) && (end_sector == 0))
		if(end_virt_block >0)
			end_virt_block--;
	if(end_sector == 0)
	{
		end_sector = 4;
		if(end_page != 0)
			end_page--;
		else
			end_page = g_nand_page_per_block - 1;	
	}		
	//printf("+++%x %x %d+\n",nSectorAddr,pBuffer,nSectorNum);
	//printf("%x %x %x %x\n",start_page,end_page,start_sector,end_sector);
	//printf("%x %x\n",start_virt_block,end_virt_block);
	if((start_page!= 0) || (start_sector !=0))
	{
		if (ssfdc_block_address_translate(start_virt_block, &phys_block) < 0)
		{
			if(start_virt_block == end_virt_block)
			{
				tmp = ((end_page - start_page) * 4 - start_sector + end_sector) * 512;
				memset(pBuffer,0xff,tmp);
				pBuffer += tmp;
				run = 0;
			}else
			{	
				 tmp = ((g_nand_page_per_block - start_page )* 4 - start_sector) * 512;
				 memset(pBuffer,0xff,tmp);
				 pBuffer += tmp;
			}
		}else
		{
			//printf("1111 phys_block = %x \n",phys_block);
			op_cache_flag = (start_virt_block == block_cache->virt_block) && (block_cache->in_use);
			if(op_cache_flag)
			{
				  
				  phys_block = block_cache->old_phys_block;
				  phys_page = phys_block * g_nand_page_per_block;
				  //printf("1111 phys_page = %x \n",phys_page);
			
				  for(i = start_page;(i < g_nand_page_per_block) & run;i++)
					{
			//				printf("111 write cache %x\n",i);
							if((block_cache->dirty_page[i] & ((1 << 4) - 1)) != 0xf)
							{
								//printf("read cache = %d %x\n",i,block_cache->dirty_page[i]);
								NAND_READ_MULTIPAGE(0,phys_page + i, 1,
								&block_cache->dirty_page[i],&block_cache->data[i*2048], &info);
				//				printf("read cache = %d %x\n",i,block_cache->dirty_page[i]);
							}	
								
							if((start_virt_block == end_virt_block) && (i >= end_page)) 
						  {	
						  		endsector = end_sector;
						  		run = 0;
						  }
						
						  tmp = (endsector - start_sector)* 512;
					//	  printf("tmp = %x pBuffer = %x %d\n",tmp,pBuffer,run);
						  memcpy(pBuffer,&(block_cache->data[i * 2048 + start_sector * 512]),tmp);
						  pBuffer += tmp;
						  start_sector = 0;
				  }
			}else
			{
				  phys_page = phys_block * g_nand_page_per_block;
			  	
			  	for(i = start_page;(i < g_nand_page_per_block) & run;i++)
					{
							
						  dirtypage = 0x0f;
				  		if((start_virt_block == end_virt_block) && (i >= end_page)) 
		  			  {
		  			  	endsector = end_sector;
		  			  	run = 0;
		  			  }else
		  			  	endsector = 4;
				  		for(j = start_sector;j < endsector;j++)
				  		{
				  				 dirtypage &= (~(1 << j));				  			  	 
				  		}
				  		//printf("111 start_page = %x %x %x %x\n",start_page,end_page,phys_page + i,endsector - start_sector);
			  			//printf("111v dirtypage = %x\n",dirtypage);
			  			
			  			NAND_READ_MULTIPAGE(0,phys_page + i, 1,
								(unsigned char *)&dirtypage,(unsigned char *)(pBuffer - start_sector * 512),&info);
							//printf("data: %x\n",*(unsigned int *) pBuffer);
							//printf("111v dirtypage = %x\n",dirtypage);
			  			
							pBuffer += (endsector - start_sector) * 512;
							start_sector = 0;   		
					}
			}	  	
		}
		start_virt_block++;	
	} 
		
	while(run && (start_virt_block < end_virt_block))
	{
		 if (ssfdc_block_address_translate(start_virt_block, &phys_block) < 0)
		 {
		 			memset(pBuffer,0xff,g_nand_page_per_block * 2048);
		 			pBuffer += g_nand_page_per_block * 2048;
		 }
		 else
		 {
		 		op_cache_flag = (start_virt_block == block_cache->virt_block) && (block_cache->in_use);
				if(op_cache_flag)
				{
					phys_block = block_cache->old_phys_block;
				  phys_page = phys_block * g_nand_page_per_block;
					// Get the un-dirty data from old block
					
						NAND_READ_MULTIPAGE(0,
						block_cache->old_phys_block * g_nand_page_per_block,
						g_nand_page_per_block,
						block_cache->dirty_page,
						block_cache->data,
						(nand_page_info_t *)block_cache->ext_data);
						memcpy(pBuffer,block_cache->data,g_nand_page_per_block * 2048);
						pBuffer += g_nand_page_per_block * 2048;
				}else
				{
					 phys_page = phys_block * g_nand_page_per_block;
					 for(i = start_page;i < g_nand_page_per_block;i++)
					 {
					  	 dirtypage = 0;
				  		 NAND_READ_MULTIPAGE(0,phys_page + i, 1,
								(unsigned char *)&dirtypage,pBuffer,&info);	 
							pBuffer += 2048;
					 }
				}	
		 }
		 start_virt_block++;			
	}
	if(run && ((end_page != 0) || (end_sector != 0)))
	{
		if (ssfdc_block_address_translate(start_virt_block, &phys_block) < 0)
		{
			 	tmp = (end_page * 4 + end_sector) * 512;
			 	memset(pBuffer,0xff,tmp);
			 	pBuffer += tmp;
		}	 		
		else
		{
			op_cache_flag = (start_virt_block == block_cache->virt_block) && (block_cache->in_use);
			if(op_cache_flag)
			{
				 phys_block = block_cache->old_phys_block;
				 phys_page = phys_block * g_nand_page_per_block;
					//printf("222phys_page = %x\n",phys_page);	
					for(i = 0;(i < g_nand_page_per_block) & run;i++)
					{
							//printf("22writecahe %x\n",i);
							if(block_cache->dirty_page[i] != 0xf)
							{
								NAND_READ_MULTIPAGE(0,phys_page, 1,
								&block_cache->dirty_page[i],&block_cache->data[i*2048], &info);
							}	
								
							if(i >= end_page)
							{
									endsector = endsector;
									run = 0;
							}
					  	tmp = endsector * 512;
					  	memcpy(pBuffer,&block_cache->data[i * 2048 + start_sector * 512],tmp);
					  	pBuffer += tmp;	  
				  }
						
		  }else
		  {
		  	
		  	phys_page = phys_block * g_nand_page_per_block;
		  	//printf("222 phys_block = %x %x %x\n",phys_block,phys_page,g_nand_page_per_block);
		  	for(i = 0; i < end_page;i++)
		  	{
		  		dirtypage = 0;
				  //printf("2222 dirtypage = %x\n",(phys_page + i));
		  		
				  NAND_READ_MULTIPAGE(0,phys_page + i, 1,
								(unsigned char *)&dirtypage,pBuffer,&info);	 
					pBuffer += 2048;
		  	}
		  	if(end_sector)
		  	{
		  		dirtypage = 0x0f;
		  		for(j = 0;j < end_sector;j++)
		  			dirtypage &= (~(1 << j));
		  		
				  //printf("dirtypage = %x\n",dirtypage);
		  		NAND_READ_MULTIPAGE(0,phys_page + i, 1,
								(unsigned char *)&dirtypage,pBuffer,&info);	 
					pBuffer += end_sector * 512;	
		  	}	
		  }	
		}	
	}	
	//printf("---%x %x %d\n",nSectorAddr,pBuffer,nSectorNum);
	
	return (oldBuffer - (unsigned int)pBuffer);
}
#endif

//------------------------------------------------------------------------------

int ssfdc_block_find_free(unsigned int	*free_phys_block)
{
	unsigned int	i = 0, phys_block, count;

	count = g_cur_phy_block;

	while (1)
	{
		i ++;

		if (i >= g_total_phys_block)
			break;

		count ++;
		if (count >= g_total_phys_block)
		{
			g_min_lifetime ++;
			count = 0;
		}

		phys_block = g_pBlockInfoIndex[count + 1];
		if (g_pBlockInfo[phys_block].tag & SSFDC_BLOCK_TAG_BAD)
			continue;

		if (g_pBlockInfo[phys_block].tag & SSFDC_BLOCK_TAG_FREE)
		{
			if (g_min_lifetime >= g_pBlockInfo[phys_block].lifetime)
			{
				g_min_lifetime = g_pBlockInfo[phys_block].lifetime;
				*free_phys_block = phys_block;
				g_cur_phy_block = phys_block;
				//printf("Find free %x\n",phys_block);
				return (0);
			}
		}
	}

	return (-ENOMEM);
}

//------------------------------------------------------------------------------

void ssfdc_block_cache_setup(	block_cache_t	*cache,
								unsigned int	virt_block,
								unsigned int	new_phys_block,
								unsigned int	old_phys_block)
{
	cache->old_phys_block = old_phys_block;
	cache->new_phys_block = new_phys_block;
	cache->virt_block = virt_block;
	cache->in_use = 1;
	memset(cache->dirty_page, 0, g_nand_page_per_block * sizeof(unsigned char));
}

//------------------------------------------------------------------------------
// ssfdc_nftl_write:   sector write
//                     
unsigned int ssfdc_nftl_write(	unsigned int	nSectorAddr,
								unsigned char	*pBuffer,
								unsigned int	nSectorNum)
{
	unsigned int	i;
	unsigned int	virt_page, virt_block;
	unsigned int	new_phys_block = 0, old_phys_block = 0;
	unsigned int	page_num_in_block, sector_num_in_page;
	block_cache_t	*block_cache = &g_write_cache;

	//printf("%s %x %x\n",__FUNCTION__,nSectorAddr,nSectorNum);
	for (i = 0; i < nSectorNum; nSectorAddr++, i++)
	{
		virt_page = nSectorAddr / SECTORS_PER_PAGE;
		sector_num_in_page = nSectorAddr % SECTORS_PER_PAGE;
		virt_block = virt_page / g_nand_page_per_block;
		page_num_in_block = virt_page % g_nand_page_per_block;

		if (ssfdc_block_address_translate(virt_block, &old_phys_block) < 0)
		{
			if (block_cache->in_use)
				ssfdc_nftl_flush_cache();

			if (ssfdc_block_find_free(&new_phys_block))
				return 0;

			ssfdc_block_cache_setup(block_cache, virt_block, new_phys_block, new_phys_block);
			ssfdc_block_lookup_map_entry(virt_block, new_phys_block);
		}
		else
		{
			if (block_cache->in_use)
			{
				if (block_cache->virt_block != virt_block)
				{
					// Cache Miss
					ssfdc_nftl_flush_cache();
					ssfdc_block_lookup_unmap_entry(virt_block);
					if (ssfdc_block_find_free(&new_phys_block))
						return (0);

					ssfdc_block_cache_setup(block_cache, virt_block, new_phys_block, old_phys_block);
					ssfdc_block_lookup_map_entry(virt_block, new_phys_block);
				}
				else
				{
					// Cache Hit
				}
			}
			else
			{
				ssfdc_block_lookup_unmap_entry(virt_block);
				if (ssfdc_block_find_free(&new_phys_block))
					return (0);

				ssfdc_block_cache_setup(block_cache, virt_block, new_phys_block, old_phys_block);
				ssfdc_block_lookup_map_entry(virt_block,new_phys_block);
			}
		}
		block_cache->dirty_page[page_num_in_block] |= (1 << sector_num_in_page);
		ssfdc_memcpy(	PAGE_PTR(block_cache->data, page_num_in_block) + sector_num_in_page * (DEFAULT_SECT_SIZE),
						pBuffer,
						DEFAULT_SECT_SIZE);
		pBuffer += (DEFAULT_SECT_SIZE);
	}

	return ( (DEFAULT_SECT_SIZE) * nSectorNum);
}

//------------------------------------------------------------------------------

int ssfdc_block_mark_bad(unsigned int phys_block)
{
	nand_page_info_t	info;
	unsigned int		phys_page;
 	block_cache_t		*cache = &g_write_cache;

	ssfdc_block_lookup_unmap_entry(cache->virt_block);
	ssfdc_block_info_map_bad_block(phys_block);
	memset(&info, 0xFF, sizeof(info));

	info.block_status = 0x00;
	phys_page = phys_block * g_nand_page_per_block;

	phys_page += (g_nand_page_per_block - 1);
	NAND_WRITE_PAGE_INFO(0, phys_page, &info);

	ssfdc_block_find_free(&cache->new_phys_block);

	phys_block = cache->new_phys_block;
	ssfdc_block_lookup_map_entry(cache->virt_block, phys_block);

	return (phys_block);
}

//------------------------------------------------------------------------------

int ssfdc_block_program(unsigned int phys_block)
{
	block_cache_t		*cache  = &g_write_cache;
	nand_page_info_t	*info;
	unsigned int		page, phys_page = 0;

	int				ret = 0;
	unsigned char	*ext_ptr = g_write_cache.ext_data;

	phys_page = phys_block * g_nand_page_per_block;

	for (page = 0; page < g_nand_page_per_block; page ++)
	{
		info = (nand_page_info_t *)ext_ptr;
		memset(info, 0xFF, NAND_OOB_SIZE);

		if (page == 0)
		{
			info->lifetime = g_pBlockInfo[phys_block].lifetime;
			info->block_addr_field = cache->virt_block;
		}
		info->page_status = ~cache->dirty_page[page];
		ext_ptr += NAND_OOB_SIZE;
	}

	ret = NAND_WRITE_MULTIPAGE(	0,
								phys_page,
								g_nand_page_per_block,
								cache->dirty_page,
								cache->data,
								(nand_page_info_t *)cache->ext_data);

	if (ret < 0)
	{	  
		ssfdc_block_mark_bad(phys_block);
		return (-1);
	}

	return (0);
}

//------------------------------------------------------------------------------

static void ssfdc_block_mark_erase(unsigned int	phys_block)
{
	nand_page_info_t	info;
	unsigned int		phys_page;
	block_cache_t		*cache = &g_write_cache;

	memset(&info, 0xFF, sizeof(info));
	g_pBlockInfo[phys_block].lifetime++;

	info.lifetime = g_pBlockInfo[phys_block].lifetime;
	if (g_min_lifetime > info.lifetime)
		g_min_lifetime = info.lifetime;

	info.block_erase = 0x00;

	phys_page = phys_block * g_nand_page_per_block;
	phys_page += (g_nand_page_per_block - 1);	

	NAND_WRITE_PAGE_INFO(0, phys_page, &info);
}

//------------------------------------------------------------------------------

int ssfdc_nftl_flush_cache(void)
{
	block_cache_t	*cache = &g_write_cache;
	int		phys_block, ret;
	
	if (!cache->in_use)
		return (0);

	// Get the un-dirty data from old block
	NAND_READ_MULTIPAGE(0,
						cache->old_phys_block * g_nand_page_per_block,
						g_nand_page_per_block,
						cache->dirty_page,
						cache->data,
						(nand_page_info_t *)cache->ext_data);

	phys_block = cache->new_phys_block;
	
restart:
	do
	{
		ret = NAND_ERASE_BLOCK(0, phys_block);
		if (ret == 0)
			phys_block = ssfdc_block_mark_bad(phys_block);

	} while (ret < 0);

	ret = ssfdc_block_program(phys_block);
	//printf("phys_block = %x\n",phys_block);
	if (ret < 0)
	{
		phys_block = ssfdc_block_mark_bad(phys_block);
		goto restart;
	}

	if (cache->old_phys_block != cache->new_phys_block)
	{
		phys_block = cache->old_phys_block;
		ssfdc_block_mark_erase(phys_block);
	}
	cache->in_use = 0;

	return (0);
}

//------------------------------------------------------------------------------

void ssfdc_nftl_getInfo(unsigned int zone, pflash_info_t pflashinfo)
{
	pflashinfo->flashType = NANDFLASH;
	pflashinfo->dwBytesPerBlock = g_nand_block_size;
	pflashinfo->dwSectorsPerBlock = g_nand_page_per_block * SECTORS_PER_PAGE;
	pflashinfo->dwDataBytesPerSector = DEFAULT_SECT_SIZE;

	pflashinfo->dwFSStartBlock = g_start_phys_block;
	pflashinfo->dwFSTotalBlocks = g_total_virt_block;
	pflashinfo->dwFSTotalSectors = g_total_virt_block * g_nand_page_per_block * SECTORS_PER_PAGE;
}

//------------------------------------------------------------------------------

static int ssfdc_init_block_mapping(void)
{
	nand_page_info_t	info, info2;
	unsigned int		phys_page, phys_block, virt_block;

	for (virt_block = 0; virt_block < g_total_virt_block; virt_block++)
	{
		g_pBlockLookup[virt_block] = 0;
	}

	for (phys_block = 0; phys_block < g_total_phys_block; phys_block++)
	{
		phys_page = phys_block * g_nand_page_per_block;

		NAND_READ_PAGE_INFO(0, phys_page, &info);
		NAND_READ_PAGE_INFO(0, phys_page + g_nand_page_per_block - 1, &info2);

		if ((info.block_status & info2.block_status) != SSFDC_BLOCK_STATUS_NORMAL)
		{
			ssfdc_block_info_map_bad_block(phys_block);
			continue;
		}

		if (info2.block_erase == 0x00)
		{
			if (info2.lifetime == 0xFFFFFFFF)
				g_pBlockInfo[phys_block].lifetime = 0x00000000;
			else
				g_pBlockInfo[phys_block].lifetime = info2.lifetime;
			virt_block = 0xFFFFFFFF;
		}
		else
		{
			if (info.lifetime == 0xFFFFFFFF)
				g_pBlockInfo[phys_block].lifetime = 0x00000000;
			else
				g_pBlockInfo[phys_block].lifetime = info.lifetime;
			virt_block = info.block_addr_field;
		}

		g_pBlockInfo[phys_block].tag = SSFDC_BLOCK_TAG_FREE;
		g_pBlockInfoIndex[phys_block + 1] = phys_block;

		ssfdc_block_lookup_map_entry(virt_block, phys_block);
		if (g_min_lifetime > g_pBlockInfo[phys_block].lifetime)
			g_min_lifetime = g_pBlockInfo[phys_block].lifetime;
	}

	return (0);
}

//------------------------------------------------------------------------------

static int ssfdc_init_variable(void)
{
	flash_info_t	flashinfo;

	jz_nand_get_info(&flashinfo);

	g_nand_page_size = flashinfo.dwDataBytesPerSector;
	g_nand_block_size = flashinfo.dwDataBytesPerSector * flashinfo.dwSectorsPerBlock;
	g_nand_page_per_block = flashinfo.dwSectorsPerBlock;

	g_start_phys_block = flashinfo.dwFSStartBlock;
	g_start_phys_page = flashinfo.dwFSStartBlock * flashinfo.dwSectorsPerBlock;
	g_total_phys_block = flashinfo.dwFSTotalBlocks;
	g_MaxBadBlocks = flashinfo.dwMaxBadBlocks;
	g_total_virt_block = g_total_phys_block - g_MaxBadBlocks;

	g_dirty_page = (unsigned char *)malloc(g_nand_page_per_block * sizeof(unsigned char));
	g_cache_data = (unsigned char *)malloc(g_nand_block_size * sizeof(unsigned char));
	(unsigned int)g_cache_data |= 0xa0000000; // enable uncache
	 
	g_read_data = (unsigned char *)malloc(g_nand_page_size * sizeof(unsigned char));
	(unsigned int)g_read_data |= 0xa0000000; // enable uncache
	g_ext_data = (unsigned char *)malloc(g_nand_page_per_block * NAND_OOB_SIZE * sizeof(unsigned char));

	if ( !g_dirty_page || !g_cache_data || !g_read_data || !g_ext_data )
		return (-ENOMEM);

	g_write_cache.data = (unsigned char *)g_cache_data;
	g_write_cache.ext_data = (unsigned char *)(g_ext_data);

	g_bad_block = 0;
	g_free_phys_block = 0;
	g_free_virt_block = 0;
	g_used_block = 0;
	g_cur_phy_block = 0;

	g_write_cache.in_use = 0;
	g_write_cache.virt_block = 0; 
	g_write_cache.old_phys_block = 0;
	g_write_cache.new_phys_block = 0;
	
	g_write_cache.dirty_page = g_dirty_page;

	g_free_phys_block = g_total_phys_block;
	g_free_virt_block = g_total_virt_block;
	g_read_page_cache.data = (unsigned char *)g_read_data;
	g_read_page_cache.page_num = -1;

	g_pBlockInfo = (block_info_t *)malloc(g_total_phys_block * sizeof(block_info_t));
	g_pBlockLookup = (unsigned int *)malloc( g_total_virt_block * sizeof(unsigned int));
	g_pBlockInfoIndex = (unsigned int *)malloc((g_total_phys_block + 1) * sizeof(unsigned int));
	if ( !g_pBlockInfo || !g_pBlockLookup || !g_pBlockInfoIndex )
		return (-ENOMEM);

	return (0);
}

//------------------------------------------------------------------------------

int ssfdc_nftl_open(void)
{
	if (ssfdc_init_variable() < 0)
		return (-ENOMEM);

	if (ssfdc_init_block_mapping())
		return (-EIO);

	return (0);
}

//------------------------------------------------------------------------------

int ssfdc_nftl_init(void)
{
	if (jz_nand_init() == 0)
		return (-EIO);

	return (0);
}

//------------------------------------------------------------------------------

static void ssfdc_deinit(void)
{
	free(g_pBlockLookup);
	free(g_pBlockInfo);
	free(g_pBlockInfoIndex);

	g_pBlockLookup = 0;
	g_pBlockInfo = 0;
	g_pBlockInfoIndex = 0;

	free(g_dirty_page);
	(unsigned int)g_cache_data &= (~0x20000000);  // enable cache
	free(g_cache_data);
	(unsigned int)g_read_data &= (~0x20000000);  // enable cache
	free(g_read_data);
	free(g_ext_data);
}

//------------------------------------------------------------------------------

int ssfdc_nftl_release(void)
{
	ssfdc_nftl_flush_cache();
	ssfdc_deinit();

	return (0);
}

//------------------------------------------------------------------------------

int ssfdc_nftl_format(void)
{

	unsigned int		phys_block;
	nand_page_info_t	info;
	int		ret;

	memset(&info, 0xFF, sizeof(nand_page_info_t));
	info.block_status = 0x00;
	for (phys_block = 0; phys_block < g_total_phys_block; phys_block ++)
	{
		ret = NAND_ERASE_BLOCK(0, phys_block);
		if (ret == 0)
		{
				nand_page_info_t	info;
	      unsigned int phys_page;
	      memset(&info, 0xFF, sizeof(info));
	      info.block_status = 0x00;
	      phys_page = phys_block * g_nand_page_per_block;
	      phys_page += (g_nand_page_per_block - 1);
	      NAND_WRITE_PAGE_INFO(0, phys_page, &info);
		}
	}
	g_bad_block = 0;
	g_free_phys_block = 0;
	g_free_virt_block = 0;
	g_used_block = 0;
	g_cur_phy_block = 0;

	g_write_cache.in_use = 0;
	g_write_cache.virt_block = 0; 
	g_write_cache.old_phys_block = 0;
	g_write_cache.new_phys_block = 0;

	if (ssfdc_init_block_mapping())
		return (-EIO);
	return (0);
}

//------------------------------------------------------------------------------