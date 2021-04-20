#ifndef TASK_H
#define TASK_H

#include "common.h"
#include "paging.h"
#include "heap.h"
#include "linked_list.h"
#include "queue.h"

#include "pipe.h"
#include "semaphore.h"

enum task_state
{
    state_new, state_ready, state_running, state_waiting, state_terminating
};

/// This structure defines a 'task' - a process.
typedef struct task
{
    uint32_t initial_priority;
    uint32_t time_slice_count;
    uint32_t priority;
    uint32_t sleep_ticks;
    uint32_t id;                // Process ID.
    uint32_t esp, ebp;       // Stack and base pointers.
    uint32_t eip;            // Instruction pointer.
    page_directory_t *page_directory; // Page directory.
    uint32_t kernel_stack; //bp
    heap_t *heap;
    enum task_state state;
    list_t *pointers;
    list_t *waiting_processes;
    list_t *semaphores;
    list_t *pipes;
} task_t;

typedef struct
{
    uint32_t pointer_memory_loc;
    uint32_t pointer_target;
} pointer_info_t;

void register_stack_pointer(uint32_t loc, uint32_t target);
void clear_stack_pointers();


void switch_to_user_mode();

/// Moves the kernel's stack (or another process, I suppose?)
/// Based heavily on JamesM's tutorial #9
void move_stack(void *new_start_stack, uint32_t size);

/// Executes the scheduler, stopping the current job and then running the job with highest priority in the
/// queue next.
/// Partially based on JamesM's code from tutorial #9, but modified to work with
/// our requirements.
void run_scheduler(int add_to_ready, int is_alive, int is_timer_based);
void initialise_scheduler();

int fork_impl();
int getpid_impl();
void yield_impl();
void exit_impl();
void *alloc_impl(uint32_t size, uint8_t page_align);
void free_impl(void *p);
int sleep_impl(unsigned int secs);
int set_priority_impl(int pid, int new_priority);
int join_impl(int pid);

task_t *get_task_by_pid(int pid);





#endif
