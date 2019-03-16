/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
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
#define GET(p) (*(unsigned int *)(p)) //int
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

#define prev(bp) (char  *)(bp)
#define succ(bp) ((char  *)(bp) + DSIZE)

#define SET_PREV(bp, qp) (prev(bp) = qp)
#define SET_NEXT(bp, qp) (succ(bp) = qp)


static void insert_at_front(char *bp){
  SET_NEXT(bp, free_listp);//PUT(succ(bp),free_listp);
  SET_PREV(free_listp, bp);//PUT(prev(free_listp),bp);
  *prev(bp) = NULL;
  free_listp = bp; 

}

static void remove_block_free(char *bp){
  if(prev(bp) != NULL){
    SET_NEXT(prev(bp),succ(bp));              // *succ(prev(bp)) = succ(bp);
  }

  else{
    free_listp = succ(bp);
  }

  SET_PREV(succ(bp),prev(bp));     // *prev(succ(bp)) = prev(bp);

}







static void *find_fit(size_t asize)
{
  void *bp;

   for (bp = free_listp; bp != NULL; bp = succ(bp)) {
    if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
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
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize - asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
  }
  else{
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
} 

static void *coalesce(void *bp)
 {
   size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
   size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
   size_t size = GET_SIZE(HDRP(bp));

   if (prev_alloc && next_alloc) {
     return bp;
   }
   else if (prev_alloc && !next_alloc){
     size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
     PUT(HDRP(bp), PACK(size, 0));
     PUT(FTRP(bp), PACK(size, 0));
     *succ(prev(NEXT_BLKP(bp))) = succ(NEXT_BLKP(bp));
     insert_at_front(bp);
     //bp = NEXT_BLKP(PREV_BLKP(NEXT_BLKP(bp)));
     //bp = PREV_BLKP(NEXT_BLKP(NEXT_BLKP(bp)));
     //NEXT_BLKP(NEXT_BLKP(bp)) = NEXT_BLKP(bp);
     //PREV_BLKP(NEXT_BLKP(bp)) = PREV_BLKP(bp);
   }
   else if (!prev_alloc && next_alloc) {
     //size += GET_SIZE(HDRP(PREV_BLKP(bp)));
     size += GET_SIZE(HDRP(PREV_BLKP(bp)));
     PUT(FTRP(bp), PACK(size, 0));
     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
     bp = PREV_BLKP(bp);
   }

   else {
     size += GET_SIZE(HDRP(PREV_BLKP(bp)))+ GET_SIZE(FTRP(NEXT_BLKP(bp)));
     //size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
     PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
     //bp = PREV_BLKP(bp);
     *succ(PREV_BLKP(bp)) = succ(NEXT_BLKP(bp));
     //NEXT_BLKP(PREV_BLKP(NEXT_BLKP(bp))) = NEXT_BLKP(NEXT_BLKP(bp));
     //PREV_BLKP(NEXT_BLKP(NEXT_BLKP(bp))) = PREV_BLKP(NEXT_BLKP(bp));
     
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
  asize = MAX(ALIGN(size + 2*DSIZE), ALIGNMENT); 
  
  if ((bp = find_fit(asize)) != NULL) {
    remove_block_free(bp);
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

  // insert_at_front(ptr);

  
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











