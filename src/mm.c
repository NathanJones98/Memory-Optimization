/*
 * ECE454 Lab 3 - Malloc
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "memorygoburrrrrrrr",
    /* First member's first name and last name */
    "Nathan:Jones",
    /* First member's student number */
    "1005003023",
    /* Second member's first name and last name (do not modify if working alone) */
    "",
    /* Second member's student number (do not modify if working alone) */
    ""
};


/*************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
*************************************************************************/
#define WSIZE       sizeof(void *)            /* word size (bytes) */
#define DSIZE       (2 * WSIZE)            /* doubleword size (bytes) */
#define CHUNKSIZE   (1<<7)      /* initial heap size (bytes) */

#define MAX(x,y) ((x) > (y)?(x) :(y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(uintptr_t *)(p))
#define PUT(p,val)      (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/**********************************************************
 * Segmented List Functions
 * Find index of segmented list for data of corrosponding size
 **********************************************************/

//Has to be big enough to store 16 bytes + 2 words (Header and footer)
#define BLOCK_SIZE 32

//Number of segmented lists
#define SEG_LIST_SIZE 21

void* heap_listp = NULL;

//Node in the segmented list
typedef struct freeblock{
        struct freeblock * next;
        struct freeblock * prev;
}freeblock;

//List of each explicit circular linked list
freeblock * segLists[SEG_LIST_SIZE];

void testhalt(char * name){
    if(true){
        printf("\nLocation: %s\n", name);
        char str1[20];
        scanf("%19s", str1);
    }
}

void printList(freeblock * head){
        if(head != NULL){
            struct freeblock * temp = head;
            do{
                printf("\nBlock: %p\n", temp);
                printf("prev: |%p| next: |%p| \n", temp->prev, temp->next);
                temp = temp->next;
            } while (head != temp);
        } else {
            printf("\nList is empty\n");
        }
}

//Find the index of segmented list that is larger than asize
int segIndex(size_t asize){
    
    int index = 0;
    bool index_found = false;

    do{
        if((asize >>= 1) <= 0){
            index_found = true;
        }
        index++;

        //printf("\nAsize: %ld Index Found: %d...", asize, index); 
    } while(index < SEG_LIST_SIZE && !index_found);
    
    index--;

    //printf("\nAsize: %ld Index Found: %d\n", asize, index);

    return index;
}

//This is to remove free space
void segLists_remove(struct freeblock * ptr){

    size_t ptr_size = (size_t)HDRP(ptr);
    int i = segIndex(ptr_size);

    //printf("\n*************Remove 1************");
    //printList(segLists[i]);
    //printf("\nRemove at Index: %d\n", i);

    //Make sure list and ptr are not empty
    if(segLists[i] && ptr){
        //testhalt("remove 2");
        //If the pointer is not the only item and is not the head
        if((ptr->next != ptr) && (segLists[i] != ptr)){
            //Block before ptr to be removed
            ptr->prev->next = ptr->next;
            //Block after pointer to be removed
            ptr->next->prev = ptr->prev;
            //testhalt("remove 3");
        
        //pointer is the head
        } else if ((ptr->next != ptr) && (segLists[i] == ptr)){
            //testhalt("remove 4");
            //Block before ptr to be removed
            ptr->prev->next = ptr->next;
            //Block after pointer to be removed
            ptr->next->prev = ptr->prev;
            //Fix head
            segLists[i] = ptr->next;
            //testhalt("remove 5");
        
        //ptr is only item
        } else {
            //Set list to default value
            segLists[i] = NULL;
        }
        
        //printList(segLists[i]);
        //printf("*************Remove 6************");
    }
}

//This is given free space
void segLists_insert(struct freeblock * ptr){

    //printf("\n*************Insert 1************");
    //Make sure the ptr is not null
    if(ptr){
        ////testhalt("Insert 1", true);
        size_t ptr_size = (size_t)HDRP(ptr);
        int i = segIndex(ptr_size);

        //printf("\nInsert at Index: %d\n", i);
        //printf("\n i, %d", i);

        //The head of the segmented list at index is not null
        if(segLists[i]){
            //printf("\nInsert 2");
            
            //New pointer
            ptr->next = segLists[i];
            ptr->prev = segLists[i]->prev;
            //testhalt("Insert 2b");
            //Block at end of list
            segLists[i]->prev->next = ptr;
            //testhalt("Insert 2c");
            //Head
            segLists[i]->prev = ptr;
            
        //The head of the segmented list at index is null (empty)
        } else {
            //printf("\nInsert 3");
            segLists[i] = ptr;
            segLists[i]->prev = segLists[i];
            segLists[i]->next = segLists[i];
        }
        //printList(segLists[i]);
    }
    //printf("\n*************Insert 4************");
}

/**********************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 **********************************************************/
 int mm_init(void)
 {
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);                         // alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));   // prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));   // prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));    // epilogue header
    heap_listp += DSIZE;

    //Init the segmented lists
    ////testhalt("Init", true);
    int i;
    for (i=0;i<SEG_LIST_SIZE;i++){
        segLists[i] = NULL;
    }

    return 0;
 }

/**********************************************************
 * coalesce
 * Covers the 4 cases discussed in the text:
 * - both neighbours are allocated
 * - the next block is available for coalescing
 * - the previous block is available for coalescing
 * - both neighbours are available for coalescing
 **********************************************************/
void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {       /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        segLists_remove((freeblock*)NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return (bp);
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        segLists_remove((freeblock*)PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        return (PREV_BLKP(bp));
    }

    else {            /* Case 4 */
        segLists_remove((freeblock*)PREV_BLKP(bp));
        segLists_remove((freeblock*)NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))  +
            GET_SIZE(FTRP(NEXT_BLKP(bp)))  ;
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        return (PREV_BLKP(bp));
    }
}

/**********************************************************
 * extend_heap
 * Extend the heap by "words" words, maintaining alignment
 * requirements of course. Free the former epilogue block
 * and reallocate its new header
 **********************************************************/
void *extend_heap(size_t words)
{

    //printf("\nEXTEND\n");
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignments */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ( (bp = mem_sbrk(size)) == (void *)-1 )
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));                // free block header
    PUT(FTRP(bp), PACK(size, 0));                // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));        // new epilogue header

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}


/**********************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned
 **********************************************************/
void * find_fit(size_t asize)
{
    int i = segIndex(asize);
    //Iterate through each list to find a large enough block
    for(;(i < SEG_LIST_SIZE); i++){

        struct freeblock * bp = segLists[i];
        //printf("\nFind Fit \nIndex: %d Asize: %ld", i, asize);
        //testhalt("F1");
        //printList(bp);
        if(!bp)continue;
        do{
            //Find size of current block
            int totalBlockSize = GET_SIZE(HDRP(bp));
            //printf("\nF1..");
            //See if block can fit asize
            if(asize <= totalBlockSize){

                //printf("F2..");
                int unusedSize = totalBlockSize - asize;

                //If the leftover space can fit a new block
                if(BLOCK_SIZE <= unusedSize){
                    //printf("F3..");
                    segLists_remove(bp);

                    void * newbp = (void *)bp + asize;
                    PUT(HDRP(bp), PACK(asize,0));
                    PUT(FTRP(bp), PACK(asize,0));

                    PUT(HDRP(newbp), PACK(unusedSize,0));
                    PUT(FTRP(newbp), PACK(unusedSize,0));

                    segLists_insert(newbp);                     
                
                //The leftover space cannot fit a new block
                } else {
                    //printf("F4..");
                    segLists_remove(bp);
                }

                //printf("F1A..");
                return (void*) bp;
            }
            bp = bp->next;
        }while(bp != segLists[i]);
        
    }
    //printf("F2B..");
    return NULL;
}

/**********************************************************
 * place
 * Mark the block as allocated
 **********************************************************/
void place(void* bp, size_t asize)
{
  /* Get the current block size */
  size_t bsize = GET_SIZE(HDRP(bp));

  PUT(HDRP(bp), PACK(bsize, 1));
  PUT(FTRP(bp), PACK(bsize, 1));
}

/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *bp)
{

    //printf("\nFREE\n");
    if(bp == NULL){
      return;
    }
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));

    segLists_insert((struct freeblock*)coalesce(bp));
}


/**********************************************************
 * mm_malloc
 * Allocate a block of size bytes.
 * The type of search is determined by find_fit
 * The decision of splitting the block, or not is determined
 *   in place(..)
 * If no block satisfies the request, the heap is extended
 **********************************************************/
void *mm_malloc(size_t size)
{
    size_t asize; /* adjusted block size */
    //size_t extendsize; /* amount to extend heap if no fit */
    char * bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/ DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    //extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(asize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;

}

/**********************************************************
 * mm_realloc
 * Implemented simply in terms of mm_malloc and mm_free
 *********************************************************/
void *mm_realloc(void *ptr, size_t size)
{
    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0){
      mm_free(ptr);
      return NULL;
    }
    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL)
      return (mm_malloc(size));

    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    /* Copy the old data. */
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/**********************************************************
 * mm_check
 * Check the consistency of the memory heap
 * Return nonzero if the heap is consistant.
 *********************************************************/
int mm_check(void){
  return 1;
}
