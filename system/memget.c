/**
 * @file memget.c
 *
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <interrupt.h>
#include <memory.h>
#include <stdio.h>
#include <paging.h>
#include <thread.h>

/**
 * @ingroup memory_mgmt
 *
 * Allocate heap memory.
 *
 * @param nbytes
 *      Number of bytes requested.
 *
 * @return
 *      ::SYSERR if @p nbytes was 0 or there is no memory to satisfy the
 *      request; otherwise returns a pointer to the allocated memory region.
 *      The returned pointer is guaranteed to be 8-byte aligned.  Free the block
 *      with memfree() when done with it.
 */
void *memget(uint nbytes)
{  
    register struct memblock *prev, *curr, *leftover;
    irqmask im;
    struct thrent *thread;

    if (0 == nbytes)
    {
        return (void *)SYSERR;
    }

    // Setup thread pointer
    thread = &thrtab[thrcurrent];

    /* round to multiple of memblock size   */
    nbytes = (ulong)roundmb(nbytes);

    im = disable();

    prev = &(thread->memlist);
    curr = thread->memlist.next;
    while (curr != NULL)
    {
        if (curr->length == nbytes)
        {
            prev->next = curr->next;
            thread->memlist.length -= nbytes;

            restore(im);
            return (void *)(curr);
        }
        else if (curr->length > nbytes)
        {
  	    pageregion((uint)curr, (uint)(curr + nbytes));
            /* split block into two */
            leftover = (struct memblock *)((ulong)curr + nbytes);
            prev->next = leftover;
            leftover->next = curr->next;
            leftover->length = curr->length - nbytes;
            thread->memlist.length -= nbytes;

            restore(im);
            return (void *)(curr);
        }
        prev = curr;
        curr = curr->next;
    }
    restore(im);
    return (void *)SYSERR;
}
