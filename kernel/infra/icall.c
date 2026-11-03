#include <linux/compiler.h>

#include "infra/icall.h"

#ifdef CONFIG_X86_64
typedef unsigned long (*ksu_icall8_fn)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long,
                                       unsigned long, unsigned long, unsigned long);

/* See icall.h.  noinline keeps the call's 14-byte window inside this function, where the
 * compiler emits only argument moves in front of it. */
noinline __nocfi unsigned long ksu_icall8(const void *fn, unsigned long a0, unsigned long a1, unsigned long a2,
                                          unsigned long a3, unsigned long a4, unsigned long a5, unsigned long a6,
                                          unsigned long a7)
{
    return ((ksu_icall8_fn)fn)(a0, a1, a2, a3, a4, a5, a6, a7);
}
#endif
