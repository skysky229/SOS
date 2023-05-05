//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/*enlist_vm_freerg_list - add new rg to freerg_list AT HEAD
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct* rg_elmt)
{
  //printf("Begin enlist_vm_freerg_list.\n");
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;

  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node;

  /* Enlist the new region */
  mm->mmap->vm_freerg_list = rg_elmt;

  //printf("End enlist_vm_freerg_list.\n");
  return 0;
}

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma= mm->mmap;

  if(mm->mmap == NULL)
    return NULL;

  int vmait = 0;
  
  while (vmait < vmaid) // HOW TO GET OUT OF WHILE LOOP?
  {
    if(pvma == NULL)
	  return NULL;

    pvma = pvma->vm_next;
  }

  return pvma;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
  /*Allocate at the toproof */
  //printf("Begin __alloc.\n");
  struct vm_rg_struct rgnode;

  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;

    *alloc_addr = rgnode.rg_start;

    return 0;
  }

  /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/

  /*Attempt to increate limit to get space */
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  int inc_sz = PAGING_PAGE_ALIGNSZ(size); // size of num of increase pages * page size 
                                          // --> internal frag
                                          // int size: actual size need to be allocated
  //int inc_limit_ret
  int old_sbrk ;

  old_sbrk = cur_vma->sbrk;
  //printf("Current sbrk position: %d\n", old_sbrk);
  //printf("Current vm_end position: %ld\n", cur_vma->vm_end);

  /* TODO INCREASE THE LIMIT
   * inc_vma_limit(caller, vmaid, inc_sz)
   */
  inc_vma_limit(caller, vmaid, inc_sz);

  /*Successful increase limit */
  //printf("BP: Region ID: %i - __alloc", rgid);
  caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;

  *alloc_addr = old_sbrk;
  //struct vm_rg_struct *ret_rg = NULL;
  //vmap_page_range(caller, old_sbrk, inc_sz / PAGING_PAGESZ, idk, ret_rg);

  //printf("End __alloc.\n");
  printf("Alloc success, PID: %d \n", caller->pid);
  print_pgtbl(caller, 0, -1);
  return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
  //printf("Begin __free.\n");
  //print_list_rg(caller->mm->mmap->vm_freerg_list);
  struct vm_rg_struct* rgnode = malloc(sizeof(struct vm_rg_struct));

  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return -1;

  /* TODO: Manage the collect freed region to freerg_list */
  //rgnode = *(get_symrg_byid(caller->mm, rgid));
  rgnode->rg_start = caller->mm->symrgtbl[rgid].rg_start;
  rgnode->rg_end = caller->mm->symrgtbl[rgid].rg_end;
  rgnode->rg_next = NULL;
  
  /*enlist the obsoleted memory region */
  //printf("Freed region id: %d\n", rgid);
  enlist_vm_freerg_list(caller->mm, rgnode);
  //print_list_rg(caller->mm->mmap->vm_freerg_list);
  //printf("End __free.\n");
  printf("Free success, PID: %d \n", caller->pid);
  return 0;
}

/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;

  /* By default using vmaid = 0 */
  /*#ifdef IODUMP
  printf("alloc size=%d register index=%d PID=%d \n", size, reg_index, proc->pid);
  #ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
  #endif
  MEMPHY_dump(proc->mram);
  #endif*/
  return __alloc(proc, 0, reg_index, size, &addr);
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int pgfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  #ifdef IODUMP
  printf("register index=%d PID: %d\n", reg_index, proc->pid);
  #ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
  #endif
  MEMPHY_dump(proc->mram);
  #endif
   return __free(proc, 0, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  //printf("Begin pg_getpage\n");
  uint32_t pte = mm->pgd[pgn]; /* Get the page table entry corresponding to the pgn page from the page table*/

  if (!PAGING_PAGE_PRESENT(pte)) // page is not in RAM (presented bit = 0)
  { /* Page is not online, make it actively living */
    int vicpgn = -1, swpfpn = -1; 

    //int vicfpn;
    //uint32_t vicpte;

    int tgtfpn = PAGING_SWP(pte);//the target frame storing our variable

    /* TODO: Play with your paging theory here */
    /* Find victim page */
    find_victim_page(caller->mm, &vicpgn);

    /* Get free frame in MEMSWP */
    MEMPHY_get_freefp(caller->active_mswp, &swpfpn);

    // CHANGE
    if(vicpgn == -1 || swpfpn == -1) return -1;
    uint32_t pte_vicpgn = caller->mm->pgd[vicpgn];
    printf("victim pgn: %i\n", vicpgn);
    //int vicfpn = PAGING_FPN(pte_vicpgn);
    int vicfpn = GETVAL(pte_vicpgn,PAGING_PTE_FPN_MASK,PAGING_PTE_FPN_LOBIT);
    /* Do swap frame from MEMRAM to MEMSWP and vice versa*/
    /* Copy victim frame to swap */
    __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
    /* Copy target frame from swap to mem */
    __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);

    printf("swap fpn: %i\n", swpfpn);
    printf("target fpn: %i\n", tgtfpn);
    printf("victim fpn: %i\n", vicfpn);
    /* Update page table */
    pte_set_swap(&mm->pgd[vicpgn], 0, swpfpn); /* Set the pte of victim page to swpfpn (which means it is stored in frame swpfpn) */

    /* Update its online status of the target page */
    //pte_set_fpn() &mm->pgd[pgn];
    pte_set_fpn(&mm->pgd[pgn], vicfpn);

    enlist_pgn_node(&caller->mm->fifo_pgn,pgn);
  }

  *fpn = PAGING_FPN(pte);
  //printf("End pg_getpage\n");
  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acesss = currg->rg_start + offset (source + offset)
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off; /* Shift left by a number of bits equal to size of a frame (for offset)*/

  MEMPHY_read(caller->mram, phyaddr, data);
  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess 
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_write(caller->mram,phyaddr, value);
  return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 * read [source] [offset] [destination]: read from source and give to destination
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */
	  return -1;

  pg_getval(caller->mm, currg->rg_start + offset, data, caller);
  printf("Read success, PID: %d \n", caller->pid);
  print_pgtbl(caller, 0, -1);
  return 0;
}


/*pgwrite - PAGING-based read a region memory */
int pgread(
		struct pcb_t * proc, // Process executing the instruction
		uint32_t source, // Index of source register
		uint32_t offset, // Source address = [source] + [offset]
		uint32_t destination) 
{
  BYTE data;
  __read(proc, 0, source, offset, &data);

  destination = (uint32_t) data;
/*#ifdef IODUMP
  printf("read region=%d offset=%d value=%d\n", source, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif*/

  return 0;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  
  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */
	  return -1;

  pg_setval(caller->mm, currg->rg_start + offset, value, caller);
  printf("Write success, PID: %d \n", caller->pid);
  print_pgtbl(caller, 0, -1);
  return 0;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(
		struct pcb_t * proc, // Process executing the instruction
		BYTE data, // Data to be wrttien into memory
		uint32_t destination, // Index of destination register
		uint32_t offset)
{
/*#ifdef IODUMP
  printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif*/

  return __write(proc, 0, destination, offset, data);
}


/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  int pagenum, fpn;
  uint32_t pte;


  for(pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte= caller->mm->pgd[pagenum];

    if (!PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else {
      fpn = PAGING_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);    
    }
  }

  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct* get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_rg_struct * newrg;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));
  //printf("BP: cur_vma->sbrk = %ld - get_vm_area_node_at_brk.\n", cur_vma->sbrk);
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + size;

  //printf("BP: newrg->rg_start = %ld - get_vm_area_node_at_brk.\n", newrg->rg_start);
  //printf("BP: newrg->rg_end = %ld - get_vm_area_node_at_brk.\n", newrg->rg_end);
  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  //struct vm_area_struct *vma = caller->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  //printf("Begin validate_overlap_vm_area.\n");
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  if(vmaend > cur_vma->vm_end) 
  {
    //printf("End validate_overlap_vm_area with return = -1.\n");
    return -1;
  }

  //printf("End validate_overlap_vm_area.\n");
  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size 
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
  //printf("Begin inc_vma_limit.\n");
  struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz); // size of increased page(s)
  int incnumpage =  inc_amt / PAGING_PAGESZ; // number of increased page(s)
  
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);

  int old_end = cur_vma->vm_end;

  /*Validate overlap of obtained region */
  // If allocated region surpasses vm_area end limit
  //if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
  //{
  //  printf("End inc_vma_limit with return = -1.\n");
  //  return -1; /*Overlap and failed allocation */
  //}

  /* The obtained vm area (only) 
   * now will be alloc real ram region */
  cur_vma->vm_end += inc_sz;
  cur_vma->sbrk += inc_sz;
  if (vm_map_ram(caller, area->rg_start, area->rg_end, 
                    old_end, incnumpage , newrg) < 0)
    return -1; /* Map the memory to MEMRAM */

  //printf("End inc_vma_limit.\n");
  return 0;

}

/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *retpgn) 
{
  //printf("Begin find_victim_page\n");
  struct pgn_t *pg = mm->fifo_pgn;
  struct pgn_t *prev_pg = NULL;
  if (pg == NULL) return -1; /* There is no pages in fifo */
  /* Apply FIFO for find victim page */
  /* TODO: Implement the theorical mechanism to find the victim page */

  // Only one victim page
  if (pg->pg_next == NULL) {
    *retpgn = pg->pgn;
    mm->fifo_pgn = NULL;
    free(pg);
    //printf("End find_victim_page\n");
    return 0;
  }
  
  while(pg->pg_next != NULL){
    prev_pg = pg;
    pg = pg->pg_next;
    //sleep(2);
    //printf("BP find_victim_page\n");
    //printf("Print page number: %i\n", pg->pgn);
    // WIP: change previous node "next" value to NULL
  }
  /* Update the tail of fifo_pgn list */
  if (prev_pg != NULL)
    prev_pg->pg_next = NULL; 

  /* The returned page */
  *retpgn = pg->pgn;
  //printf("Victim page number is: %i\n", pg->pgn);
  free(pg);

  //printf("End find_victim_page\n");
  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size 
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  //printf("Begin get_free_vmrg_area.\n");
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
  {
    //printf("End get_free_vmrg_area with return = -1.\n");
    return -1;
  }

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL) /* Use first fit */
  {
    if (rgit->rg_start + size <= rgit->rg_end)
    { /* Current region has enough space */
      //printf("BP: region has enough space - get_free_vmrg_area.\n");
      //printf("BP: rgit->rg_start = %ld - get_free_vmrg_area.\n", rgit->rg_start);
      //printf("BP: rgit->end = %ld - get_free_vmrg_area.\n", rgit->rg_end);
      //sleep(1);
      // BUG!!
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      /* Update left space in chosen region */
      if (rgit->rg_start + size < rgit->rg_end)
      {
        rgit->rg_start = rgit->rg_start + size;
      }
      else
      { /*Use up all space, remove current node */
        /*Clone next rg node */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        /*Cloning */
        if (nextrg != NULL)
        {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;

          rgit->rg_next = nextrg->rg_next;

          free(nextrg); // WHY FREE HERE NO MEMORY LEAK????
        }
        else
        { /*End of free list */
          rgit->rg_start = rgit->rg_end;	//dummy, size 0 region
          rgit->rg_next = NULL;
        }
      }
    }
    else
    {
      //printf("BP: region doesn't have enough space - get_free_vmrg_area.\n");
      //printf("BP: rgit->rg_start = %ld - get_free_vmrg_area.\n", rgit->rg_start);
      //printf("BP: rgit->end = %ld - get_free_vmrg_area.\n", rgit->rg_end);
      rgit = rgit->rg_next;	// Traverse next rg
    }
  }

  if(newrg->rg_start == -1) // new region not found
  {
    return -1;
    //printf("End get_free_vmrg_area with return = -1.\n");
  }

  //printf("End get_free_vmrg_area.\n");
  return 0;
}

//#endif