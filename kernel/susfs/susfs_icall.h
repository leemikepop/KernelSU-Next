/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SUSFS_ICALL_H
#define __SUSFS_ICALL_H

/*
 * SUSFS_ICALL(fn, args...): call through a runtime-resolved pointer without a CFI check.
 *
 * Every call through a resolved kernel address must be unchecked (the callee's type id does
 * not match), which on arm64 is all the enclosing __nocfi does.  On x86_64 an unchecked
 * indirect call is also a hazard when the module is LOADED: module_finalize() ->
 * apply_fineibt() reads the 14 bytes in front of every .retpoline_sites call
 * (cfi_disable_callers(), cfi_rand_callers()) and, when they start with 41 ba
 * (movl $hash,%r10d) or eb 0c, takes them for the kCFI caller sequence and rewrites the four
 * bytes after them.  In front of an unchecked call those bytes are whatever the compiler put
 * there, often a rel32 whose value depends on where the module and the kernel were loaded, so
 * a load can corrupt the instruction before the call.  Measured on android15-6.6: about one
 * load in 16 hit susfs_watch_freeing(), with "WARNING ... apply_retpolines".
 *
 * So on x86_64 every unchecked call goes through one out-of-line trampoline, susfs_icall8(),
 * whose own call is preceded by fixed register moves only, and tools/x86_icall_windows.py
 * checks the built .ko for any other unchecked call.  Up to eight integer or pointer arguments;
 * the result is converted back to the callee's return type (a bool from %al alone, as the ABI
 * leaves the rest of %rax undefined).  The enclosing functions keep __nocfi for arm64.
 */
#ifdef CONFIG_X86_64
unsigned long susfs_icall8(const void *fn, unsigned long a0, unsigned long a1, unsigned long a2,
			   unsigned long a3, unsigned long a4, unsigned long a5, unsigned long a6,
			   unsigned long a7);

#define __SUSFS_IC_UL(x) ((unsigned long)(x))
#define __SUSFS_IC_0(f) susfs_icall8(f, 0, 0, 0, 0, 0, 0, 0, 0)
#define __SUSFS_IC_1(f, a) __SUSFS_IC_2(f, a, 0)
#define __SUSFS_IC_2(f, a, b) __SUSFS_IC_3(f, a, b, 0)
#define __SUSFS_IC_3(f, a, b, c) __SUSFS_IC_4(f, a, b, c, 0)
#define __SUSFS_IC_4(f, a, b, c, d) __SUSFS_IC_5(f, a, b, c, d, 0)
#define __SUSFS_IC_5(f, a, b, c, d, e) __SUSFS_IC_6(f, a, b, c, d, e, 0)
#define __SUSFS_IC_6(f, a, b, c, d, e, g) __SUSFS_IC_7(f, a, b, c, d, e, g, 0)
#define __SUSFS_IC_7(f, a, b, c, d, e, g, h) __SUSFS_IC_8(f, a, b, c, d, e, g, h, 0)
#define __SUSFS_IC_8(f, a, b, c, d, e, g, h, i)                                               \
	susfs_icall8(f, __SUSFS_IC_UL(a), __SUSFS_IC_UL(b), __SUSFS_IC_UL(c), __SUSFS_IC_UL(d), \
		     __SUSFS_IC_UL(e), __SUSFS_IC_UL(g), __SUSFS_IC_UL(h), __SUSFS_IC_UL(i))
/* Counts the arguments after macro expansion: an argument such as SUS_XATTR_MNT_ID_ARG expands
 * to "idmap," and has to count as two.  That is why the function is part of __VA_ARGS__ here
 * (never empty, so no `##`, which would suppress the expansion).  More than eight arguments
 * selects an argument as the "macro" and fails to compile. */
#define __SUSFS_IC_PICK(_f, _1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define __SUSFS_IC_N(...)                                                                      \
	__SUSFS_IC_PICK(__VA_ARGS__, __SUSFS_IC_8, __SUSFS_IC_7, __SUSFS_IC_6, __SUSFS_IC_5,      \
			__SUSFS_IC_4, __SUSFS_IC_3, __SUSFS_IC_2, __SUSFS_IC_1,                   \
			__SUSFS_IC_0)(__VA_ARGS__)

#define __SUSFS_IC_RET(T, v)                                                      \
	__builtin_choose_expr(__builtin_types_compatible_p(T, void), (void)(v),  \
		__builtin_choose_expr(__builtin_types_compatible_p(T, _Bool),     \
				      (_Bool)(unsigned char)(v), (T)(v)))

/* The dead direct call type-checks the arguments against fn's prototype; it is never emitted. */
#define SUSFS_ICALL(fn, ...)                                                        \
	({                                                                          \
		__auto_type __susfs_ic_fn = (fn);                                   \
		if (0)                                                              \
			(void)__susfs_ic_fn(__VA_ARGS__);                           \
		__SUSFS_IC_RET(typeof(__susfs_ic_fn(__VA_ARGS__)),                  \
			       __SUSFS_IC_N((const void *)__susfs_ic_fn, ##__VA_ARGS__)); \
	})
#else
#define SUSFS_ICALL(fn, ...) ((fn)(__VA_ARGS__))
#endif

#endif /* __SUSFS_ICALL_H */
