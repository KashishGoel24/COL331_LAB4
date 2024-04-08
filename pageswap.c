#include "types.h"
#include "stat.h"
#include "param.h"
#include "mmu.h"
#include "memlayout.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"

void editPTEentry(pte_t* pte, int diskblock){

    uint flags = PTE_FLAGS(pte); // Get flags from existing PTE
    uint new_entry = ((uint)diskblock << 12) | (flags & ~PTE_P) | PTE_SWAP;   // Use bitwise OR operator to set PTE_P and unset PTE_SWAP
    *pte = (pte_t)new_entry;

    return;
}

void swapOut(void){
    struct proc *p;
    p = findVictimProcess();
    p->rss -= PGSIZE;
    pte_t *pte;
    pte = findVictimPage(p);
    // cprintf("the pte entry of the victim page is %d \n",(int)*pte);
    char *addPage = (char*)(P2V(PTE_ADDR(*pte)));
    int vacantSlot = findVacantSwapSlot();
    int diskBlock = diskBlockNumber(vacantSlot);

    // cprintf("the disk id to which the page is being written in swap out is %d \n",diskBlock);

    writeToDisk(ROOTDEV, addPage, diskBlock);

    updateSwapSlot(vacantSlot, 0, (int)PTE_FLAGS(*pte));

    // swap_block.slots[vacantSlot].is_free = 0;
    // swap_block.slots[vacantSlot].page_perm = (int)PTE_FLAGS(pte);

    // cprintf("Swapping Out: %x\n", pte);

    kfree(addPage);
    editPTEentry(pte, diskBlock);

    // cprintf("printing the pte entry after the edit pte entry fucntion in swap out is %d \n",(int)*pte);
    
    lcr3(V2P(p->pgdir));
}



void pgfault_handler(void){
    // Get the address of the page
    uint va;
    va = PGROUNDDOWN(rcr2());

    // cprintf("the value of the rcr 2 register is : ");
    // printint((int)rcr2(),10,0);
    // cprintf("\n");
    
    // iterate over the page directory and extract the page table enries
    pte_t *pte = walkpgdir(myproc()->pgdir, (void*)va, 0);
    uint diskBlock = (uint)(*pte >> 12);

    // cprintf("Swapping In: %x\n", pte);

    // cprintf("printing the pte entry received from the walkpgdir in pagefault_handler %d \n",(int)*pte);
    // cprintf("printing the disk block corresp to swapped out entry %d \n",(int)diskBlock);

    // int flags = PTE_FLAGS(pte);
    
    //call kalloc to get a page
    char *pg;
    // cprintf("before calling kalloc in swap in \n");
    pg = kalloc();
    // cprintf("after calling kalloc in swap in \n");

    myproc()->rss += PGSIZE;

    readFromDiskWriteToMem(ROOTDEV, pg, diskBlock);
    
    //update pte
    // int prev_perm = getPerm(((diskBlock-2)/8));
    // *pte = V2P(pg) |PTE_U|PTE_W|PTE_A;
    *pte =  V2P(pg) | getPerm(((diskBlock-2)/8));
    *pte |= PTE_P; 

    updateSwapSlot(((diskBlock-2)/8), 1, PTE_FLAGS(*pte));

    // cprintf(" printing the pte entry after the updating in the swap in function %d \n",(int)*pte);

    // pte = walkpgdir(myproc()->pgdir, (void*)va, 0);
    // printint((int)*pte,10,0);
    // cprintf("\n printing the pte entry received \n");
    // lcr3(V2P(myproc()->pgdir));
}