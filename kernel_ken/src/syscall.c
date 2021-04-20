#include "syscall.h"
#include "monitor.h"


extern uint32_t read_eip();
static void syscall_handler(registers_t *regs);

///
/// Define system calls here.
///

#define SYSCALL_FORK 3
DEFN_SYSCALL1(monitor_write, 0, const char*);
DEFN_SYSCALL1(monitor_write_hex, 1, uint32_t);
DEFN_SYSCALL1(monitor_write_dec, 2, uint32_t);
DEFN_SYSCALL0(fork_impl, SYSCALL_FORK);
DEFN_SYSCALL0(getpid_impl, 4);
DEFN_SYSCALL0(yield_impl, 5);
DEFN_SYSCALL0(exit_impl, 6);
DEFN_SYSCALL2(alloc_impl, 7, uint32_t, uint8_t);
DEFN_SYSCALL1(free_impl, 8, void*);
DEFN_SYSCALL1(sleep_impl, 9, unsigned int);
DEFN_SYSCALL2(set_priority_impl, 10, int, int);
DEFN_SYSCALL1(open_sem_impl, 11, int);
DEFN_SYSCALL1(wait_impl, 12, int);
DEFN_SYSCALL1(signal_impl, 13, int);
DEFN_SYSCALL1(close_sem_impl, 14, int);
DEFN_SYSCALL0(open_pipe_impl, 15);
DEFN_SYSCALL3(write_impl, 16, int, const void *, unsigned int);
DEFN_SYSCALL3(read_impl, 17, int, void *, unsigned int);
DEFN_SYSCALL1(close_pipe_impl, 18, int);
DEFN_SYSCALL1(join_impl, 19, int);
DEFN_SYSCALL3(monitor_colour, 20, int, int, unsigned int);

///
/// Now register them in the following array:
///
#define NUM_SYSCALLS 21
static void *syscalls[NUM_SYSCALLS] =
{
        &monitor_write,
        &monitor_write_hex,
        &monitor_write_dec,
        &fork_impl,
        &getpid_impl,
        &yield_impl,
        &exit_impl,
        &alloc_impl,
        &free_impl,
        &sleep_impl,
        &set_priority_impl,
        &open_sem_impl,
        &wait_impl,
        &signal_impl,
        &close_sem_impl,
        &open_pipe_impl,
        &write_impl,
        &read_impl,
        &close_pipe_impl,
        &join_impl,
        &monitor_colour
};

/// -----------------------------------------

void initialise_syscalls()
{
    // Register our syscall handler.
    register_interrupt_handler (0x80, &syscall_handler);
}

void syscall_handler(registers_t *regs)
{
    // Firstly, check if the requested syscall number is valid.
    // The syscall number is found in EAX.
    if (regs->eax >= NUM_SYSCALLS)
        return;

    if(regs->eax == SYSCALL_FORK){
        register_stack_pointer((uint32_t)&regs, (uint32_t)regs);
    }

    // Get the required syscall location.
    void *location = syscalls[regs->eax];

    // We don't know how many parameters the function wants, so we just
    // push them all onto the stack in the correct order. The function will
    // use all the parameters it wants, and we can pop them all back off afterwards.
    uint32_t ret;
    asm volatile (" \
     push %1; \
     push %2; \
     push %3; \
     push %4; \
     push %5; \
     call *%6; \
     pop %%ebx; \
     pop %%ebx; \
     pop %%ebx; \
     pop %%ebx; \
     pop %%ebx; \
   " : "=a" (ret) : "r" (regs->edi), "r" (regs->esi), "r" (regs->edx), "r" (regs->ecx), "r" (regs->ebx), "r" (location));
    regs->eax = ret;



    clear_stack_pointers();


}
