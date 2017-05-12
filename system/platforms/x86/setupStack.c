/**
 * @file setupStack.c
 */
/* Embedded Xinu, Copyright (C) 2007, 2013.  All rights reserved. */

#include <platform.h>
#include <thread.h>

/** Set up the context record and arguments on the stack for a new thread
 * (x86 version)  */
// stackaddr is the address that will actually be used when the thread is running
// virtualstackaddr is the virtual address used as a placeholder for stackaddr due to virtual memory
void *setupVirtualStack(void *stackaddr, void *virtualstackaddr, void *procaddr,
                 void *retaddr, uint nargs, va_list ap)
{
    ulong *saddr = virtualstackaddr;
    // Have a copy of the real eventual address for saddr follow along
    ulong *saddrreal = stackaddr;
    ulong  savsp;          /* for remembering stack pointer   */
    uint i;

    savsp = (ulong)stackaddr;

    /* push arguments */
    saddr -= nargs;
    saddrreal -= nargs;
    for (i = 0; i < nargs; i++)
    {
        saddr[i] = va_arg(ap, ulong);
    }

    *--saddr     = (ulong)INITRET;
    saddrreal--;
    *--saddr     = (ulong)procaddr;
    saddrreal--;
    *--saddr     = savsp;
    saddrreal--;
    savsp        = (ulong)saddrreal;

    /* now we must emulate what ctxsw expects: flags, regs, and old SP */
    /* emulate 386 "pushal" instruction */
    *--saddr     = 0;
    saddrreal--;
    *--saddr     = 0; /* %eax */
    saddrreal--;
    *--saddr     = 0; /* %ecx */
    saddrreal--;
    *--saddr     = 0; /* %edx */
    saddrreal--;
    *--saddr     = 0; /* %ebx */
    saddrreal--;
    *--saddr     = 0; /* %esp; "popal" doesn't actually pop it, so 0 is ok */
    saddrreal--;
    *--saddr     = savsp; /* %ebp */
    saddrreal--;
    *--saddr     = 0; /* %esi */
    saddrreal--;
    *--saddr     = 0; /* %edi */
    saddrreal--;
    return saddrreal;
}

// Old copy
/** Set up the context record and arguments on the stack for a new thread
 * (x86 version)  */
void *setupStack(void *stackaddr, void *procaddr,
                 void *retaddr, uint nargs, va_list ap)
{
    ulong *saddr = stackaddr;
    ulong  savsp;          /* for remembering stack pointer   */
    uint i;

    savsp = (ulong)saddr;

    /* push arguments */
    saddr -= nargs;
    for (i = 0; i < nargs; i++)
    {
        saddr[i] = va_arg(ap, ulong);
    }

    *--saddr     = (ulong)INITRET;
    *--saddr     = (ulong)procaddr;
    *--saddr     = savsp;
    savsp        = (ulong)saddr;

    /* now we must emulate what ctxsw expects: flags, regs, and old SP */
    /* emulate 386 "pushal" instruction */
    *--saddr     = 0;
    *--saddr     = 0; /* %eax */
    *--saddr     = 0; /* %ecx */
    *--saddr     = 0; /* %edx */
    *--saddr     = 0; /* %ebx */
    *--saddr     = 0; /* %esp; "popal" doesn't actually pop it, so 0 is ok */
    *--saddr     = savsp; /* %ebp */
    *--saddr     = 0; /* %esi */
    *--saddr     = 0; /* %edi */
    return saddr;
}

