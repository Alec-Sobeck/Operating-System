//
// For the most part, these are just wrapper functions for system calls.
//
// initialise_tasking is special though. It puts the OS in user mode and does all other initialization.
//
#include "kernel_ken.h"
#include "syscall.h"
#include "monitor.h"
#include "descriptor_tables.h"
#include "timer.h"

// Output a null-terminated ASCII string to the monitor
void print(const char *c)
{
    syscall_monitor_write(c);
}

// Print an unsigned integer to the monitor in base 16
void print_hex(unsigned int n)
{
    syscall_monitor_write_hex(n);
}

// Print an unsigned integer to the monitor in base 10
void print_dec(unsigned int n)
{
    syscall_monitor_write_dec(n);
}

int fork()
{
    return syscall_fork_impl();
}

int getpid()
{
    return syscall_getpid_impl();
}

///
/// Due to some confusion on how exactly things should be laid out, I ended up putting my OS
/// initialization in here. With the addition of a user_app.c (and .h) file, this is not really
/// needed at all.
/// But I've decided to leave all initialization here just to be consistent and have it all in one location.
/// NOTE: this function will error if called twice, and definitely error if called in user mode.
void initialise_tasking()
{
    static int already_called = FALSE;

    if(already_called){
        PANIC("Error cannot call initialise_tasking() twice.");
    }
    already_called = TRUE;

    init_descriptor_tables();
    monitor_clear();
    asm volatile("sti");
    init_timer(TICKS_PER_SECOND);
    initialise_paging();
    initialise_scheduler();
    initialise_syscalls();
    switch_to_user_mode();

    // I think this is kind of dumb, but I don't see a way around it right now.
    // For the first process created (the parent of everything) just sit in a spinlock
    // this is only scheduled if nothing else is running (due to having a fixed priority of 11)
    if(fork() != 0) {
        assert(getpid() == 1);
        for(;;);
    }

}

void yield()
{
    syscall_yield_impl();
}

void *alloc(uint32_t size, uint8_t page_align)
{
    return (void *) syscall_alloc_impl(size, page_align);
}

void free(void *p)
{
    syscall_free_impl(p);
}

void exit()
{
    syscall_exit_impl();
}

int sleep(unsigned int secs)
{
    return syscall_sleep_impl(secs);
}

int setpriority(int pid, int new_priority)
{
    return syscall_set_priority_impl(pid, new_priority);
}




//  n is the number of processes that can be granted access to the critical
//  region for this semaphore simultaneously. The return value is a semaphore
//  identifier to be used by signal and wait. 0 indicates open_sem failed.
int open_sem(int n)
{
    return syscall_open_sem_impl(n);
}

// The invoking process is requesting to acquire the semaphore, s. If the
//   internal counter allows, the process will continue executing after acquiring
//   the semaphore. If not, the calling process will block and release the
//   processor to the scheduler. Returns semaphore id on success of acquiring
//   the semaphore, 0 on failure.
int wait(int s)
{
    return syscall_wait_impl(s);

}

// The invoking process will release the semaphore, if and only if the process
//   is currently holding the semaphore. If a process is waiting on
//   the semaphore, the process will be granted the semaphore and if appropriate
//   the process will be given control of the processor, i.e. the waking process
//   has a higher scheduling precedence than the current process. The return value
//   is the seamphore id on success, 0 on failure.
int signal(int s)
{
    return syscall_signal_impl(s);
}

// Close the semaphore s and release any associated resources. If s is invalid then
//   return 0, otherwise the semaphore id.
int close_sem(int s)
{
    return syscall_close_sem_impl(s);
}

// Initialize a new pipe and returns a descriptor. It returns INVALID_PIPE
//   when none is available.
int open_pipe()
{
    return syscall_open_pipe_impl();
}

// Write the first nbyte of bytes from buf into the pipe fildes. The return value is the
//   number of bytes written to the pipe. If the pipe is full, no bytes are written.
//   Only write to the pipe if all nbytes can be written.
unsigned int write(int fildes, const void *buf, unsigned int nbyte)
{
    return syscall_write_impl(fildes, buf, nbyte);
}

// Read the first nbyte of bytes from the pipe fildes and store them in buf. The
//   return value is the number of bytes successfully read. If the pipe is
//   invalid, it returns -1.
unsigned int read(int fildes, void *buf, unsigned int nbyte)
{
    return syscall_read_impl(fildes, buf, nbyte);
}

// Close the pipe specified by fildes. It returns INVALID_PIPE if the fildes
//   is not valid.
int close_pipe(int fildes)
{
    return syscall_close_pipe_impl(fildes);
}