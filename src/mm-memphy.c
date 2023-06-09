//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */

#include "mm.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <semaphore.h>

sem_t mutex, wrt; /* Semaphore lock for locking physical memory (making sure that two processes cannot access phymem at the same time) */
bool init;/* Check if mutex lock is initialized yet */
int reader_count = 0;
pthread_mutex_t free_frame_mutex; /* mutex lock for free frame list */

/*
* mutex_init() - initialize mutex lock if it is not initialized yet
*/
int sem_initial(){
   if (!init) {
      sem_init(&mutex, 0, 1);
      sem_init(&wrt, 0, 1);
      pthread_mutex_init(&free_frame_mutex, NULL);
      init = true;
      return 1;
   } else return 0; /* Not init */
}

/*
 *  MEMPHY_mv_csr - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
int MEMPHY_mv_csr(struct memphy_struct *mp, int offset)
{
   int numstep = 0;

   mp->cursor = 0;
   while(numstep < offset && numstep < mp->maxsz){
     /* Traverse sequentially */
     mp->cursor = (mp->cursor + 1) % mp->maxsz;
     numstep++;
   }

   return 0;
}

/*
 *  MEMPHY_seq_read - read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_seq_read(struct memphy_struct *mp, int addr, BYTE *value)
{
   if (mp == NULL)
     return -1;

   if (!mp->rdmflg)
     return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   *value = (BYTE) mp->storage[addr];

   return 0;
}

/*
 *  MEMPHY_read read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   if (!init) {
      sem_initial();
   }

   /* lock the writer if there is one or more reader */
   sem_wait(&mutex);
   //printf("Lock reader mutex\n");
   reader_count++;
   if (reader_count == 1){
      // printf("Reader count: %d, Lock writer\n", reader_count);
      sem_wait(&wrt);
   }
   //printf("Unlock reader mutex\n");
   sem_post(&mutex);

   if (mp == NULL)
   {
      sem_post(&mutex);
      return -1;
   }

   if (mp->rdmflg)
      *value = mp->storage[addr];
   else /* Sequential access device */
   {
      sem_post(&mutex);
      return MEMPHY_seq_read(mp, addr, value);
   }

   sem_wait(&mutex);
   //printf("Lock reader mutex\n");
   reader_count--;
   //printf("Reader count: %d\n", reader_count);
   if (reader_count == 0){
      sem_post(&wrt);
   }
   //printf("Unlock reader mutex\n");
   sem_post(&mutex);
   return 0;
}

/*
 *  MEMPHY_seq_write - write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_seq_write(struct memphy_struct * mp, int addr, BYTE value)
{

   if (mp == NULL)
     return -1;

   if (!mp->rdmflg)
     return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   mp->storage[addr] = value;

   return 0;
}

/*
 *  MEMPHY_write-write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
   if (!init) {
      sem_initial();
   }
   
   //printf("Lock writer mutex\n");
   sem_wait(&wrt);
   //printf("Mutex lock is locked in MEMPHY_write \n");
   if (mp == NULL){
      //printf("Mutex lock is unlocked in MEMPHY_write \n");
      sem_post(&wrt);
      return -1;
   }

   if (mp->rdmflg)
      mp->storage[addr] = data;
   else /* Sequential access device */
      return MEMPHY_seq_write(mp, addr, data);
   //printf("Mutex lock is unlocked in MEMPHY_write \n");
      //printf("Lock writer mutex\n");
   sem_post(&wrt);
   return 0;
}

/*
 *  MEMPHY_format-format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
   // if (!mutex_initialized) {
   //    mutex_init();
   // }
   
   // pthread_mutex_lock(&mutex);
   // printf("Mutex lock is locked in MEMPHY_format \n");
    /* This setting come with fixed constant PAGESZ */
    int numfp = mp->maxsz / pagesz;
    struct framephy_struct *newfst, *fst;
    int iter = 0;

    if (numfp <= 0)
      return -1;

    /* Init head of free framephy list */ 
    fst = malloc(sizeof(struct framephy_struct));
    fst->fpn = iter;
    mp->free_fp_list = fst;

    /* We have list with first element, fill in the rest num-1 element member*/
    for (iter = 1; iter < numfp ; iter++)
    {
       newfst =  malloc(sizeof(struct framephy_struct));
       newfst->fpn = iter;
       newfst->fp_next = NULL;
       fst->fp_next = newfst;
       fst = newfst;
    }
   // printf("Mutex lock is unlocked in MEMPHY_format \n");
   // pthread_mutex_unlock(&mutex);

    return 0;
}

int MEMPHY_get_freefp(struct memphy_struct *mp, int *retfpn)
{
   if (!init) {
      sem_initial();
   }
   
   pthread_mutex_lock(&free_frame_mutex);
   //printf("Mutex lock is locked in MEMPHY_get_freefp \n");
   struct framephy_struct *fp = mp->free_fp_list; // get ffl

   if (fp == NULL){
      //printf("Mutex lock is unlocked in MEMPHY_get_freefp due to out of free frames \n");
      pthread_mutex_unlock(&free_frame_mutex);
      return -1; // Out of free frames

   }

   *retfpn = fp->fpn; // return frame page number
   mp->free_fp_list = fp->fp_next; // change head of ffl
   //printf("Returned free frame: %d \n", *retfpn);

   /* MEMPHY is iteratively used up until its exhausted
    * No garbage collector acting then it not been released
    */
   free(fp); // free frame from ffl
   //printf("Mutex lock is unlocked in MEMPHY_get_freefp \n");
   pthread_mutex_unlock(&free_frame_mutex);
   return 0;
}

int MEMPHY_dump(struct memphy_struct * mp)
{
    /*TODO dump memphy contnt mp->storage 
     *     for tracing the memory content
     */
   /*for(int i = 0; i < mp->maxsz; i++)
   {
      printf("Byte %i: %hhu\n", i, mp->storage[i]);
   }*/
    return 0;
}

int MEMPHY_put_freefp(struct memphy_struct *mp, int fpn)
{
   if (!init) {
      sem_initial();
   }
   
   pthread_mutex_lock(&free_frame_mutex);
   struct framephy_struct *fp = mp->free_fp_list;
   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));

   /* Create new node with value fpn */
   newnode->fpn = fpn;
   newnode->fp_next = fp;
   mp->free_fp_list = newnode;
   pthread_mutex_unlock(&free_frame_mutex);

   return 0;
}


/*
 *  Init MEMPHY struct
 */
int init_memphy(struct memphy_struct *mp, int max_size, int randomflg)
{
   // if (!mutex_initialized) {
   //    mutex_init();
   // }
   
   // pthread_mutex_lock(&mutex);
   // printf("Mutex lock in init_memphy is locked \n");
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   MEMPHY_format(mp,PAGING_PAGESZ);

   mp->rdmflg = (randomflg != 0)?1:0;

   if (!mp->rdmflg )   /* Not Ramdom acess device, then it serial device*/
      mp->cursor = 0;
   // printf("Mutex lock in init_memphy is unlocked \n");
   // pthread_mutex_unlock(&mutex);

   return 0;
}

//#endif