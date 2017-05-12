/**
 * @file framealloc.c
 * Definitions and initialization for page frame allocation
 */

#include <framealloc.h>
#include <platform.h>
#include <stdio.h>

// Definitions of declared global vars
uint framebaseaddr;
uint numframetableentries;

/**
 * Initialize the necessary structures for page frame allocation
 *
 */
syscall initframealloc() {
  // framelist is apointer to the first entry of the bitmap frame table
  numframetableentries = ((USERSPACE_END - USERSPACE_BASE) / FRAME_SIZE) / 32;
  // Note that we divide by 32 since a single word describes the state of 32 pages

  // Set the base address of the first frame to allocate
  framebaseaddr = USERSPACE_BASE;

  // Zero out all frame table entries to mark the frames as clear
  uint i;
  for(i = 0; i < numframetableentries; i++) {
    *((uint*)framelist + i) = 0x00000000;
  }

  return OK;
}

/**
 * Allocate a free physical frame in memory
 * @return The physical address of the free frame
 */
uint framealloc() {
  uint i,j;
  // Do a search over all frames in our frame table
  for(i = 0; i < numframetableentries; i++) {
    for(j = 0; j < 32; j++) {
      // Check if the jth bit in the ith frame table entry is a 1
      if(!((*(framelist + i) >> j) & 0x00000001)) {
	// Mark the frame as occupied
	*(framelist + i) |= (0x00000001 << j);
	
	return (uint) (framebaseaddr + ((i * 32) + j) * FRAME_SIZE);
      }
    }
  }
  return SYSERR;
}

/**
 * Mark a frame as free
 *
 */
void freeframe(uint frameaddr) {
  if(frameaddr >= framebaseaddr && frameaddr <= framebaseaddr + (numframetableentries * FRAME_SIZE)) {
    int entry = ((frameaddr - framebaseaddr) / FRAME_SIZE) / 32;
    int bit = ((frameaddr - framebaseaddr) / FRAME_SIZE) % 32;
    *(framelist + entry) &= ~(0x00000001 << bit);
  }
}

/**
 * Truncates an address down to the nearest frame-aligned address
 */
uint truncframe(uint addr) {
  return (uint) (addr & ~(FRAME_SIZE - 1));
}

/**
 * Rounds up an address to the nearest frame address
 * Note that a page-aligned address will round up to the next page address
 */
uint roundframe(uint addr) {
  return (uint) (truncframe(addr) + FRAME_SIZE);
}
