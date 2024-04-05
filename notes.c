// 1. Make the structure for the swap space 
// 2. For swapping in I need to do the following
//     1. Find the victim process for which I will iterate through the entire process table and get the process with the largest rss value and the lower pid value
//     2. once i find the process then i will look for the page i need to swap out
//     3. for swapping out i need to find the space in the disk wherein i will put that page
//     4. once i find the space i will copy the contents of the page to the swap space
//     5. when i do copy the contents tho, i will have to change the access and present and other bits in the page table entry
//     6. 


/*
FUNCTIONS I NEED TO MAKE
1. make a mapping between the array index of the swap block array and the disk block number
2. make functions to write to and from the disk -> accomodate the writing of permissions etc here only
3. make a function to find the victim process which returns an int -> the index of the process in the nproc table
4. make a function to find the victim page in the victim process -> do all the 10 percent stuff in this function itself and return the entry in the page table 
5. make the swap out function wherein you call these helper functions and also update the permissions and the access bits and the otehr bits on the page
6. need to make functions to update the rss value -> where all to update ? -> swap in , swap out , while doing kalloc (can do in allocuvm) , while doing kfree (can do in deallocuvm)
7. 
*/


/*
WHAT ALL DONE
1. Make the struct of swap blocks and swap slot in mkfs.c
2. defined the flag pte_swap in the file mmu.h       ##### check if the address is correct or not
3. in the main func of mkfs.c, changed the value of the variable nmeta by +1
4. 
*/


/*
DOUBTTTTT
1. Do i neeed to shift the disk block numbers of the log blocks and the inode etc etc blocks
*/