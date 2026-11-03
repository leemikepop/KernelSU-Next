// SPDX-License-Identifier: GPL-2.0
#include <linux/compiler.h>

#include "susfs_icall.h"

#ifdef CONFIG_X86_64
typedef unsigned long (*susfs_icall8_fn)(unsigned long, unsigned long, unsigned long,
					 unsigned long, unsigned long, unsigned long,
					 unsigned long, unsigned long);

/* The only unchecked indirect call in the module on x86_64 (see susfs_icall.h).  noinline keeps
 * it out of its callers, so the bytes in front of the call are this function's own argument
 * shuffle and never a relocated field. */
noinline __nocfi unsigned long susfs_icall8(const void *fn, unsigned long a0, unsigned long a1,
					    unsigned long a2, unsigned long a3, unsigned long a4,
					    unsigned long a5, unsigned long a6, unsigned long a7)
{
	return ((susfs_icall8_fn)fn)(a0, a1, a2, a3, a4, a5, a6, a7);
}
#endif
