/*

Name: Bijay Raj Raut
ID:   1001562222

*/


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)


static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev;  /* Pointer to the previous _block of allcated memory   */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
};
//save the size of structure for ease of use
size_t sizeofblock = sizeof(struct _block);

struct _block *freeList = NULL; /* Free list to track the _blocks available */
struct _block *lastused = NULL; /* Track last used node for next fit usage*/

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size)
{
   struct _block *curr = freeList;

#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0

    //start looking from the begining that is going to fit the required size
    struct _block *temp;
    while (curr && !(curr->free && curr->size >= size))
    {
        *last = curr;
        curr  = curr->next;
    }
    //save the block to temp block and then traverse through rest of the
    //list and swap the values if a better block is found rather than
    //what was chosen
    temp = *last;
    if(temp!=NULL)
    {
        while(temp!=NULL)
        {
            if(temp->free && temp->size >= size && temp->size < curr->size)
            {
                *last = temp->prev; //make *last prev value of the matching block
                curr  = temp; //update the current block
            }
            temp = temp->next;
        }
    }

#endif

#if defined WORST && WORST == 0
   struct _block *temp;
   while (curr && !(curr->free && curr->size >= size))
   {
       *last = curr;
       curr  = curr->next;
   }
    //save the block to temp block and then traverse through rest of the
    //list and swap the values if a larger block is found rather than
    //what was chosen at the begining
   temp = *last;
   if(temp!=NULL)
   {
       while(temp!=NULL)
       {
           if(temp->free && temp->size >= size && temp->size > curr->size)
           {
               *last = temp->prev; //make *last prev value of the matching block
               curr  = temp; //update the current block
           }
           temp = temp->next;
       }
   }
#endif

#if defined NEXT && NEXT == 0
    //Condition when we dont have anything as last used : for the first request
    //do the first fit and update the lastused blocks address
    //and for the remaining request do the first fit starting from lastused block
    //if hit the null starting from last used state restart the process
restart:
    if(lastused==NULL)
    {
        while (curr && !(curr->free && curr->size >= size))
        {
            *last = curr;
            lastused = curr;
            curr  = curr->next;
        }
    }
    //if inedded their is a last used block then start from that block
    else if(lastused!=NULL)
    {
        curr = lastused;
        while(curr && !(curr->free && curr->size >= size))
        {
            *last = curr;
            curr  = curr->next;
        }
        lastused = curr;
        if(curr==NULL)
        {
            curr = freeList;
            goto restart;
        }
    }

#endif
    if(curr!=NULL)
        num_reuses++; //if free block found increment the reuses number
    return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size)
{
    /* Request more space from OS */
    struct _block *curr = (struct _block *)sbrk(0);
    struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

    assert(curr == prev);

    /* OS allocation failed */
    if (curr == (struct _block *)-1)
    {
        return NULL;
    }
    /* Update freeList if not set */
    if (freeList == NULL)
    {
        freeList = curr;
    }
    /* Attach new _block to prev _block */
    //upto this point growheap function has created a block
    //and grown the size of it as well so increment them
    num_grows++;
    num_blocks++;
    if (last)
    {
        last->next = curr;
    }

    /* Update _block metadata */
    curr->size = size;
    curr->next = NULL;
    curr->free = false;
    max_heap = max_heap + size + sizeofblock; //increase heapsize every time growheap gets called
    //printf("\n\nRequested Size = %zu\n Size of Block = %zu\nMax_heap = %d\n\n",size,sizeofblock,max_heap);
    return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */

void *malloc(size_t size)
{
    num_requested+=size; //keep all the requested sizes
    num_mallocs++; //increment each time malloc gets called
    if( atexit_registered == 0 )
    {
       atexit_registered = 1;
       atexit( printStatistics );
    }

    /* Align to multiple of 4 */
    size = ALIGN4(size);

    /* Handle 0 size */
    if (size == 0)
    {
       return NULL;
    }

    /* Look for free _block */
    struct _block *last = freeList;
    struct _block *next = findFreeBlock(&last, size);

    //if free block is found which is greator than the overall size of
    //the block, we will trim it
    /*if(next!=NULL&&(next->size)>=(size+sizeofblock))
        num_++;
     */
    //splitting only when the size we get is bigger than requested size
    if(next!=NULL&&(next->size)>(size+sizeofblock))
    {
        size_t previous_size = next->size; //save the previous size of the next
        //track the offset to jump the pointer to new address
        size_t offset = (size + sizeofblock);
        //create new block and provide it with the address of new
        struct _block *prev_next = next;
        num_blocks++; //increment on new block creation
        //add offset to start previous _block address to point at new address
        prev_next = (struct _block*) ((void *)next + offset);
        //Retrieve info on size of deducting requested size and size of block
        //to its previous size
        prev_next->size = previous_size - size - sizeofblock;
        //set the new block free for later usage
        prev_next->free = true;
        //make new block point at next's next pointer
        prev_next->next = next->next;
        //make next block to point at new block's pointer
        next->next = prev_next;
        //update the size of next
        next->size = size;
        num_splits++;//increment on each successful split

         /*printf("Old Size = %zd\nBlock Requested = %zu\nBlock Size = %d\n New size = %zu\nNew block size = %zu",previous_size,size,sizeofblock,next->size,prev_next->size);*/

    }
    //printf("%Size of next = zu",next->size);
    /* Could not find free _block, so grow heap */
    if (next == NULL)
    {
        //printf("\nRequested Size: %zu **BEFORE**\n",size);
        next = growHeap(last, size);
    }
    //linking the last as the previous of the next block
    if( next )
    {
        //printf("Setitng prev to %p\n", last);
        next->prev = last;
    }
    /* Could not find free _block or grow heap, so just return NULL */
    if (next == NULL)
    {
        return NULL;
    }

    /* Mark _block as in use */
    next->free = false;

    /* Return data address associated with _block */
    return BLOCK_DATA(next);
}
/*From man page:
 void *calloc(size_t nmemb,size_t size);
 calloc() allocates memory for an array of nmemb elements of size bytes each and returns a pointer to the allocated memory. The memory is set to zero.
 */
void *calloc(size_t nmemb,size_t size)
{
    /*
    void *address = malloc(nmemb*size);
    if(address!=NULL)
        memset(address,0,nmemb*size);
    return address;
    */
    //calling malloc by multiplying the number of blocks with the size
    void *address = malloc(nmemb*size);
    //if malloc was able to return the address to the memory
    //zeroing out memory and returning it else NULL would be returned.
    if(address!=NULL)
    {
        //zeroing out the memory
        memset(address,0,nmemb*size);
        return address;
    }
    //return NULL if mallocs return NULL
    return NULL;

}
/*
 From man page
 void *realloc(void *ptr,size_t size);
 Reallocate to the new size or return null
 */
void *realloc(void *ptr,size_t size)
{
    //allocating the new address
    void *address = malloc(size);
    if(address!=NULL)
    {
        //copy the previous info and return the new address
        memcpy(address,ptr,size);
        //free(ptr);
        return address;
    }
    //return NULL if mallocs return NULL
    return NULL;
}
/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    num_frees++; //increment on every sucessful call of free
    /* Make _block as free */
    struct _block *curr = BLOCK_HEADER(ptr);
    assert(curr->free == 0);
    curr->free = true;
    //printf("Checking %p\n", curr->prev);
    //TODO: Coalesce free _blocks if needed

    //Next Coalesce
    //if next block is free save the value of the next's next and update curr's
    //size by adding next's size and size of block to its own size then point curr's
    //next to next's next
    if(curr->next!=NULL)
    {
        if((curr->next)->free)
        {
            //printf("Next coal\n");
            size_t next_size = (curr->next)->size;
            struct _block *next_next = (curr->next)->next;
            curr->next = next_next;
            curr->size = curr->size + sizeofblock + next_size;
            num_coalesces++;

        }
    }
    //Previous Coalesce
    //if the previous block is free then save the address of previous pointer then
    // move the pointer to the top of the previous block, update its size by
    //adding the curr's size and size of the block and the previous address it had
    // after that point to the next block that curr's pointing from prev block
    else if((curr->prev)!=NULL)
    {
        //printf("%p %p\n", curr, curr->prev);
        if((curr->prev)->free)
        {
            //printf("Previous coal\n");
            struct _block *temp2 = curr->prev;
            struct _block *prev_next = curr->next;
            size_t previous_size = curr->size;
            temp2->size = temp2->size + previous_size + sizeofblock;
            temp2->next = prev_next;
            //curr = temp2;
            num_coalesces++;
        }
    }

}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
