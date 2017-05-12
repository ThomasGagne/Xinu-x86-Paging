/**
 * @file paging.h
 * Definitions for paging and frame allocation structures
 */

#ifndef _PAGING_H_
#define _PAGING_H_

#include <stddef.h>

#define KERNELSPACE_BASE 0x00000000
#define USERSPACE_BASE   0x00400000
#define USERSPACE_END    0xFFC00000

// Equals the value of the pos'th bit in var
#define get_bit(var,pos) ((var) & (1<<pos))

// PAGING STRUCTURES

// Address of page table for identity mapping the kernel space
extern uint *kernelidentitytable;

// Memory function prototypes
void initpagetable(uint *tablebaseaddr);
void reclaimframes(uint *pagedir);
void pageregion(uint regionstart, uint regionend);
void pageregionwith(uint regionstart, uint regionend, uint *pagedir, uint *ptemodifier, uint *virtualpagetableaddr);
void enablepaging(void);
void loadCR3(uint *pagedir);

#endif
