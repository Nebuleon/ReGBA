#include<stdio.h>


#define MEM_U8 unsigned char 
#define MEM_ULONG unsigned int 
#define MEM_LENGHT (0x0200000)  //2MB


#define TRUE		1
#define FALSE		0

#define FREE		0
#define RESERVED	1
static const char *statStr[] = {"FREE","RESERVED"};

/* Memory block header = 9 bytes = 4 previous + 4 next + status */

#define SIZE_HEADER	16

//#define prev(i)		*((MEM_ULONG *)&memory[i])
//#define prev(i)		((MEM_ULONG)*((MEM_ULONG *) (i)))
//#define next(i)		((MEM_ULONG)*((MEM_ULONG *) (i+4)))
#define prev(i)		(*((MEM_ULONG *) (i)))
#define next(i)		(*((MEM_ULONG *) (i+4)))
//#define status(i)	((MEM_U8)*((MEM_U8 *) (i+8)))
//#define setalign(i,y)  ((MEM_U8)*((MEM_U8 *)(i-1))) = y
//#define getalign(i)  ((MEM_U8)*((MEM_U8 *)(i-1)))
#define status(i)	(*((MEM_U8 *) (i+8)))
#define setalign(i,y)  (*((MEM_U8 *)(i-1))) = y
#define getalign(i)  (*((MEM_U8 *)(i-1)))

#define size(i)		(next(i)-i-SIZE_HEADER)

/* if going to split free block, need at least 8 bytes in new free part */

#define MIN_FREE_BYTES   4

static MEM_U8 memory[MEM_LENGHT] __attribute__ ((aligned (MIN_FREE_BYTES)));

static MEM_ULONG first = 0; /*stores address of first byte of heap*/
static MEM_ULONG last = MEM_LENGHT; /*store address of last byte of heap + 1*/

void heapInit()
{
	first = (MEM_ULONG)&memory[0];
	last = (MEM_ULONG)&memory[MEM_LENGHT - 1];
	
	prev(first)=0;
	next(first)=0;
	status(first)=FREE;

	return;

}/*end heapInit*/

static int currentNodeAlloc(MEM_ULONG i,MEM_ULONG nbytes)
{
	MEM_ULONG free_size;

	/*handle case of current block being the last*/
  //printf("currentNodeAlloc addr = 0x%08x size = 0x%x\r\n",i,nbytes);

 	if(next(i) == 0){
		free_size = last - i - SIZE_HEADER;
	}else{
	    free_size = size(i);
	}
    //printf("currentNodeAlloc freesize = 0x%x\r\n", free_size);
	
	/*either split current block, use entire current block, or fail*/
    
	if(free_size >= nbytes + SIZE_HEADER + MIN_FREE_BYTES)
	{
		MEM_ULONG old_next;
		MEM_ULONG old_block;
		MEM_ULONG new_block;

		old_next = next(i);
		old_block = i;

		/*fix up original block*/

		next(i) = i+SIZE_HEADER+nbytes;	
		new_block = next(i);
		status(i)=RESERVED;

		/*set up new free block*/

		i = next(i);					
		next(i)=old_next;
		prev(i)=old_block;
		status(i)=FREE;

		/*right nieghbor must point to new free block*/

		if(next(i)!=0)			
		{
			i = next(i);
			prev(i)=new_block;
		}
		
		return(TRUE);
	}
	else if(free_size >= nbytes)
	{
		status(i)=RESERVED;
		return(TRUE);
	}

	return(FALSE);

}/*end currentNodeAlloc*/
static MEM_ULONG my_alloc(MEM_ULONG nbytes)
{
	int ret;
	MEM_ULONG i;
 //printf("alloc %x\n",nbytes);
	nbytes = ((nbytes  + MIN_FREE_BYTES - 1)/ MIN_FREE_BYTES )  * MIN_FREE_BYTES;
	i=first;
	if(status(i)==FREE)
	{
		ret = currentNodeAlloc(i,nbytes);
		if(ret==TRUE)
		{
			//printf("alloc mem = 0x%x,size = %d\n",i+SIZE_HEADER,nbytes); 
			return(i+SIZE_HEADER);
		}
	}

	while(next(i)!=0)
	{
		i=next(i);
		if(status(i)==FREE)
		{
			ret = currentNodeAlloc(i,nbytes);
			if(ret==TRUE)
			{
				//printf("alloc mem = 0x%x,size = %d\n",i+SIZE_HEADER,nbytes); 
				return(i+SIZE_HEADER);
			}
		}
	}

    return 0;
}
MEM_ULONG alloc(MEM_ULONG nbytes)
{
//  unsigned int reg_ra;

//  __asm__ __volatile__("or %0, $0, $ra"
//                        : "=r" (reg_ra)
//                        :);
  
//  printf("alloc ra= %08x\n", reg_ra);

	MEM_ULONG i = my_alloc(nbytes);
	if(i != 0)
		setalign(i,0);    
	return i;
}/*end alloc*/

#if 0
MEM_ULONG alignAlloc(MEM_ULONG align,MEM_ULONG size)
{
	MEM_ULONG i = my_alloc(size + align);
	if(i != 0)
	{
		align = i - (i / align) * align;
		i += align;
		setalign(i,align);    
	}
	return i;
}/*end alloc*/
#endif

static void my_free(MEM_ULONG address)
{
	MEM_ULONG block;
	MEM_ULONG lblock;
	MEM_ULONG rblock;
  
	block = address-SIZE_HEADER;
	lblock = prev(block);
	rblock = next(block);

	/*
	4 cases: FFF->F, FFR->FR, RFF->RF, RFR 
	always want to merge free blocks 
	*/

	if((lblock!=0)&&(rblock!=0)&&(status(lblock)==FREE)&&(status(rblock)==FREE))
	{
		next(lblock)=next(rblock);
		status(lblock)=FREE;
		if(next(rblock)!=0){ prev(next(rblock))=lblock; }
	}
	else if((lblock!=0)&&(status(lblock)==FREE))
	{
		next(lblock)=next(block);
		status(lblock)=FREE;
		if(next(block)!=0){ prev(next(block))=lblock; }
	}
	else if((rblock!=0)&&(status(rblock)==FREE))
	{
		next(block)=next(rblock);
		status(block)=FREE;
		if(next(rblock)!=0){ prev(next(rblock))=block; }
	}
	else{ status(block)=FREE; }

}
/*Note: disaster will strike if fed wrong address*/

void deAlloc(MEM_ULONG address)
{
	address -= getalign(address);
	my_free(address);
}/*end deAlloc*/


MEM_ULONG Drv_realloc(MEM_ULONG address,MEM_ULONG nbytes)
{
	
	MEM_ULONG rr,addr,oldsize;
	MEM_ULONG block,rblock,rrblock;
	MEM_ULONG bsize,rbsize,align;
	oldsize = nbytes;
	nbytes = ((nbytes  + MIN_FREE_BYTES - 1)/ MIN_FREE_BYTES )  * MIN_FREE_BYTES;
	
	rr = address;
	if (nbytes == 0) {
			deAlloc(rr);
			return 0;
  }
	if (address == 0)
	{	
	    addr = my_alloc(nbytes);
	    if(addr != 0)
	    	setalign(addr,0);
	    return addr;
	}
	align = getalign(address);
	address -= getalign(address);
	address -= SIZE_HEADER;    
	bsize = size(address);
	//printf("align = %d,address = %x  %d %d\n",align,address,nbytes,bsize);
	if(nbytes <= bsize-align)
	{
		return rr;
	}	
	
	rblock = next(address);
	if((rblock != 0) &&(status(rblock) == FREE) )
	{
		//printf("rblock = %x %d %d",rblock,status(rblock),size(rblock));
		bsize += size(rblock);
		if(bsize >= nbytes + align)
		{
			rrblock = next(rblock);
			next(address) = (address + nbytes + align + SIZE_HEADER);
			block = next(address);
			prev(block) = address;
			next(block) = rrblock; 
			status(block) = FREE;
			return rr;
	  }
	}   
	addr = my_alloc(nbytes+align);
	//printf("realloc %x %x %x %x\n",addr,rr,nbytes,bsize);
	
	if(addr == 0)
		return 0;
  addr += align;		
	setalign(addr,align);	
	memcpy(addr,rr,bsize);
	deAlloc(rr);
  
  return addr;	
}
MEM_ULONG Drv_calloc(MEM_ULONG size,MEM_ULONG n)
{
//  unsigned int reg_ra;

//  __asm__ __volatile__("or %0, $0, $ra"
//                        : "=r" (reg_ra)
//                        :);
  
//  printf("calloc ra= %08x\n", reg_ra);

	MEM_ULONG rr;
	rr = alloc(size * n);
	memset((void *)rr,0,size * n);
	return rr;
}

void printMemory()
{
	MEM_ULONG i;
	i=first;
	printf("[0x%08x,0x%08x,0x%08x,%s]\n",prev(i),i,next(i),statStr[status(i)]);
	while(next(i)!=0)
	{
		i=next(i);
		printf("[0x%08x,0x%08x,0x%08x,%s]\n",prev(i),i,next(i),statStr[status(i)]);
	}
	return;

}/*end printMemory*/

/*
void main()
{
	MEM_ULONG address[10];
	heapInit();

	printf("Allocating----------------\n");
	address[0]=alloc(10); printf("\taddress= %I64u\n",address[0]);
	address[1]=alloc(20); printf("\taddress= %I64u\n",address[1]);
	address[2]=alloc(15); printf("\taddress= %I64u\n",address[2]);
	address[3]=alloc(40); printf("\taddress= %I64u\n",address[3]);
	address[4]=alloc(5); printf("\taddress= %I64u\n",address[4]);
	address[5]=alloc(1); printf("\taddress= %I64u\n",address[5]);
	address[6]=alloc(2); printf("\taddress= %I64u\n",address[6]);
	address[7]=alloc(200); printf("\taddress= %I64u\n",address[7]);
	address[8]=alloc(1); printf("\taddress= %I64u\n",address[8]);
	printf("Dump map------------------\n");
	printMemory();
	printf("\nDeallocating address[8]+++\n");deAlloc(address[8]);printMemory();
	printf("\nDeallocating address[7]+++\n");deAlloc(address[7]);printMemory();
	printf("\nDeallocating address[1]+++\n");deAlloc(address[1]);printMemory();
	printf("\nDeallocating address[3]+++\n");deAlloc(address[3]);printMemory();
	printf("\nDeallocating address[5]+++\n");deAlloc(address[5]);printMemory();
	printf("\nDeallocating address[4]+++\n");deAlloc(address[4]);printMemory();
	printf("\nDeallocating address[0]+++\n");deAlloc(address[0]);printMemory();
	printf("\nDeallocating address[2]+++\n");deAlloc(address[2]);printMemory();
	printf("\nDeallocating address[6]+++\n");deAlloc(address[6]);printMemory();
	return;

}*/
