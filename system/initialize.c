/**
 * @file initialize.c
 * The system begins intializing after the C environment has been
 * established.  After intialization, the null thread remains always in
 * a ready (THRREADY) or running (THRCURR) state.
 */
/* Embedded Xinu, Copyright (C) 2009, 2013.  All rights reserved. */

#include <kernel.h>
#include <backplane.h>
#include <clock.h>
#include <device.h>
#include <gpio.h>
#include <memory.h>
#include <bufpool.h>
#include <mips.h>
#include <thread.h>
#include <tlb.h>
#include <queue.h>
#include <semaphore.h>
#include <monitor.h>
#include <mailbox.h>
#include <network.h>
#include <nvram.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <safemem.h>
#include <platform.h>
#include <framealloc.h>
#include <paging.h>

#include <stdlib.h>

#ifdef WITH_USB
#  include <usb_subsystem.h>
#endif

/* Function prototypes */
extern thread main(void);       /* main is the first thread created    */
static int sysinit(void);       /* intializes system structures        */

/* Declarations of major kernel variables */
struct thrent thrtab[NTHREAD];  /* Thread table                   */
struct sement semtab[NSEM];     /* Semaphore table                */
struct monent montab[NMON];     /* Monitor table                  */
qid_typ readylist;              /* List of READY threads          */
struct memblock memlist;        /* List of free memory blocks     */
struct bfpentry bfptab[NPOOL];  /* List of memory buffer pools    */

/* Active system status */
int thrcount;                   /* Number of live user threads         */
tid_typ thrcurrent;             /* Id of currently running thread      */

/* Params set by startup.S */
void *memheap;                  /* Bottom of heap (top of O/S stack)   */
ulong cpuid;                    /* Processor id                        */
uint *framelist;                /* address for the bottom of the frame allocation bitmap */

struct platform platform;       /* Platform specific configuration     */

/**
 * Intializes the system and becomes the null thread.
 * This is where the system begins after the C environment has been 
 * established.  Interrupts are initially DISABLED, and must eventually 
 * be enabled explicitly.  This routine turns itself into the null thread 
 * after initialization.  Because the null thread must always remain ready 
 * to run, it cannot execute code that might cause it to be suspended, wait 
 * for a semaphore, or put to sleep, or exit.  In particular, it must not 
 * do I/O unless it uses kprintf for synchronous output.
 */
void nulluser(void)
{
    /* Platform-specific initialization  */
    platforminit();

    kprintf("Initializing OS...\n");

    /* General initialization  */
    sysinit();

    /* Enable interrupts  */
    enable();

    kprintf("Gonna spawn mah new thread\n");
    /* Spawn the main thread  */
    //ready(create(main, INITSTK, INITPRIO, "MAIN", 0), RESCHED_YES);
    // Spawn a test thread to figure out these paging issues
    ready(create(nothingthread, INITSTK, INITPRIO, "NOTHINGTHREAD1", 0), RESCHED_NO);

    /* null thread has nothing else to do but cannot exit  */
    while (TRUE)
    {
      
#ifndef DEBUG
        pause();
#endif                          /* DEBUG */
    }
}

void nothingthread(void) {
  //asm("nop");
  ready(create(spawnedthread, INITSTK, INITPRIO, "SPAWNEDTHREAD", 0), RESCHED_NO);
  //kprintf("Gonna print stuff out yeahhhhhh\n");
  //kprintf("hoooo boy do I sure love printing shit out\n");
  //resched();
}

void spawnedthread(void) {
  // Just try some basic math for now
  int thing = 2;
  thing += thing;
}


/**
 * Intializes all Xinu data structures and devices.
 * @return OK if everything is initialized successfully
 */
static int sysinit(void)
{
  initframealloc();

  // Create a page directory and page table for identity mapping the kernel
  uint *nullthreadpagedir = (uint*)framealloc();
  initpagetable(nullthreadpagedir);
  // Map the last entry of the directory to itself
  *(nullthreadpagedir + 1023) = (uint)nullthreadpagedir | 3;
  kernelidentitytable = (uint*)framealloc();
  initpagetable(kernelidentitytable);

  // File kernelidentitytable with identity mappings over kernelspace
  int j;
  for(j = 0; (j * 0x1000) < USERSPACE_BASE; j++) {
    *(kernelidentitytable + j) = (j * 0x1000) | 3;
  }

  // Place kernel identity table in kernel's page directory
  *(nullthreadpagedir + 0) = (uint)kernelidentitytable | 3;
  
  // Attempt to log info about x86_64 control registers
  uint cr0, cr2, cr3, cr4;
  __asm__ __volatile__ (
			"mov %%cr0, %%eax\n\t"
			"mov %%eax, %0\n\t"
			"mov %%cr2, %%eax\n\t"
			"mov %%eax, %1\n\t"
			"mov %%cr3, %%eax\n\t"
			"mov %%eax, %2\n\t"
			"mov %%cr4, %%eax\n\t"
			"mov %%eax, %3\n\t"
			: "=m" (cr0), "=m" (cr2), "=m" (cr3), "=m" (cr4)
			:
			: "%eax"
			);
  kprintf("cr0= 0x%8.8X\n", cr0);
  kprintf("cr2= 0x%8.8X\n", cr2);
  kprintf("cr3= 0x%8.8X\n", cr3);
  kprintf("cr4= 0x%8.8X\n", cr4);

  loadCR3(nullthreadpagedir);
  enablepaging();
  
  
  kprintf("Tried to enable paging\n\n");
    __asm__ __volatile__ (
			"mov %%cr0, %%eax\n\t"
			"mov %%eax, %0\n\t"
			"mov %%cr2, %%eax\n\t"
			"mov %%eax, %1\n\t"
			"mov %%cr3, %%eax\n\t"
			"mov %%eax, %2\n\t"
			"mov %%cr4, %%eax\n\t"
			"mov %%eax, %3\n\t"
			: "=m" (cr0), "=m" (cr2), "=m" (cr3), "=m" (cr4)
			:
			: "%eax"
			);
  kprintf("cr0= 0x%8.8X\n", cr0);
  kprintf("cr2= 0x%8.8X\n", cr2);
  kprintf("cr3= 0x%8.8X\n", cr3);
  kprintf("cr4= 0x%8.8X\n", cr4);

  kprintf("End: 0x%8.8X\n", (ulong)&_end);
  kprintf("memheap: 0x%8.8X\n", memheap);

    int i;
    struct thrent *thrptr;      /* thread control block pointer  */
    struct memblock *pmblock;   /* memory block pointer          */

    /* Initialize system variables */
    /* Count this NULLTHREAD as the first thread in the system. */
    thrcount = 1;

    /* Initialize free memory list */
    // I don't think I need any of this but we'll leave it for now just in case
    memheap = roundmb(memheap);
    platform.maxaddr = truncmb(platform.maxaddr);
    memlist.next = pmblock = (struct memblock *)memheap;
    memlist.length = (uint)(platform.maxaddr - memheap);
    pmblock->next = NULL;
    pmblock->length = (uint)(platform.maxaddr - memheap);

    /* Initialize thread table */
    for (i = 0; i < NTHREAD; i++)
    {
        thrtab[i].state = THRFREE;
    }

    /* initialize null thread entry */
    thrptr = &thrtab[NULLTHREAD];
    thrptr->state = THRCURR;
    thrptr->prio = 0;
    strlcpy(thrptr->name, "prnull", TNMLEN);
    thrptr->pagedir = nullthreadpagedir;

    // Setup thread memory management
    // Essentially just a copy of some of the work done in create.c

    // 0x10000 is the normal kernel stack size in x86 xinu
    thrptr->stkbase = (void *) (USERSPACE_END - 0x10000);
    thrptr->stklen = 0x10000;
    // Give it an extra frame below the stack for when it initializes the stackx
    pageregion(USERSPACE_END - 0x21000, USERSPACE_END - 1);
    thrptr->stkptr = 0;
    // Setup memlist stuff
    // First, create the memblock for the thread->memlist
    // This is a bad hack, but gcc's giving me type issues otherwise
    pageregion(USERSPACE_BASE, USERSPACE_BASE + 0x1000);
    uint *memlisteditor = (uint*) &(thrptr->memlist);
    *memlisteditor = USERSPACE_BASE;
    // Now, create the memblock for the first memblock
    struct memblock *memlistnext = (struct memblock*)(USERSPACE_BASE + sizeof(struct memblock));
    // Initialize those data structures
    memlistnext->next = NULL;
    memlistnext->length = USERSPACE_END - sizeof(struct memblock) * 2;
    thrptr->memlist.next = memlistnext;
    thrptr->memlist.length = USERSPACE_END - sizeof(struct memblock) * 2;
      
    thrcurrent = NULLTHREAD;

    /* Initialize semaphores */
    for (i = 0; i < NSEM; i++)
    {
        semtab[i].state = SFREE;
        semtab[i].queue = queinit();
    }

    /* Initialize monitors */
    for (i = 0; i < NMON; i++)
    {
        montab[i].state = MFREE;
    }

    /* Initialize buffer pools */
    for (i = 0; i < NPOOL; i++)
    {
        bfptab[i].state = BFPFREE;
    }

    /* initialize thread ready list */
    readylist = queinit();

#if SB_BUS
    backplaneInit(NULL);
#endif                          /* SB_BUS */

#if RTCLOCK
    /* initialize real time clock */
    clkinit();
#endif                          /* RTCLOCK */

#ifdef UHEAP_SIZE
    /* Initialize user memory manager */
    {

        void *userheap;             /* pointer to user memory heap   */
        userheap = stkget(UHEAP_SIZE);
        if (SYSERR != (int)userheap)
        {
            userheap = (void *)((uint)userheap - UHEAP_SIZE + sizeof(int));
            memRegionInit(userheap, UHEAP_SIZE);

            /* initialize memory protection */
            safeInit();

            /* initialize kernel page mappings */
            safeKmapInit();
        }
    }
#endif

#if USE_TLB
    /* initialize TLB */
    tlbInit();
    /* register system call handler */
    exceptionVector[EXC_SYS] = syscall_entry;
#endif                          /* USE_TLB */

#if NMAILBOX
    /* intialize mailboxes */
    mailboxInit();
#endif

#if NDEVS
    for (i = 0; i < NDEVS; i++)
    {
        devtab[i].init((device*)&devtab[i]);
    }
#endif

#ifdef WITH_USB
    usbinit();
#endif

#if NVRAM
    nvramInit();
#endif

#if NNETIF
    netInit();
#endif

#if GPIO
    // I can't change the value of this GPIO definition in xinu.conf for some reason
    // All this really does is just turn on an LED though :p
    //gpioLEDOn(GPIO_LED_CISCOWHT);
#endif
    kprintf("Got to the end of initialize.c\n");
    return OK;
}
