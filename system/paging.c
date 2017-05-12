/**
 * @file paging.h
 * Functions for performing paging operations
 */

#include <paging.h>
#include <framealloc.h>
#include <stdio.h>

// Definitions of declared global vars
uint *kernelidentitytable;

/**
 * Initializes the page table given a logical base address of the table
 *
 */
void initpagetable(uint *tablebaseaddr) {
  uint i;
  for(i = 0; i < 1024; i++) {
    // Mark each page as read/write and not present
    *(tablebaseaddr + i) = 0x00000002;
  }
}

/**
 * Reclaim all the frames allocated to a certain thread
 * `pagedir` is the page directory of the thread to reclaim from
 */
void reclaimframes(uint *pagedir) {
  // Sadly, we have to do the same thing we did for create, and perform some readdressing

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
  uint savedptes[2];
  savedptes[0] = *currpagetablemodifier;
  savedptes[1] = *(currpagetablemodifier + 1);
  
  // Describe the 4 values to be used to create the old thread
  uint *oldpagedir = (uint*)0x00400000;
  uint *oldpagedirmodifier = currpagetablemodifier;
  uint *oldpagetable = (uint*)0x00401000;
  uint *oldpagetablemodifier = currpagetablemodifier + 1;

  // Now load the page directory and start reclaiming memory
  *oldpagedirmodifier = (uint)pagedir | 3;

  int i,j;
  for(i = 0; i < 1023; i++) {
    if(get_bit(*(oldpagedir + i), 0) == 0) { // If the page table is present
      *oldpagetablemodifier = *(oldpagedir + i) | 3;
      for(j = 0; j < 1024; j++) {
	if(get_bit(*(oldpagetable + j), 0) == 1) { // If frame is present
	  freeframe((*(oldpagetable + j)) & 0xFFFFFFF0); // Clear rightmost 4 bits
	}
      }
      // Reclaim the page table itself last
      freeframe((*(oldpagedir + i)) & 0xFFFFFFF0); // Clear rightmost 4 bits
    }
  }
  
  // Reclaim the page directory of the dying thread
  freeframe(((uint)pagedir) & 0xFFFFFFF0);

  // Restore the original value of the currently running thread's pte
  *currpagetablemodifier = savedptes[0];
  *(currpagetablemodifier + 1) = savedptes[1];
  if(hadtomakepagetable) {
    freeframe((*pagedirentry) & 0xFFFFFFF0); // Clear the rightmost flag bits
    *pagedirentry = 0x00000000;
  }
  
}

/**
 * Uses the page frame allocator to page the defined virtual region, inclusively
 * This will allocate the necessary number of frames, page tables if needed, and
 * will point the page table entries to the allocated frames and the page directory
 * entries to the created page tables
 * If a page for a section of the defined region has already been paged, new frames will not be allocated
 */
void pageregion(uint regionstart, uint regionend) {
  uint pagestart = truncframe(regionstart);
  uint pageend = roundframe(regionend);

  // The virtual address to access the entries of the current page directory with
  uint *pagedir = (uint*)0xFFFFF000;

  uint i;
  for(i = pagestart; i < pageend; i += 0x1000) {
    // Extract the page directory index
    uint pagedirindex = i >> 22;

    uint *pagetable;

    // If the page table's entry in the pde is not present, create a new page table
    if(get_bit(*(pagedir + pagedirindex), 0) == 0) {
      uint pagetableaddr = framealloc();

      // Set page table to R/W and present
      *(pagedir + pagedirindex) = pagetableaddr | 3;

      // Determine the virtual address to modify the entries of the page table
      pagetable = (uint*)(0xFFC00000 + 0x1000 * pagedirindex);

      initpagetable(pagetable);
    }

    // Determine the virtual address to modify the entries of the page table
    pagetable = (uint*)(0xFFC00000 + 0x1000 * pagedirindex);

    // Extract the page table index
    uint pagetableindex = (i << 10) >> 22;

    // Allocate a frame for that page if there isn't one already
    if(get_bit(*(pagetable + pagetableindex), 0) == 0) {
      // Allocate a frame and point the page table entry to it
      *(pagetable + pagetableindex) = framealloc() | 3;
    }
  }
}

/**
 * A modification of pageregion, this performs the same functionality but rather than using the page 
 * directory and tables of the currently running thread, it operates on the given page directory address.
 * Additionally, `ptemodifier` is required to be a pointer to a pte of the current thread which we can freely modify
 * and point to new pages. `virtualpagetableaddr` must be the virtual address of the page which `ptemodifier` corresponds to.
 *
 * I know this is very convoluted and awkward, but separating this code out into this method makes it much easier to set up
 * structures in memory for a new thread during a create()
 */
void pageregionwith(uint regionstart, uint regionend, uint *pagedir, uint *ptemodifier, uint *virtualpagetableaddr) {
  kprintf("entering pageregionwith\n");
  uint pagestart = truncframe(regionstart);
  uint pageend = roundframe(regionend);

  uint i;
  for(i = pagestart; i < pageend; i += 0x1000) {
    // Extract the page directory index
    uint pagedirindex = i >> 22;

    // If the page table's entry in the pde is not present, create a new page table
    if(get_bit(*(pagedir + pagedirindex), 0) == 0) {
      uint pagetableaddr = framealloc();
      kprintf("creating pagetable for pagedir index %d with frameaddr 0x%8.8X\n", pagedirindex, pagetableaddr);

      // Set page table to R/W and present
      *(pagedir + pagedirindex) = pagetableaddr | 3;

      // Set ptemodifier to point to pagetableaddr so we can use virtualpagetableaddr to modify the page table
      *ptemodifier = pagetableaddr | 3;
      
      initpagetable(virtualpagetableaddr);
    } else {
      // Set ptemodifier to point to the page table's address so we can use virtualpagetableaddr to modify the page table
      *ptemodifier = (*(pagedir + pagedirindex)) | 3;
    }

    // Extract the page table index
    uint pagetableindex = (i << 10) >> 22;

    // Allocate a frame for that page if there isn't one already
    if(get_bit(*(virtualpagetableaddr + pagetableindex), 0) == 0) {
      kprintf("allocating page with index %d\n", pagetableindex);
      // Allocate a frame and point the page table entry to it
      *(virtualpagetableaddr + pagetableindex) = framealloc() | 3;
    }
  }

  kprintf("Done with pageregionwith\n");
  //kprintf("\0");
} 

/**
 * Enable paging in the CPU
 *
 */
void enablepaging() {
  __asm__(
	  "mov %%cr0, %%eax\n\t"
	  "or $0x80000000, %%eax\n\t"
	  "mov %%eax, %%cr0\n\t"
	  :
	  :
	  : "%eax"
	  );
}

/**
 * Load the given page directory into CR3
 *
 */
void loadCR3(uint *pagedir) {
  if((uint)pagedir == 0x43d000) {
    asm("nop");
  }
  
  __asm__(
	  "mov %0, %%eax\n\t"
	  "mov %%eax, %%cr3\n\t"
	  :
	  : "r" ((uint)pagedir)
	  : "%eax"
	  );
}
