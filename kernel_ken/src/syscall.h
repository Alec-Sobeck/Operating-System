// syscall.h -- Defines the interface for and structures relating to the syscall dispatch system.
//              Written for JamesM's kernel development tutorials.


#ifndef SYSCALL_H
#define SYSCALL_H

#include "common.h"
#include "task.h"

void initialise_syscalls();

#define DECL_SYSCALL0(fn) int syscall_##fn();
#define DECL_SYSCALL1(fn,p1) int syscall_##fn(p1);
#define DECL_SYSCALL2(fn,p1,p2) int syscall_##fn(p1,p2);
#define DECL_SYSCALL3(fn,p1,p2,p3) int syscall_##fn(p1,p2,p3);
#define DECL_SYSCALL4(fn,p1,p2,p3,p4) int syscall_##fn(p1,p2,p3,p4);
#define DECL_SYSCALL5(fn,p1,p2,p3,p4,p5) int syscall_##fn(p1,p2,p3,p4,p5);

#define DEFN_SYSCALL0(fn, num) \
int syscall_##fn() \
{ \
    int a; \
    asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
    return a; \
}

#define DEFN_SYSCALL1(fn, num, P1) \
int syscall_##fn(P1 p1) \
{ \
    int a; \
    asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1)); \
    return a; \
}

#define DEFN_SYSCALL2(fn, num, P1, P2) \
int syscall_##fn(P1 p1, P2 p2) \
{ \
    int a; \
    asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2)); \
    return a; \
}

#define DEFN_SYSCALL3(fn, num, P1, P2, P3) \
int syscall_##fn(P1 p1, P2 p2, P3 p3) \
{ \
    int a; \
    asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d"((int)p3)); \
    return a; \
}

#define DEFN_SYSCALL4(fn, num, P1, P2, P3, P4) \
int syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4) \
{ \
    int a; \
    asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4)); \
    return a; \
}

#define DEFN_SYSCALL5(fn, num) \
int syscall_##fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) \
{ \
    int a; \
    asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((int)p1), "c" ((int)p2), "d" ((int)p3), "S" ((int)p4), "D" ((int)p5)); \
    return a; \
}

DECL_SYSCALL1(monitor_write, const char*)
DECL_SYSCALL1(monitor_write_hex, uint32_t)
DECL_SYSCALL1(monitor_write_dec, uint32_t)
DECL_SYSCALL0(fork_impl);
DECL_SYSCALL0(getpid_impl);
DECL_SYSCALL0(yield_impl);
DECL_SYSCALL0(exit_impl);
DECL_SYSCALL2(alloc_impl, uint32_t, uint8_t);
DECL_SYSCALL1(free_impl, void*);
DECL_SYSCALL1(sleep_impl, unsigned int);
DECL_SYSCALL2(set_priority_impl, int, int);
DECL_SYSCALL1(open_sem_impl, int);
DECL_SYSCALL1(wait_impl, int);
DECL_SYSCALL1(signal_impl, int);
DECL_SYSCALL1(close_sem_impl, int);
DECL_SYSCALL0(open_pipe_impl);
DECL_SYSCALL3(write_impl, int, const void *, unsigned int);
DECL_SYSCALL3(read_impl, int, void *, unsigned int);
DECL_SYSCALL1(close_pipe_impl, int);
DECL_SYSCALL1(join_impl, int);
DECL_SYSCALL3(monitor_colour, int, int, unsigned int);








#endif