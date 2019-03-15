/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * For our free list, we have decided to put each free block in the beginning o * f our free list. We do this through our function insert_at_front, and it is
 * done after coalescing. Our allocator then stitches together the list. 


  */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
  "team awesome",
    /* First member's full name */
    "Akhil Kambhammettu",
    /* First member's NetID */
    "akt7491",
    /* Second member's full name (leave blank if none) */
    "Annika Amlie",
    /* Second member's NetID */
    "Ava7121"
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x, y) ((x) > (y)? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))
//static void cheese (size_t somethingy){printf("here");}
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
//static void cheese2 (size_t somethingy){printf("2");}

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp)   ((char *)(bp) - WSIZE)
#define FTRP(bp)   ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)


#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
//static void cheese3 (size_t somethingy){printf("3");}
static char *heap_listp;
static char *free_listp;
//static char *alloc_listp;
//NEXT_BLKP(*free_listp)=NULL;

#define prev(bp) *(char * *)(bp)
#define succ(bp) *((char * *)(bp) + sizeof(bp))

#define PUT_PREV(bp, val) (prev(bp) = val)//*bp = val
#define PUT_SUCC(bp, val) (succ(bp) = val)//*((char *)(bp) + sizeof(bp)) = val




static void insert_at_front(char *bp){ //inserts a block at the front of the free list
  if (free_listp != NULL){
  PUT_SUCC(bp,free_listp); //succ(bp) = free_listp 
  PUT_PREV(free_listp,bp); //prev(free_listp) = bp
  PUT_PREV(bp, NULL); 
  free_listp = bp; //move free list pointer to bp because it is now at the front
  }
  else {
    free_listp = bp;
    PUT_SUCC(bp, NULL);
    PUT_PREV(bp, NULL);
  }
}

static void remove_block_free(char *bp){ //removes block from free list
  if(prev(bp) != NULL && succ(bp) != NULL){ //if desired block is in middle of free list
    PUT_SUCC(prev(bp), succ(bp)); //
    PUT_PREV(succ(bp), prev(bp)); //always make successor of bp's prev point to what bp's prev was
  }

  else if(prev(bp) == NULL && succ(bp) != NULL){
    free_listp = succ(bp); //move free list pointer to next block
  }

  else if(prev(bp) != NULL && succ(bp) == NULL){  //if desired block is at front of free list
    PUT_SUCC(prev(bp), NULL);
  }

  else{  //if there's only one in the free list
    free_listp = NULL;
  }
}


//static void *next_fit(size_t asize)
/* Next fit search 
  char *curr = free_listp;

for (curr; GET_SIZE(HDRP(curr))>0; curr = succ(curr))
  if (asize <= GET_SIZE(HDRP(curr)))
    return curr;


/* Search from the rover to the end of list 
for ( ; GET_SIZE(HDRP(rover)) > 0; rover = NEXT_BLKP(rover))
  if (!GET_ALLOC(HDRP(rover)) && (asize <= GET_SIZE(HDRP(rover))))
  return rover;*/

// search from start of list to old
 
static void *find_fit(size_t asize) //next fit approach
{

  /* char *curr = free_listp;
  char *oldcurr = curr;
  
  for ( ; GET_SIZE(HDRP(curr))>0; curr = succ(curr))
    if (asize <= GET_SIZE(HDRP(curr)))
      return curr;
  for (curr; curr < oldcurr; curr = NEXT_BLKP(curr))
    if (asize <= GET_SIZE(HDRP(curr)))
	return curr;
	
return NULL;
  */

  //!GET_ALLOC(HDRP(bp))

  void *bp;

  for (bp = free_listp; bp != NULL; bp = succ(bp)) { 
  
    if (asize <= GET_SIZE(HDRP(bp))){
      return bp;
    }
   }

 return NULL;
  /* #endif */
}



static void place(void *bp, size_t asize)
{
  size_t csize = GET_SIZE(HDRP(bp));

  if ((csize - asize) >= (2*DSIZE)) {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    remove_block_free(bp);
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize - asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
  }
  else{
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
    remove_block_free(bp);
  }
} 

static void *coalesce(void *bp)
 {
   size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
   size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
   size_t size = GET_SIZE(HDRP(bp));
   size_t prevsize = GET_SIZE(HDRP(PREV_BLKP(bp)));

   if (prev_alloc && next_alloc) {
     return bp;
     insert_at_front(bp);
   }
   else if (prev_alloc && !next_alloc){

     remove_block_free(NEXT_BLKP(bp));
     size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
     PUT(HDRP(bp), PACK(size, 0));                                                                            
     PUT(FTRP(bp), PACK(size, 0)); 
     insert_at_front(bp);

     /*
     size += GET_SIZE(HDRP(NEXT_BLKP(bp))); //add size of bp to size of NEXT
     PUT(HDRP(bp), PACK(size, 0));                                                                            
     PUT(FTRP(bp), PACK(size, 0)); 
     bp = NEXT_BLKP(bp); //move bp to point to NEXT which now holds size of bp
     */
     // size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
     // PUT(HDRP(bp), PACK(size, 0));
     // PUT(FTRP(bp), PACK(size, 0));
     // *succ(prev(NEXT_BLKP(bp))) = succ(NEXT_BLKP(bp));
     // insert_at_front(bp);

     //bp = NEXT_BLKP(PREV_BLKP(NEXT_BLKP(bp)));
     //bp = PREV_BLKP(NEXT_BLKP(NEXT_BLKP(bp)));
     //NEXT_BLKP(NEXT_BLKP(bp)) = NEXT_BLKP(bp);
     //PREV_BLKP(NEXT_BLKP(bp)) = PREV_BLKP(bp);
   }
   else if (!prev_alloc && next_alloc) {
     remove_block_free(PREV_BLKP(bp));
     size += prevsize;
     PUT(HDRP(bp), PACK(size, 0));                                                                            
     PUT(FTRP(bp), PACK(size, 0));
     insert_at_front(bp);

 /*
//size += GET_SIZE(HDRP(PREV_BLKP(bp)));
     free_listp = succ(bp);
     prevsize += size; //add size to size of prev
     PUT(FTRP(bp), PACK(size, 0));
     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
     bp = PREV_BLKP(bp);
     */
   }

   else {

     remove_block_free(PREV_BLKP(bp));
     remove_block_free(NEXT_BLKP(bp));
     size += (prevsize + GET_SIZE(HDRP(bp)));
     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
     PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
     insert_at_front(bp);


     /*
     free_listp = succ(bp);
     remove_block_free(NEXT_BLKP(bp)); //remove S
     size += GET_SIZE(HDRP(PREV_BLKP(bp)))+ GET_SIZE(FTRP(NEXT_BLKP(bp)));
     //size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
     PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
     //bp = PREV_BLKP(bp);
     *succ(PREV_BLKP(bp)) = succ(NEXT_BLKP(bp));
     //NEXT_BLKP(PREV_BLKP(NEXT_BLKP(bp))) = NEXT_BLKP(NEXT_BLKP(bp));
     //PREV_BLKP(NEXT_BLKP(NEXT_BLKP(bp))) = PREV_BLKP(NEXT_BLKP(bp));
     */
   }
   return bp;
}



static void *extend_heap(size_t words)
{
  char *bp;
  size_t size;

  size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
  if ((long)(bp = mem_sbrk(size)) == -1)
    return NULL;

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

  return coalesce(bp);
}
 
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  /*Create the initial empty heap */
  if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
      return -1;
      PUT(heap_listp, 0);
      PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
      PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
      PUT(heap_listp + (3*WSIZE), PACK(0, 1));
      heap_listp += (2*WSIZE);

      /*extend the empty heap with a free block of CHUNKSIZE bytes */
      if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
	return -1;      
 return 0;
}
 

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
  size_t asize;
  size_t extendsize;
  char *bp;

  if (size == 0)
    return NULL;

  /* 
 if (size <= DSIZE)
    asize = 2*DSIZE;
  else
    asize = ALIGN (size + DSIZE + DSIZE);
    //asize = DSIZE * ((size + (DSIZE) + (DSIZE-1) / DSIZE);
 
    */
  asize = MAX(ALIGN(size + DSIZE), ALIGNMENT); 
  
  if ((bp = find_fit(asize)) != NULL) {
    //remove_block_free(bp);
    place(bp, asize);
    return bp;
  }

  extendsize = MAX(asize, CHUNKSIZE);
  if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
    return NULL;
  place(bp, asize);
  return bp;
 

  /* int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
	}*/
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  size_t size = GET_SIZE(HDRP(ptr));

  PUT(HDRP(ptr) , PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  coalesce(ptr);
  
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(ptr));// *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
    


}


/* int mm_check(void) {

  char *bp;
  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
    if ((GET_SIZE(HDRP(bp))== GET_SIZE(FTRP(bp)))&&(GET_ALLOC(HDRP(bp))==GET_ALLOC(FTRP(bp)))&&(GET_SIZE(HDRP(bp))%8==0))
      return 0;
  }
}
*/










