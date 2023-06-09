//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/* 
 * init_pte - Initialize PTE entry
 */
int init_pte(uint32_t *pte,
             int pre,    // present
             int fpn,    // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             int swpoff) //swap offset
{
  if (pre != 0) {
    if (swp == 0) { // Non swap ~ page online
      if (fpn == 0) 
        return -1; // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT); 
    } else { // page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT); 
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;   
}

/* 
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(uint32_t *pte, int swptyp, int swpoff)
{
  CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
  //printf("In pte_set_swap: swptyp: %d, swpoff: %d, pte: %08x \n", swptyp, swpoff, *pte);
  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  //printf("In pte_set_swap: After set swptyp: pte: %08x \n", *pte);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
  //printf("In pte_set_swap: After set swpoff: pte: %08x \n", *pte);

  return 0;
}

/* 
 * pte_set_swap - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(uint32_t *pte, int fpn)
{
  //printf("Setfpn called for pte %08x\n", *pte);
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
  //printf("Setfpn's pte after set bit %08x\n", *pte);
  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT); 
  //printf("Setfpn's pte final %08x\n", *pte);

  return 0;
}


/* 
 * vmap_page_range - map a range of page at aligned address
 */
int vmap_page_range(struct pcb_t *caller, // process call
                                int addr, // start address which is aligned to pagesz
                               int pgnum, // num of mapping page
           struct framephy_struct *frames,// list of the mapped frames (free frames)
              struct vm_rg_struct *ret_rg)// return mapped region, the real mapped fp
{                                         // no guarantee all given pages are mapped
  //uint32_t * pte = malloc(sizeof(uint32_t));
  //printf("Begin vmap_page_range.\n");
  struct framephy_struct *fpit = malloc(sizeof(struct framephy_struct));
  //int  fpn;
  int pgit = 0;
  // int pgn = PAGING_PGN(addr); // DON'T CARE ABOUT THIS FUNCTION 
                              // --> KNOWS THAT IT CREATES PAGE NUMBER OUT OF ADDRESS GIVEN
  

  ret_rg->rg_end = ret_rg->rg_start = addr; // at least the very first space is usable

  fpit->fp_next = frames;

  /* TODO map range of frame to address space 
   *      [addr to addr + pgnum*PAGING_PAGESZ
   *      in page table caller->mm->pgd[]
   */
  //printf("Number of mapping page of vm_page_range: %i\n", pgnum);
  //printf("Current vm_end of vm_page_range: %i\n", addr);
  for(; pgit < pgnum; pgit++)
  {
    int pageAddr = addr + pgit * PAGING_PAGESZ;
    int pageNum = PAGING_PGN(pageAddr);
    //printf("Page number of vm_page_range: %i\n", pageNum);
    fpit = fpit->fp_next;
    if(fpit == NULL)
    //printf("frames numb: %d - vmap_page_range: ", fpn);
    {
      printf("fpit in mm.c is NULL \n");
      return -1;
    }
    int fpn = fpit->fpn;
    //printf("Frame page number: %i\n", fpn);
    // set frame page number bits (from 0 to 12 --> 13 bits)

    //caller->mm->pgd[pageNum] = ((caller->mm->pgd[pageNum]) & 0xffffe000) | (fpn & 0x1fff);
    pte_set_fpn(&(caller->mm->pgd[pageNum]), fpn); //SETVAL(caller->mm->pgd[pageNum],fpn,PAGING_PTE_FPN_MASK,0);
    //caller->mm->pgd[pageNum] = PAGING_PTE_SET_PRESENT(caller->mm->pgd[pageNum]); // set present bit to 1
    //caller->mm->pgd[pageNum] = PAGING_PTE_UNSET_SWAPPED(caller->mm->pgd[pageNum]); // set swapped bit to 0
    enlist_pgn_node(&caller->mm->fifo_pgn, pageNum);
  }
  ret_rg->rg_end = addr + pgnum * PAGING_PAGESZ;
  //pte_set_fpn(caller->mm->pgd[pageNum], fpn);
  //printf("page addr: %d - vmap_page_range \n", pageAddr);
  //printf("frame test: %d \n", GETVAL(caller->mm->pgd[pageNum], PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT));
  //enlist_pgn_node(&caller->mm->fifo_pgn, pageNum);
  //printf("BP: ret_rg->rg_end = %ld - vmap_page_range\n", ret_rg->rg_end);
  //printf("BP: ret_rg->rg_start = %ld - vmap_page_range\n", caller->mm->symrgtbl[rgid].rg_start);
  //printf("BP: ret_rg->rg_end = %ld - vmap_page_range\n", caller->mm->symrgtbl[rgid].rg_end);
  //caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
   /* Tracking for later page replacement activities (if needed)
    * Enqueue new usage page */
   //enlist_pgn_node(&caller->mm->fifo_pgn, pgn+pgit);
  //printf("End vmap_page_range.\n");
  return 0;
}

/* 
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */

int alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct** frm_lst)
{
  //printf("Begin alloc_pages_range.\n");
  int pgit, fpn;
  //struct framephy_struct *newfp_str;
  // initially: struct framephy_struct* frm_lst = NULL;

  for(pgit = 0; pgit < req_pgnum; pgit++)
  {
    struct framephy_struct* temp = malloc(sizeof(struct framephy_struct));
    if(MEMPHY_get_freefp(caller->mram, &fpn) == 0)
    {
      // If there is free frame available
      // TODO: create new frame --> append to frm_lst --> add fpn
      temp->fpn = fpn;
    } 
    else 
    {  // ERROR CODE of obtaining somes but not enough frames
      //printf("End alloc_pages_range with return = -3000.\n\n");

      // Page replacement for alloc if RAM is out of free frames
      // WIP
      
      int vicpgn = -1, swpfpn = -1;
      find_victim_page(caller->mm, &vicpgn);
      //printf("Victim page number is: %i\n", vicpgn);
      MEMPHY_get_freefp(caller->active_mswp, &swpfpn);
      if(swpfpn != -1  && vicpgn != -1) /* A frame in swap is available */
      {
        //printf("victim page number found: %i\n", vicpgn);
        //printf("swpfpn found: %d\n", swpfpn);
        uint32_t pte_vicpgn = caller->mm->pgd[vicpgn];
        //printf("Page %i: ", vicpgn);
        //printf("pte for victim page: %08x\n", pte_vicpgn);
        //int vicfpn = PAGING_FPN(pte_vicpgn); // WRONG!!
        int vicfpn = GETVAL(pte_vicpgn,PAGING_PTE_FPN_MASK,PAGING_PTE_FPN_LOBIT);
        //printf("Victim frame number is: %i\n", vicpgn);
        __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
        // change to 5-25 form
        pte_set_swap(&(caller->mm->pgd[vicpgn]), 0, swpfpn);
        temp->fpn = vicfpn;
        //printf("Victim Frame Page Number: %i\n", vicfpn);
      }
      else return -3000; //if out of memory (frames)
    }

    /* Enlist to free frame list */
    if(*frm_lst == NULL){
      *frm_lst = temp;
    } else {
      temp->fp_next = *frm_lst;
      *frm_lst = temp;
    }
  }
  //printf("End alloc_pages_range.\n");
  //print_list_fp(*frm_lst);
  return 0;
}


/* 
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start ==> vm_region start
 * @aend      : vm area end ==> vm_region end
 * @mapstart  : start mapping point ==> vm_area end
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
int vm_map_ram(struct pcb_t *caller, int astart, int aend, int mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  //printf("Begin vm_map_ram.\n");
  struct framephy_struct *frm_lst = NULL;
  int ret_alloc;
  //printf("BP: astart = %i\n", astart);
  //printf("BP: aend = %i\n", aend);

  /*@bksysnet: author provides a feasible solution of getting frames
   *FATAL logic in here, wrong behaviour if we have not enough page
   *i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
   *Don't try to perform that case in this simple work, it will result
   *in endless procedure of swap-off to get frame and we have not provide 
   *duplicate control mechanism, keep it simple
   */

  ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst); 
  // return a list of free frames to frm_lst
  // ret_alloc: give status result;
  if (ret_alloc < 0 && ret_alloc != -3000)
    return -1;

  /* Out of memory */
  if (ret_alloc == -3000) 
  {
    printf("OOM: vm_map_ram out of memory \n");
    return -1;
  }

  /* it leaves the case of memory is enough but half in ram, half in swap
   * do the swaping all to swapper to get the all in ram */
  int flag = vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);
  if(flag == -1) return -1; // fpit is NULL

  //printf("End vm_map_ram.\n");
  return 0;
}

/* Swap copy content page from source frame to destination frame 
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
int __swap_cp_page(struct memphy_struct *mpsrc, int srcfpn,
                struct memphy_struct *mpdst, int dstfpn) 
{
    //printf("Swap activated \n");
  int cellidx;
  int addrsrc,addrdst;
  for(cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
  {
    addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING_PAGESZ + cellidx;

    BYTE data;
    //printf("addrsrc: %d, addrdst: %d \n", addrsrc, addrdst);
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
  }
  return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct * vma = malloc(sizeof(struct vm_area_struct));

  mm->pgd = malloc(PAGING_MAX_PGN*sizeof(uint32_t));

  /* By default the owner comes with at least one vma */
  vma->vm_id = 1;
  vma->vm_start = 0;
  vma->vm_end = vma->vm_start;
  vma->sbrk = vma->vm_start;
  struct vm_rg_struct *first_rg = init_vm_rg(vma->vm_start, vma->vm_end);
  enlist_vm_rg_node(&vma->vm_freerg_list, first_rg);

  vma->vm_next = NULL;
  vma->vm_mm = mm; /*point back to vma owner */

  mm->mmap = vma;

  // Init symbol table range
  for(int i = 0; i <= PAGING_MAX_SYMTBL_SZ; i++)
  {
    caller->mm->symrgtbl[i].rg_start = 0;
    caller->mm->symrgtbl[i].rg_end = 0;
  }
   
  return 0;
}

struct vm_rg_struct* init_vm_rg(int rg_start, int rg_end)
{
  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

  rgnode->rg_start = rg_start;
  rgnode->rg_end = rg_end;
  rgnode->rg_next = NULL;

  return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct* rgnode)
{
  rgnode->rg_next = *rglist;
  *rglist = rgnode;

  return 0;
}

int enlist_pgn_node(struct pgn_t **plist, int pgn)
{
  //printf("Begin enlist_pgn_node.\n");
  //printf("Added page: %d \n", pgn);
  struct pgn_t* pnode = malloc(sizeof(struct pgn_t));

  pnode->pgn = pgn;
  pnode->pg_next = *plist;
  *plist = pnode;

  //printf("End enlist_pgn_node.\n");
  return 0;
}

int print_list_fp(struct framephy_struct *ifp)
{
   struct framephy_struct *fp = ifp;
 
   printf("print_list_fp: ");
   if (fp == NULL) {printf("NULL list\n"); return -1;}
   printf("\n");
   while (fp != NULL )
   {
       printf("fp[%d]\n",fp->fpn);
       fp = fp->fp_next;
   }
   printf("\n");
   return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
   struct vm_rg_struct *rg = irg;
 
   printf("print_list_rg: ");
   if (rg == NULL) {printf("NULL list\n"); return -1;}
   printf("\n");
   while (rg != NULL)
   {
       printf("rg[%ld->%ld]\n",rg->rg_start, rg->rg_end);
       rg = rg->rg_next;
   }
   printf("\n");
   return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
   struct vm_area_struct *vma = ivma;
 
   printf("print_list_vma: ");
   if (vma == NULL) {printf("NULL list\n"); return -1;}
   printf("\n");
   while (vma != NULL )
   {
       printf("va[%ld->%ld]\n",vma->vm_start, vma->vm_end);
       vma = vma->vm_next;
   }
   printf("\n");
   return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
   printf("print_list_pgn: ");
   if (ip == NULL) {printf("NULL list\n"); return -1;}
   printf("\n");
   while (ip != NULL )
   {
       printf("va[%d]-\n",ip->pgn);
       ip = ip->pg_next;
   }
   printf("n");
   return 0;
}

int print_pgtbl(struct pcb_t *caller, uint32_t start, uint32_t end)
{
  int pgn_start,pgn_end;
  int pgit;

  if(end == -1){
    pgn_start = 0;
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, 0);
    end = cur_vma->vm_end;
  }
  pgn_start = PAGING_PGN(start);
  pgn_end = PAGING_PGN(end);

  printf("print_pgtbl: %d - %d", start, end);
  if (caller == NULL) {printf("NULL caller\n"); return -1;}
    printf("\n");


  for(pgit = pgn_start; pgit < pgn_end; pgit++)
  {
    printf("%08ld: %08x\n", pgit * sizeof(uint32_t), caller->mm->pgd[pgit]);
    //printf("PTE %i: %08x\n", pgit, caller->mm->pgd[pgit]);
  }

  return 0;
}

//#endif