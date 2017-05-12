/**
 * @file framealloc.h
 * Definitions for structures and initialization of physical page frame allocation
 */

#ifndef _FRAME_ALLOC_H_
#define _FRAME_ALLOC_H_

#include <stddef.h>
#include <paging.h>

#define FRAME_SIZE 4096

// FRAME ALLOCATION
// framelist is a pointer to the first entry of the bitmap frame table
extern uint *framelist;
// The number of entries in the bitmap frame table
extern uint numframetableentries;
// The base physical address of the lowest frame which can be allocated
extern uint framebaseaddr;
// An int corresponding to the last allocated frame
extern uint lastallocatedframe;

// Function prototypes
uint framealloc(void);
void freeframe(uint frameaddr);
syscall initframealloc(void);
uint truncframe(uint addr);
uint roundframe(uint addr);

#endif
