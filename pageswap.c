#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#include "types.h"
#include "fs.h"
#include "stat.h"
#include "param.h"
#include "buf.h"
#include "mmu.h"
#include "proc.h"

// function 1 -> to find the mapping between index of swap block array and disk block number 

int diskBlockNumber(int arrayindex){
    return 8*arrayindex + 1;    // check if disk blocks are 0 indexed or 1 indexed
}

void writeToDisk(uint dev, char* pg, int blockno){
    struct buf *b;
    for (int i = 0; i < 8 ; i++){
        b = bget(dev, blockno+i);
        memmove(b->data,pg+i*512,512);   // check if the index pg+i*512 is correct
        bwrite(b);
        brelse(b);
    }
}

void readFromDiskWriteToMem(uint dev, char *pg, uint blockno){
    struct buf* b;
    for (int i = 0 ; i < 8 ; i++){
        b = bread(dev,blockno+i);
        memmove(pg+i*512,b->data,512);
        brelse(b);
    }
}

// put this function in proc.c
struct proc* findVictimProcess(void){
    int maxRss, pid_cur;
    struct proc *p = 0, *p1 = 0;
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
        // if((p->state == UNUSED))
        // continue;
        if ((p->rss > maxRss) || (p->rss == maxRss && p->pid < pid_cur)){
            maxRss = p->rss;
            pid_cur = p->pid;
            p1 = p;   // correct this as this will give wrong as they are pointers
        }
    }
    release(&ptable.lock);
    return p;
}

// write the function to find the victim page given the process and hence its pgdir address
pte_t* findVictimPage(struct proc *p){
    pde_t *pgdir = p->pgdir, *pde;
    pte_t *pgtab, *pte;
    uint va;
    // construct a virtual address using the PGADDR macro wherein you set the d t o to 0 initially
    // make two for loops the outside one iterating over d and the inside one over t 
    // use this virtual addr and other macros pdx etx to index into the pgdir and check the present bit
    // if pte_p is there then index into the page table and check each and every page and its access bits
    // choose the page if pte_p set and pte_a not set
    // return the ddress of the page table entr corresp to it
    for (int d = 0 ; d < pow(2,10) ; d++){
        va = PGADDR((uint)d,0,0);
        pde = &pgdir[PDX(va)];
        if (*pde & PTE_P){
            for (int t = 0 ; t < pow(2,10) ; t++){
                va = PGADDR((uint)d,(uint)t,0);
                pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
                pte = &pgtab[PTX(va)];
                if ((*pte & PTE_P) && !(*pte & PTE_A)){
                    return pte;
                }
            }
        }
    }
    // if we did not find the approproate page then we make 10 percent of the pages as not accessed and then select the victim page accordingly
    int count = 0, found = 0;
    pte_t* toReturn;
    for (int d = 0 ; d < pow(2,10) ; d++){
        va = PGADDR((uint)d,0,0);
        pde = &pgdir[PDX(va)];
        if (*pde & PTE_P){
            for (int t = 0 ; t < pow(2,10) ; t++){
                va = PGADDR((uint)d,(uint)t,0);
                pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
                pte = &pgtab[PTX(va)];
                if ((*pte & PTE_P) && (*pte & PTE_A) && (count%10 == 0)){
                    if (found == 0){
                        found = 1;
                        toReturn = pte;
                    }
                    *pte &= ~PTE_A;
                }
                count = (count+1)%10;
            }
        }
    }
    return toReturn;
}