/*
 * mm.c - correct version
 * 
 * In our approach, a block is allocated and freed by encoding its block size and allocated bits, which are stored in the header and footer.
 * The allocated bits are set to one when allocated and zero when freed.
 * The block structure includes the payload (only in allocated blocks), and a header and footer of 4 bytes (WSIZE). 
 * 
 * In our implicit free list the allocator can traverse the entire heap list and search for free blocks
 * by looking at the allocated bits and checking for an appropriate size. After finding an appropriate free block,
 * our allocator places the desired block into this free block.
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

#define PACK(size, alloc) ((size) | (alloc))                                //sets size and allocated bit into a word

#define GET(p) (*(unsigned int *)(p))                                       //reads a word at address p
#define PUT(p, val) (*(unsigned int *)(p) = (val))                          //puts in val at specified address p


#define GET_SIZE(p) (GET(p) & ~0x7)                                         //gets size from specified address p
#define GET_ALLOC(p) (GET(p) & 0x1)                                         //gets allocated bit by reading allocated field from address p

#define HDRP(bp)   ((char *)(bp) - WSIZE)                                   //retrieves address of header from given address bp
#define FTRP(bp)   ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)              //retrieves address of footer from given address bp


#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))    //retrieves address of block next to bp in memory
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))    //retrieves address of block previous to bp in memory

static char *heap_listp;   //creates pointer at beginning of our heap


/* find_fit 
 * takes in an adjusted size and traverses the list to find a block that can hold the adjusted size
 * returns that block
 * if no block is found then return NULL
*/

static void *find_fit(size_t asize)
{
  void *bp;

  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {       //traverses through heap
    if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){             //if block is free and adjusted size <= size of bp 
      return bp;
    }
  }
 return NULL;
  /* #endif */
}

/* place 
 * takes in a block pointer and an adjusted size
 * if the remainder of the size of the pointer minus the adjusted size is greater than the minimum block size
 * set the allocated bits in the header and footer to 1
 * then split the block
 * set the allocated bits of the header and footer of the remainder block to 0
 * otherwise set the allocated bits in the header and the footer of the block pointer to 1 
*/

static void place(void *bp, size_t asize)
{
  size_t csize = GET_SIZE(HDRP(bp));

  if ((csize - asize) >= (2*DSIZE)) {          //split block
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize - asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
  }

  else{                                        //don't split
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
} 

/* 
 * Coalesce combines free blocks that are adjacent in memory
 * We do this by setting up four different cases in which adjacent blocks would either be allocated or free
 * return the coalesced block pointer
*/

static void *coalesce(void *bp)
 {
   size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
   size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
   size_t size = GET_SIZE(HDRP(bp));

   if (prev_alloc && next_alloc) {                                          //case 1: neither adjacent block is free
     return bp;
   }

   else if (prev_alloc && !next_alloc){                                     //case 2: next block in memory is free, previous is allocated
     size += GET_SIZE(HDRP(NEXT_BLKP(bp)));                                 //add size of next block to size of bp
     PUT(HDRP(bp), PACK(size, 0));
     PUT(FTRP(bp), PACK(size, 0));
   }

   else if (!prev_alloc && next_alloc) {                                    //case 3: previous block in memory is free, next is allocated
     size += GET_SIZE(HDRP(PREV_BLKP(bp)));                                 //add size of previous block to size of bp
     PUT(FTRP(bp), PACK(size, 0));
     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
     bp = PREV_BLKP(bp);
   }

   else {                                                                    //case 4: both previous block and next block are free
     size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));  //add size of previous block and size of next block
     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
     PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
     bp = PREV_BLKP(bp);
   }

   return bp;
}

/* extend_heap 
 * extends the heap with a new free block that maintains allignment
 * Initializes the header, footer and epilogue header
 * Then coalesces if the previous block is free
*/

static void *extend_heap(size_t words)
{
  char *bp;
  size_t size;

  size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; //ensures that size fits alignment requirements
  if ((long)(bp = mem_sbrk(size)) == -1)
    return NULL;

  PUT(HDRP(bp), PACK(size, 0));                //initializes header of new free block
  PUT(FTRP(bp), PACK(size, 0));                //initializes footer of new free block
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));        //initializes epilogue header

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
  PUT(heap_listp, 0);                               //alignment padding
  PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));      //prologue header
  PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));      //prologue footer
  PUT(heap_listp + (3*WSIZE), PACK(0, 1));          //epilogue header
  heap_listp += (2*WSIZE);                       

      /*extend the empty heap with a free block of CHUNKSIZE bytes */
      if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
	return -1;      
 return 0;
}
 

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 *
 * mm_malloc takes in a size to allocate and checks if it meets the allignment requirement
 * It then uses find_fit to find the appropriate block then uses the place function to allocate the block with the given size
 * if it does not find a block then it extends the heap and allocates the new free block with the given size
 */
 
void *mm_malloc(size_t size)
{
  size_t asize;                                               //initialize variables
  size_t extendsize;
  char *bp;

  if (size == 0)                                              //ignore requests of size 0
    return NULL;

  if (size <= DSIZE)                                          //check if size is less than minimum alignment requirement
    asize = 2*DSIZE;                                          //adjust size to meet minimum alignment requirement

  else
    asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);   //adjust size to incorporate header and footer sizes and alignment requirement
  
  if ((bp = find_fit(asize)) != NULL) {                       //search heap for appropriately sized free block
    place(bp, asize);
    return bp;
  }

  extendsize = MAX(asize, CHUNKSIZE);                         //if no appropriately sized block is found, extend heap

  if ((bp = extend_heap(extendsize/WSIZE)) == NULL)           //place block within new free block in the extended heap
    return NULL;
  place(bp, asize);
  return bp;
 
 
}

/* mm_free
 * takes in a pointer
 * sets the allocated bits of the pointer to zero
 * calls coalesce to combine any adjacent free blocks
 * (coalesce also takes care of the case where there are no adjacent free blocks)
 */

void mm_free(void *ptr)
{

  size_t size = GET_SIZE(HDRP(ptr));

  PUT(HDRP(ptr) , PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));

  coalesce(ptr);
}



/*
 * mm_realloc
 * functions identically to malloc and free unless the block size needs to be changed
 * if so, it changes the size of the block and copies the existing data into the newly sized block
 */
 
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);

    if (newptr == NULL)                                                       //ignore if pointer points to NULL
      return NULL;

    copySize = GET_SIZE(HDRP(ptr));

    if (size < copySize)
      copySize = size;

    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);

    return newptr;
}


/* 
 * heap checker - traverses through the heap list to see if 
 * 1. the size of the header and footer of each block are equal
 * 2. the allocated bits of the header and footer of each block are equal
 * returns 0 if there is an error
*/

int  *mm_check(void){
  void *bp;

  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){

    if (GET_SIZE(HDRP(bp)) != GET_SIZE(FTRP(bp))){                                  //case 1
      printf("Header and footer not same");
      return 0;
    }

    if (GET_ALLOC(HDRP(bp)) != GET_ALLOC(FTRP(bp))){                                //case 2
       	return 0;
    }
  }
}









