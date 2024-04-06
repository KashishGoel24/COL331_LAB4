#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h> 

#include "types.h"
#include "fs.h"
#include "stat.h"
#include "param.h"
#include "buf.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"

// Assuming NSLOTS is defined somewhere
// #define NSLOTS 256

// function 1 -> to find the mapping between index of swap block array and disk block number 
// struct swap_slot {
//   // add a lock here and acquire it when you are writing the contents to it
//   // release the lock once you have written the contents and update
//   int page_perm;
//   int is_free;
// };

// struct swap_block {
//     struct swap_slot slots[NSLOTS];
// };

// // Global variable
// struct swap_block *swap_block; // Assuming swap_block is defined somewhere

// int diskBlockNumber(int arrayindex){
//     return 8*arrayindex + 1;    // check if disk blocks are 1 indexed or 0 indexed
// }

// int findVacantSwapSlot(void){
//     for (int i = 0 ; i < NSLOTS ; i++){
//       if (swap_block->slots[i].is_free == 1){
//         return i;
//       }
//     }
//     panic("no vacant swap slot found"); // if no slot found -> for debugging
// }

// void writeToDisk(uint dev, char* pg, int blockno){
//     struct buf *b;
//     for (int i = 0; i < 8 ; i++){
//         b = bget(dev, blockno+i);
//         memmove(b->data,pg+i*512,512);   // check if the index pg+i*512 is correct
//         bwrite(b);
//         brelse(b);
//     }
// }

// void readFromDiskWriteToMem(uint dev, char *pg, uint blockno){
//     struct buf* b;
//     for (int i = 0 ; i < 8 ; i++){
//         b = bread(dev,blockno+i);
//         memmove(pg+i*512,b->data,512);
//         brelse(b);
//     }
// }

// put this function in proc.c
// struct proc* findVictimProcess(void){
//     int maxRss = -1; // Initialize maxRss and pid_cur
//     int pid_cur = -1;
//     struct proc *p = NULL, *p1 = NULL;
//     acquire(&ptable.lock);
//     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
//     {
//         if((p->state == UNUSED))
//         continue;
//         if ((p->rss > maxRss) || (p->rss == maxRss && p->pid < pid_cur)){
//             maxRss = p->rss;
//             pid_cur = p->pid;
//             p1 = p;   // correct this as this will give wrong as they are pointers
//         }
//     }
//     release(&ptable.lock);
//     return p1;
// }

// // write the function to find the victim page given the process and hence its pgdir address
// pte_t* findVictimPage(struct proc *p){
//     pde_t *pgdir = p->pgdir, *pde;
//     pte_t *pgtab, *pte;
//     uint va;
//     // construct a virtual address using the PGADDR macro wherein you set the d t o to 0 initially
//     // make two for loops the outside one iterating over d and the inside one over t 
//     // use this virtual addr and other macros pdx etx to index into the pgdir and check the present bit
//     // if pte_p is there then index into the page table and check each and every page and its access bits
//     // choose the page if pte_p set and pte_a not set
//     // return the ddress of the page table entr corresp to it
//     for (int d = 0 ; d < pow(2,10) ; d++){
//         va = PGADDR((uint)d,0,0);
//         pde = &pgdir[PDX(va)];
//         if (*pde & PTE_P){
//             for (int t = 0 ; t < pow(2,10) ; t++){
//                 va = PGADDR((uint)d,(uint)t,0);
//                 pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
//                 pte = &pgtab[PTX(va)];
//                 if ((*pte & PTE_P) && !(*pte & PTE_A)){
//                     return pte;
//                 }
//             }
//         }
//     }
//     // if we did not find the approproate page then we make 10 percent of the pages as not accessed and then select the victim page accordingly
//     int count = 0, found = 0;
//     pte_t* toReturn = NULL;
//     for (int d = 0 ; d < pow(2,10) ; d++){
//         va = PGADDR((uint)d,0,0);
//         pde = &pgdir[PDX(va)];
//         if (*pde & PTE_P){
//             for (int t = 0 ; t < pow(2,10) ; t++){
//                 va = PGADDR((uint)d,(uint)t,0);
//                 pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
//                 pte = &pgtab[PTX(va)];
//                 if ((*pte & PTE_P) && (*pte & PTE_A) && (count%10 == 0)){
//                     if (found == 0){
//                         found = 1;
//                         toReturn = pte;
//                     }
//                     *pte &= ~PTE_A;
//                 }
//                 count = (count+1)%10;
//             }
//         }
//     }
//     return toReturn;
// }

void editPTEentry(pte_t* pte, int diskblock){
    // need to replace the ppn with the disk block id
    // need to unset the present bit
    // need to set the swap bit
    uint flags = PTE_FLAGS(pte); // Get flags from existing PTE
    uint new_entry = ((uint)diskblock << 12) | PTE_P | (flags & ~PTE_SWAP);   // Use bitwise OR operator to set PTE_P and unset PTE_SWAP
    *pte = (pte_t)new_entry;
}

void swapOut(void){
    // find a victim process
    // find the victim page of teh process using the vicim process
    // find the vacant slot in the swap block array
    // fetch the page using the return valie of th victim page process
    // then use this in the write to disk function
    // need to update the rss value of the process
    // also need to edit the attributes of the swap slot
    // need to edit the pte entry of the victim page 
    struct proc *p;
    p = findVictimProcess();
    if (p == NULL) // Check if a valid process is found
        panic("not a valid process");
    p->rss -= 1;
    pte_t *pte;
    pte = findVictimPage(p);
    char *addPage = (char*)PGROUNDUP(P2V(pte));
    if (pte == NULL) // Check if a valid page table entry is found
        panic("not a valid page found");
    int vacantSlot = findVacantSwapSlot();
    int diskBlock = diskBlockNumber(vacantSlot);
    // acquire the lock on the swap slot here 
    acquire(&swap_block.lock);
    writeToDisk(ROOTDEV, addPage, diskBlock);
    swap_block.slots[vacantSlot].is_free = 0;
    swap_block.slots[vacantSlot].page_perm = (int)PTE_FLAGS(pte);
    // release the lock on the swap_slot
    release(&swap_block.lock);
    kfree(addPage);
    editPTEentry(pte, diskBlock);
}

