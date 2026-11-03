#ifndef __KSU_ICALL_H
#define __KSU_ICALL_H

/*
 * KSU_ICALL(fn, args...): call through a runtime-resolved pointer without a CFI check.
 *
 * Calls through a resolved kernel address (the saved syscall table, an LSM hook's original,
 * a kallsyms helper) must be unchecked, which is what the enclosing __nocfi does.  On x86_64
 * an unchecked indirect call is also a hazard when the module is LOADED: module_finalize() ->
 * apply_fineibt() reads the 14 bytes in front of every .retpoline_sites call
 * (cfi_disable_callers(), cfi_rand_callers(), Linux 6.2+) and, when they start with 41 ba
 * (movl $hash,%r10d) or eb 0c, takes them for the kCFI caller sequence and rewrites the four
 * bytes after them.  In front of an unchecked call those bytes are whatever the compiler put
 * there, often a rel32 whose value depends on where the module was loaded, so a load can
 * corrupt the code before the call.
 *
 * So on x86_64 every unchecked call goes through one out-of-line trampoline, ksu_icall8(),
 * whose own call is preceded by fixed register moves only.  Up to eight integer or pointer
 * arguments; the result is converted back to the callee's return type (a bool from %al alone,
 * as the ABI leaves the rest of %rax undefined).  Elsewhere this is a plain call.
 */
#ifdef CONFIG_X86_64
unsigned long ksu_icall8(const void *fn, unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
                         unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7);

#define __KSU_IC_UL(x) ((unsigned long)(x))
#define __KSU_IC_0(f) ksu_icall8(f, 0, 0, 0, 0, 0, 0, 0, 0)
#define __KSU_IC_1(f, a) __KSU_IC_2(f, a, 0)
#define __KSU_IC_2(f, a, b) __KSU_IC_3(f, a, b, 0)
#define __KSU_IC_3(f, a, b, c) __KSU_IC_4(f, a, b, c, 0)
#define __KSU_IC_4(f, a, b, c, d) __KSU_IC_5(f, a, b, c, d, 0)
#define __KSU_IC_5(f, a, b, c, d, e) __KSU_IC_6(f, a, b, c, d, e, 0)
#define __KSU_IC_6(f, a, b, c, d, e, g) __KSU_IC_7(f, a, b, c, d, e, g, 0)
#define __KSU_IC_7(f, a, b, c, d, e, g, h) __KSU_IC_8(f, a, b, c, d, e, g, h, 0)
#define __KSU_IC_8(f, a, b, c, d, e, g, h, i)                                                                     \
    ksu_icall8(f, __KSU_IC_UL(a), __KSU_IC_UL(b), __KSU_IC_UL(c), __KSU_IC_UL(d), __KSU_IC_UL(e), __KSU_IC_UL(g), \
               __KSU_IC_UL(h), __KSU_IC_UL(i))
/* Counts the arguments after macro expansion (an argument may expand to "x, y"), so the
 * function is part of __VA_ARGS__ here: never empty, no `##` to suppress the expansion.
 * More than eight arguments selects an argument as the "macro" and fails to compile. */
#define __KSU_IC_PICK(_f, _1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define __KSU_IC_N(...)                                                                                       \
    __KSU_IC_PICK(__VA_ARGS__, __KSU_IC_8, __KSU_IC_7, __KSU_IC_6, __KSU_IC_5, __KSU_IC_4, __KSU_IC_3, __KSU_IC_2, \
                  __KSU_IC_1, __KSU_IC_0)(__VA_ARGS__)

#define __KSU_IC_RET(T, v)                                               \
    __builtin_choose_expr(__builtin_types_compatible_p(T, void), (void)(v), \
                          __builtin_choose_expr(__builtin_types_compatible_p(T, _Bool), (_Bool)(unsigned char)(v), (T)(v)))

/* The dead direct call type-checks the arguments against fn's prototype; it is never emitted. */
#define KSU_ICALL(fn, ...)                                                                                       \
    ({                                                                                                           \
        __auto_type __ksu_ic_fn = (fn);                                                                          \
        if (0)                                                                                                   \
            (void)__ksu_ic_fn(__VA_ARGS__);                                                                      \
        __KSU_IC_RET(typeof(__ksu_ic_fn(__VA_ARGS__)), __KSU_IC_N((const void *)__ksu_ic_fn, ##__VA_ARGS__)); \
    })
#else
#define KSU_ICALL(fn, ...) ((fn)(__VA_ARGS__))
#endif

#endif
