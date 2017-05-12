/**
 * @file create.c
 */
/* Embedded Xinu, Copyright (C) 2007, 2013.  All rights reserved. */

#include <platform.h>
#include <string.h>
#include <thread.h>
#include <paging.h>
#include <framealloc.h>

static int thrnew(void);

/**
 * @ingroup threads
 *
 * Create a thread to start running a procedure.
 *
 * @param procaddr
 *      procedure address
 * @param ssize
 *      stack size in bytes
 * @param priority
 *      thread priority (0 is lowest priority)
 * @param name
 *      name of the thread, used for debugging
 * @param nargs
 *      number of arguments that follow
 * @param ...
 *      arguments to pass to thread procedure
 * @return
 *      the new thread's thread id, or ::SYSERR if a new thread could not be
 *      created (not enough memory or thread entries).
 */
tid_typ create(void *procaddr, uint ssize, int priority,
               const char *name, int nargs, ...)
{
  kprintf("Attempting to create--------------------------------\n");
    irqmask im;                 /* saved interrupt state               */
    ulong *saddr;               /* stack address                       */
    tid_typ tid;                /* new thread ID                       */
    struct thrent *thrptr;      /* pointer to new thread control block */
    va_list ap;                 /* list of thread arguments            */

    im = disable();

    // Initialize the new thread's paging structures
    uint pagediraddr = framealloc();

    // To initialize the data structures of the new thread, we will take the first four page table entries
    // in the second page table of the currently running thread (with virtual address 0x00400000) and readdress them to point to the
    // frames which will be used in the new thread.
    // In particular, we will have a page entry for modifying page directory entries, a page table's entries
    // (and we can update which page table), and the entries in two pages
    // This allows us to initialize the new thread's page directory, stack, and memlist

    // Obviously, there are more compact ways to do this but I think this is the clearest to understand

    ////////////////////////////////
    // Use re-addressing to initialize the new thread's page directory

    // First, we must initialize the page table in the current thread if it hasn't been allocated
    uint *pagedirentry = (uint*)0xFFFFF004; //pagedirentry of the currently running thread
    bool hadtomakepagetable = FALSE;
    if(get_bit(*pagedirentry, 0) == 0) {
      hadtomakepagetable = TRUE;
      *pagedirentry = framealloc() | 3;
    }

    // Now we can use the page table
    uint *currpagetablemodifier = (uint*)0xFFC01000;
    // Save the current pte values so we can restore them later
    uint savedptes[4];
    int i;
    for(i = 0; i < 4; i++) {
      savedptes[i] = *(currpagetablemodifier + i);
    }

    // Describe the 4 values to be used to create the new thread
    //uint *newfirstpage = (uint*)0x00400000;
    uint *newfirstpagemodifier = currpagetablemodifier + 0;
    //uint *newsecondpage = (uint*)0x00401000;
    uint *newsecondpagemodifier = currpagetablemodifier + 1;
    uint *newpagedir = (uint*)0x00402000;
    uint *newpagedirmodifier = currpagetablemodifier + 2;
    uint *newpagetable = (uint*)0x00403000;
    uint *newpagetablemodifier = currpagetablemodifier + 3;
    
    // Point the pte to our new page directory
    *newpagedirmodifier = pagediraddr | 3;
    // Now we can use the variable newpagedir as a virtual pointer to the pagedirectory's contents
    initpagetable(newpagedir);
    // Map the kernel identity table
    *newpagedir = (uint)kernelidentitytable | 3;
    // Map the page directory to itself
    *(newpagedir + 1023) = pagediraddr | 3;

    //////////////////////////////
    // Now we use this same re-addressing technique to initialize the new thread's stack
    
    if (ssize < MINSTK)
    {
        ssize = MINSTK;
    }

    /* Allocate new stack.  */
    //saddr = stkget(ssize);
    saddr = (ulong*) (USERSPACE_END - ssize);
    // Give it an extra page of stack size since setting up the stack requires accessing several addresses
    // south of saddr
    pageregionwith(USERSPACE_END - ssize - 0x1000, USERSPACE_END - 1, newpagedir, newpagetablemodifier, newpagetable);
	
    if (SYSERR == (int)saddr)
    {
      // TODO: Resolve re-addressing for paging
        restore(im);
        return SYSERR;
    }

    /* Allocate new thread ID.  */
    tid = thrnew();
    if (SYSERR == (int)tid)
    {
      // TODO: Resolve re-addressing for paging
        stkfree(saddr, ssize);
        restore(im);
        return SYSERR;
    }

    /* Set up thread table entry for new thread.  */
    thrcount++;
    thrptr = &thrtab[tid];

    thrptr->state = THRSUSP;
    thrptr->prio = priority;
    thrptr->stkbase = saddr;
    thrptr->stklen = ssize;
    strlcpy(thrptr->name, name, TNMLEN);
    thrptr->parent = gettid();
    thrptr->hasmsg = FALSE;

    thrptr->pagedir = (uint*)pagediraddr;

    // Setup memlist stuff
    // Page the region using re-addressing
    pageregionwith(USERSPACE_BASE, USERSPACE_BASE + 0x1000, newpagedir, newpagetablemodifier, newpagetable);
    // Now, for the purposes of re-addressing, we are going to point newfirstpage so that it points to the page which will be used
    // in the new thread
    // In particular, we are going to read off the page table address from the page directory, and use it to point newpagetable
    // to that page
    // Then we shall read that to get the address of the frame corresponding to virtual addresses 0x00400000-0x00401000 for the new
    // thread, and point newfirstpage's modifier so that it points to that frame
    // Since newfirstpage corresponds to virtual addresses 0x00400000, we can hence use those addresses to modify the intended frame
    // for the new thread
    *newpagetablemodifier = *(newpagedir + 1) | 3;
    kprintf("*newpagetablemodifier: 0x%8.8X\n", *newpagetablemodifier);
    *newfirstpagemodifier = *newpagetable | 3;
    kprintf("*newfirstpagemodifier: 0x%8.8X\n", *newfirstpagemodifier);
    
    // First, create the memblock for the thread->memlist
    // This is a bad hack, but gcc's giving me weird type issues otherwise
    uint *memlisteditor = (uint*) &(thrptr->memlist);
    *memlisteditor = USERSPACE_BASE;
    // Now, create the memblock for the first memblock
    struct memblock *memlistnext = (struct memblock*)(USERSPACE_BASE + sizeof(struct memblock));
    // Initialize those data structures
    memlistnext->next = NULL;
    memlistnext->length = USERSPACE_END - USERSPACE_BASE - ssize - sizeof(struct memblock);
    thrptr->memlist.next = memlistnext;
    thrptr->memlist.length = USERSPACE_END - USERSPACE_BASE - ssize - sizeof(struct memblock);
    kprintf("done setting up memlist stuff\n");

    /* Set up default file descriptors.  */
    thrptr->fdesc[0] = CONSOLE; /* stdin  is console */
    thrptr->fdesc[1] = CONSOLE; /* stdout is console */
    thrptr->fdesc[2] = CONSOLE; /* stderr is console */

    /* Set up new thread's stack with context record and arguments.
     * Architecture-specific.  */
    va_start(ap, nargs);
    
    // Now I need to do the same thing, where I'm going to use logicalfirstaddr and logicalsecondaddr to point to the pages
    // To do this, I need to set up a virtual region where we can modify the frames which will make up the stack
    // Since setupStack actually goes a little below saddr, we need to include the page below it.
    // Hence, we will map page 0x00401000 to the page containing saddr and page 0x00400000 to the page below that
    *newpagetablemodifier = *(newpagedir + (((uint)saddr & 0xFFC00000) >> 22)); // Get the page directory index from saddr
    // Technically, the stack and the lower addresses accessed could potentially be in different page tables
    // This would imply a stack larger than 0x00400000 though, so it's kinda unrealistic to assume that
    uint firststackframeaddr =   *(newpagetable + ((((uint)saddr & 0x003FF000) >> 12) - 1));
    *newfirstpagemodifier = firststackframeaddr | 3;
    uint secondstackframeaddr =  *(newpagetable + (((uint)saddr & 0x003FF000) >> 12));
    *newsecondpagemodifier = secondstackframeaddr | 3;

    // Take the offset of saddr in its page and put it in the 0x00401000 page
    // Now svirtualaddr can be modified and later on when the thread is running, the thread's virtual stack pointer will see those changes
    uint *svirtualaddr = (uint*)(0x00401000 + ((uint)saddr & 0x0000000F));
    thrptr->stkptr = setupVirtualStack(saddr, svirtualaddr, procaddr, INITRET, nargs, ap);
    va_end(ap);

    // Reload CR3 with the address of the currently running thread
    //loadCR3(thrtab[thrcurrent].pagedir);

    // Restore the original value of the currently running thread's pte
    for(i = 0; i < 4; i++) {
      *(currpagetablemodifier + i) = savedptes[i];
    }
    if(hadtomakepagetable) {
      freeframe((*pagedirentry) & 0xFFFFFFF0); // Clear the rightmost flag bits
      *pagedirentry = 0x00000000;
    }

    /* Restore interrupts and return new thread TID.  */
    restore(im);
    kprintf("Done creating\n");

    // So we don't loop, to make output easier to read
    /*
    while(1){
    }
    */
    
    return tid;
}

/*
 * Obtain a new (free) thread ID.  Returns a free thread ID, or SYSERR if all
 * thread IDs are already in use.  This assumes IRQs have been disabled so that
 * the contents of the threads table are stable.
 */
static int thrnew(void)
{
    int tid;
    static int nexttid = 0;

    /* check all NTHREAD slots    */
    for (tid = 0; tid < NTHREAD; tid++)
    {
        nexttid = (nexttid + 1) % NTHREAD;
        if (THRFREE == thrtab[nexttid].state)
        {
            return nexttid;
        }
    }
    return SYSERR;
}
