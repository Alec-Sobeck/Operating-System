#include "kheap.h"
#include "paging.h"
#include "task.h"
#include "descriptor_tables.h"
#include "klib.h"
#include "heap.h"
#include "queue.h"
#include "linked_list.h"
#include "heapindex.h"

extern uint32_t kernel_cleanup_stack;
extern uint32_t initial_esp;
extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;
extern uint32_t read_eip();
extern uint32_t initial_esp;

// Global job queues
list_t *sleeping_jobs = NULL;
list_t *task_list = NULL;
list_t *ready_queue = NULL;
task_t *current_process = NULL;
// Global lists of all semaphores/pipes open
list_t *semaphores_list = NULL;
list_t *pipe_list = NULL;
// Global id generators
uint32_t pipe_id_generator = 1;
uint32_t sem_id_generator = 1;
uint32_t pid_generator = 1;
const int global_parent_id = 1;

static int comparator_pid(void *a, void *b)
{
    task_t *h_a = a;
    task_t *h_b = b;
    return h_a->id < h_b->id ? -1 : h_a->id > h_b->id ? 1 : 0;
}

void register_stack_pointer(uint32_t loc, uint32_t target)
{
    pointer_info_t *ptr = kmalloc(sizeof(*ptr));
    ptr->pointer_memory_loc = loc;
    ptr->pointer_target = target;
    list_add_back(current_process->pointers, ptr);
}

void clear_stack_pointers()
{
    list_foreach(current_process->pointers, kfree);
    list_clear(current_process->pointers);
}

task_t *task_init(page_directory_t *page_dir)
{
    task_t *t = kmalloc(sizeof(*t));
    t->id = pid_generator++;
    t->sleep_ticks = 0;
    t->esp = 0;
    t->ebp = 0;
    t->eip = 0;
    t->page_directory = page_dir;
    t->priority = PRIORITY_NORMAL;
    t->kernel_stack = (uint32_t)kmalloc_a(KERNEL_STACK_SIZE);
    t->initial_priority = t->priority;
    t->time_slice_count = 0;
    t->heap = NULL;
    t->state = state_new;
    t->pointers = list_init();
    t->waiting_processes = list_init();
    t->semaphores = list_init();
    t->pipes = list_init();
    list_add_back(task_list, t);
    return t;
}

void task_create_heap(task_t *t, uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly)
{
    // Need frames for the initial contents
    for(uint32_t i = start; i < end; i += PAGE_SIZE) {
        alloc_frame(get_page(i, 1, current_directory), FALSE, TRUE);
    }
    // Need 1 frame for the index initially. It grows dynamically.
    alloc_frame(get_page(max, 1, current_directory), FALSE, TRUE);
    t->heap = heap_init(start, end, max, supervisor, readonly);
}

void switch_to_user_mode()
{
    // Set up our kernel stack.
    set_kernel_stack(current_process->kernel_stack+KERNEL_STACK_SIZE);

    // Set up a stack structure for switching to user mode.
    asm volatile("  \
      cli; \
      mov $0x23, %ax; \
      mov %ax, %ds; \
      mov %ax, %es; \
      mov %ax, %fs; \
      mov %ax, %gs; \
                    \
       \
      mov %esp, %eax; \
      pushl $0x23; \
      pushl %esp; \
      pushf; \
      pop %eax ;\
      or $0x200, %eax;\
      push %eax;\
      pushl $0x1B; \
      push $1f; \
      iret; \
    1: \
      ");
}

void initialise_scheduler()
{
    asm volatile("cli");

    // We could move the stack at a few different places. JamesM's tutorials suggest moving it here is OK, especially
    // since that won't cause any damage
    move_stack((void*)KSTACK_START, KSTACK_SIZE);

    ready_queue = list_init();
    sleeping_jobs = list_init();
    task_list = list_init();
    current_process = task_init(current_directory);
    current_process->priority = 11;
    current_process->initial_priority = 11;
    current_process->state = state_ready;
    task_create_heap(current_process, UHEAP_START, UHEAP_START + UHEAP_INITIAL_SIZE, UHEAP_MAX, FALSE, FALSE);
    current_process->heap->directory = current_directory;

    semaphores_list = list_init();

    pipe_list = list_init();

    ASSERT(current_process->id == global_parent_id);

    asm volatile("sti");
}

void apply_aging(void *a)
{
    task_t *value = a;
    value->time_slice_count++;

    if(value->id == global_parent_id) {
        value->priority = 11;
    } else if(value->time_slice_count == TIME_SLICE_PER_AGE) {
        value->priority = MAX(value->priority - 1, PRIORITY_MAX);
        value->time_slice_count = 0;
    }
}

void run_scheduler(int add_to_ready, int is_alive, int is_timer_based)
{
    if(!current_process){
        return;
    }

    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    asm volatile("mov %%ebp, %0" : "=r"(ebp));
    eip = read_eip();

    if(is_alive && current_process->state != state_terminating){
        current_process->eip = eip;
        current_process->esp = esp;
        current_process->ebp = ebp;

        list_foreach(ready_queue, apply_aging);

        if(add_to_ready){
            list_add_back(ready_queue, (void*)current_process);
            current_process->state = state_ready;
        } else {
            current_process->state = state_waiting;
        }
    }

    // Update sleeping things.
    struct linked_list_node *node;
    if(is_timer_based){
        node = sleeping_jobs->front;
        while(node){
            task_t *value = node->value;
            assert(value->sleep_ticks > 0);
            value->sleep_ticks--;
            if(value->sleep_ticks == 0){
                list_add_back(ready_queue, value);

                struct linked_list_node *temp = node->next;
                list_remove_node(sleeping_jobs, node);
                node = temp;
                continue;
            }
            node = node->next;
        }
    }

    // Schedule the next job. I assume there's a better way to do this, as a list_ function,
    // but I don't know what it is right now.
    node = ready_queue->front;
    struct linked_list_node *best = node;
    while(node){
        task_t *value = node->value;
        task_t *best_value = best->value;
        if(value->priority < best_value->priority) {
            best = node;
        }
        node = node->next;
    }
    current_process = best->value;
    current_process->priority = current_process->initial_priority;
    current_process->time_slice_count = 0;
    enum task_state prev_state = current_process->state;
    current_process->state = state_running;
    list_remove_node(ready_queue, best);

    // Set other variables so thing don't explode.
    current_directory = current_process->page_directory;
    set_kernel_stack(current_process->kernel_stack + KERNEL_STACK_SIZE);

    eip = current_process->eip;
    esp = current_process->esp;
    ebp = current_process->ebp;

    // For new tasks, we have to JMP to the fork syscall to let them return gracefully at the same location.
    // otherwise, we can just return (eliminating the crazy code from the tutorial)
    if(prev_state == state_new){
        asm volatile("mov %0, %%ecx" : : "r"(eip));
        asm volatile("mov %0, %%esp" : : "r"(esp));
        asm volatile("mov %0, %%ebp" : : "r"(ebp));
        asm volatile("mov %0, %%cr3" : : "r"(current_process->page_directory->physicalAddr));
        asm volatile("jmp *%ecx");
    } else {
        asm volatile("mov %0, %%esp" : : "r"(esp));
        asm volatile("mov %0, %%ebp" : : "r"(ebp));
        asm volatile("mov %0, %%cr3" : : "r"(current_process->page_directory->physicalAddr));
    }

}

void move_stack(void *new_start_stack, uint32_t size)
{
    // Allocate frames for the stack.
    uint32_t i;
    for(i = (uint32_t)new_start_stack - size; i <= (uint32_t)new_start_stack; i += PAGE_SIZE) {
        // The tutorial notes that we want this stack have its flag set to user mode, not kernel mode.
        alloc_frame(get_page(i, 1 , current_directory), FALSE, TRUE);
    }

    // flush the TLB
    uint32_t pagedir_addr;
    asm volatile("mov %%cr3, %0" : "=r" (pagedir_addr));
    asm volatile("mov %0, %%cr3" : : "r" (pagedir_addr));

    // ESP/EBP need fixed too.
    // Aside: ESP tends to be the top of the stack
    // EBP by convention is assigned ESP when calling a function (preserved across calls)
    // initial_esp in the very bottom of the stack (from main.c)
    uint32_t old_esp;
    uint32_t old_ebp;
    asm volatile("mov %%esp, %0" : "=r" (old_esp));
    asm volatile("mov %%ebp, %0" : "=r" (old_ebp));
    // (The stack is definitely moving forward. The question is by how much.)
    uint32_t offset = (uint32_t)new_start_stack - initial_esp;
    uint32_t new_esp = old_esp + offset;
    uint32_t new_ebp = old_ebp + offset;

    memcpy((void*)new_esp, (uint8_t*)old_esp, initial_esp - old_esp/*Just copy the bytes in use*/);

    // Traverse for EBP values and update them.
    uint32_t ebp;
    asm volatile("mov %%ebp, %0 " : "=r"(ebp));
    do {
        uint32_t *ebp_ptr = (uint32_t *)(ebp );
        uint32_t new_ptr = (uint32_t)ebp_ptr + offset;
        uint32_t *as_ptr = (uint32_t *)new_ptr;
        *as_ptr = (*ebp_ptr + offset);
        ebp = *ebp_ptr;
    } while(between((uint32_t)old_esp, ebp, initial_esp));

    // Changing these finalizes our move to the new stack.
    asm volatile("mov %0, %%esp" : : "r"(new_esp));
    asm volatile("mov %0, %%ebp" : : "r"(new_ebp));
}

/// Similar to move_stack except this function copies the contents of the stack to another location,
/// leaving the current one alone. Necessary to get the fork() function to work properly because the
/// kernel stacks for the interupts are in the same memory space
/// This isn't fully safe (if a word on the stack happens to fall inside the two page boundary, things will
/// get interesting) but since this isn't used much, I'm OK with that.
/// \param [in] dest the address of the child stack's memory block. Note that the address is weird here.
/// Stacks ordinarily grow downwards, so this isn't actual stack start. Rather (uint32_t)dest + size === stack start.
/// \param [in] src the address of the parent stack's memory block. Note that the address is weird here.
/// Stacks ordinarily grow downwards, so this isn't actual stack start. Rather (uint32_t)dest + size === stack start.
/// \param [in] size the number of bytes to copy.
void copy_stack(void *dest, void *src, uint32_t size)
{
    // Copy memory
    memcpy(dest, src, size);
    // Fix any registered pointers (eg registers_t *)
    struct linked_list_node *node = current_process->pointers->front;
    while (node) {
        pointer_info_t *ptr = node->value;
        uint32_t ptr_loc = ptr->pointer_memory_loc - (uint32_t) src + (uint32_t) dest;
        uint32_t *word = (uint32_t *) (ptr_loc);
        uint32_t word_offset_from_base = *word - (uint32_t) src;
        *word = word_offset_from_base + (uint32_t) dest;
        node = node->next;
    }

    // Traverse for EBP values and update them.
    uint32_t ebp;
    asm volatile("mov %%ebp, %0 " : "=r"(ebp));
    do {
        uint32_t *ebp_ptr = (uint32_t *)(ebp );
        uint32_t new_ptr = (uint32_t)ebp_ptr - (uint32_t)src + (uint32_t)dest;
        uint32_t *as_ptr = (uint32_t *)new_ptr;
        *as_ptr = (*ebp_ptr - (uint32_t)src + (uint32_t)dest);
        ebp = *ebp_ptr;
    } while(between((uint32_t)src, ebp, (uint32_t)src + size));

}

int cmp_job_pid(void *a, void *b)
{
    task_t *task = a;
    uint32_t pid = (uint32_t)b;
    return task->id == pid ? 0 : -1;
}

task_t *get_task_by_pid(int pid)
{
    return list_get(task_list, (void*)pid, cmp_job_pid);
}

/// Note: is is assumed that since this task has to be
/// current_process in order to execute this function,
/// it must be the case that task is not in the ready queue.
/// this makes our lives a lot easier because we don't need
/// to write a new function to remove from ordered_array_t
void remove_process_from_queues(task_t *task)
{
    ASSERT(task == current_process);
    ASSERT(task);

    // This is slightly bad style because we're doing real work in an assert, but anyway...
    ASSERT(list_remove(task_list, task, comparator_pid) == 0);
    ASSERT(list_remove(sleeping_jobs, task, comparator_pid) == 1);
}

void destroy_directory(page_directory_t *src)
{
    for(int i = 0; i < 1024; i++){
        if(!src->tables[i]){
            continue;
        }

        if(kernel_directory->tables[i] != src->tables[i]){
            // Free any frames the user process has allocated.
            // Doing it here is much better because we can't just lose frames by accident
            page_table_t *table = src->tables[i];
            for(int j = 0; j < 1024; j++){
                if(!page_get_present(&table->pages[j])) {
                    continue;
                }
                free_frame(&table->pages[j]);
            }

            kfree(src->tables[i]);
        }
    }
    kfree(src);
}

void restore_joined_processes(void *data)
{
    int pid = (int)data;
    task_t *task = get_task_by_pid(pid);
    ASSERT(task);

    task->state = state_ready;
    list_add_back(ready_queue, task);

}

void free_current_process_kernel_structs()
{
    // --- Start of black magic 
    // We have to free the stack we're currently executing on and
    // the page_directory_t that is currently loaded in cr3.
    // We'll have to use another block of memory for the stack until we
    // run the scheduler and swap to a running process.

    // Further, we have to free the page directory by swapping to another
    // first. Let's just use the one for the kernel since we're executing on
    // a block of memory in its stack anyway.

    // Change stacks. We don't need anything that is on our current stack.
    // That makes this quite easy because we can just trash the contents
    // Also: give the stack frame non-zero size just for the fun of it.
    uint32_t new_esp = kernel_cleanup_stack + KERNEL_STACK_SIZE - 64;
    uint32_t new_ebp = kernel_cleanup_stack + KERNEL_STACK_SIZE - 4;

    asm volatile("mov %0, %%esp" : : "r"(new_esp));
    asm volatile("mov %0, %%ebp" : : "r"(new_ebp));

    kfree(current_process->heap->index);
    kfree(current_process->heap);

    // Switch to the kernel_directory since the current one is about to get trashed.
    switch_page_directory(kernel_directory);
    destroy_directory(current_process->page_directory);
    list_foreach(current_process->semaphores, reduce_sem_refcounts);
    list_destroy(current_process->semaphores);
    list_foreach(current_process->pipes, reduce_pipe_refcounts);
    list_destroy(current_process->pipes);

    list_foreach(current_process->waiting_processes, restore_joined_processes);
    list_destroy(current_process->waiting_processes);

    // Destroy the kernel stack
    kfree((void*)current_process->kernel_stack);

    kfree(current_process->pointers);

#ifdef DEBUG_MEMORY
    uint32_t pid = current_process->id;
#endif

    // Remember to free the process itself!
    kfree((void*)current_process);

#ifdef DEBUG_MEMORY
    kprintf("[Exit pid = %d]: %d free frames. %d free heap memory / %d (account for 28 bytes less due to a pointer_info_t allocation being different) \n",
            pid, get_number_free_frames(), heap_remaining_space(kernel_heap), (kernel_heap->end_address - kernel_heap->start_address));
//    kprintf("=> %d \n", list_size(ready_queue));
#endif


    // Finally, select the next process to run.
    run_scheduler(FALSE, FALSE, FALSE);
}

int fork_impl()
{
    // To create a new process, it needs:
    // -- a new address space
    // -- new registers
    // -- a parent

#ifdef DEBUG_MEMORY
    kprintf("[Fork pid = %d]: %d free frames. %d free heap memory / %d \n", current_process->id,
            get_number_free_frames(), heap_remaining_space(kernel_heap), (kernel_heap->end_address - kernel_heap->start_address));
//    kprintf("=> %d \n", list_size(ready_queue));
#endif

    task_t *parent = (task_t*)current_process;
    task_t *child = task_init(clone_directory(current_directory));

    list_add_back(ready_queue, child);
    uint32_t eip = read_eip(); // Child will enter here.

    if(current_process == parent) {
        // Parent process: Initialize the rest of the child
        uint32_t esp; asm volatile("mov %%esp, %0" : "=r"(esp));
        uint32_t ebp; asm volatile("mov %%ebp, %0" : "=r"(ebp));
        uint32_t offset_ebp = ebp - parent->kernel_stack;
        uint32_t offset_esp = esp - parent->kernel_stack;
        ASSERT (offset_ebp < KERNEL_STACK_SIZE && offset_esp < KERNEL_STACK_SIZE );
        child->esp = child->kernel_stack + offset_esp;
        child->ebp = child->kernel_stack + offset_ebp;
        child->eip = eip;
        copy_stack((void*)child->kernel_stack, (void*)parent->kernel_stack, KERNEL_STACK_SIZE);

        // The behaviour of the copy, with respect to priorities, is kind of left up to us
        // Thus, I've chosen to copy the priority and initial priorities from the parent
        // but let the child's time_slice_count start out fresh from 0.
        if(parent->id != global_parent_id){
            child->priority = parent->priority;
            child->initial_priority = parent->initial_priority;
        } else {
            child->priority = PRIORITY_NORMAL;
            child->initial_priority = PRIORITY_NORMAL;
        }

        // Copy the heap
        heap_t *heap_copy = kmalloc(sizeof(heap_t));
        heap_copy->start_address = parent->heap->start_address;
        heap_copy->end_address = parent->heap->end_address;
        heap_copy->max_address = parent->heap->max_address;
        heap_copy->readonly = parent->heap->readonly;
        heap_copy->supervisor = parent->heap->supervisor;
        heap_copy->index = kmalloc(sizeof(heap_index_t));
        heap_copy->index->size = parent->heap->index->size;
        heap_copy->index->comparator = parent->heap->index->comparator;
        heap_copy->index->max_size = parent->heap->index->max_size;
        heap_copy->index->max_address = parent->heap->index->max_address;
        heap_copy->index->owner = heap_copy;
        heap_copy->directory = child->page_directory;
        child->heap = heap_copy;

        list_enumerator_t enumerator = list_get_enumerator(parent->semaphores);
        while(list_has_next(&enumerator)){
            void *id = list_next_value(&enumerator);
            sem_t *sem = get_semaphore_by_id((uint32_t)id);
            if(!sem){
                continue; // sem is dead already.
            }
            sem->refcount++;
            list_add_back(child->semaphores, id);
        }

        enumerator = list_get_enumerator(parent->pipes);
        while(list_has_next(&enumerator)){
            void *id = list_next_value(&enumerator);
            pipe_t *pipe = get_pipe_by_id((uint32_t)id);
            if(!pipe){
                continue;
            }
            pipe->refcount++;
            list_add_back(child->pipes, id);
        }


        return child->id;
    } else {
        // Child process: just exit.
        return 0;
    }
}

int getpid_impl()
{
    return current_process->id;
}

void yield_impl()
{
    run_scheduler(TRUE, TRUE, FALSE);
}

void exit_impl()
{
    // We have to free the page directory
    // user stack
    // user heap
    // plus remove from all relevant queues
    // resource handles eg pipes and semaphores (if their refcount == 0)
    current_process->state = state_terminating;
    remove_process_from_queues((task_t *) current_process);

    // ~~~~ IMPORTANT ~~~~
    // This effectively kills the process because it destroys its memory space
    // Nothing after this in the exit_impl() function will run or have any effect
    free_current_process_kernel_structs();

    // --- the system call will never make it here. The above function call makes sure of that.
    ASSERT(FALSE);
}

void *alloc_impl(uint32_t size, uint8_t page_align)
{
    ASSERT(current_directory == current_process->heap->directory);
    return heap_alloc(current_process->heap, size, page_align);
}

void free_impl(void *p)
{
    heap_free(current_process->heap, p);
}

int sleep_impl(unsigned int secs)
{
    current_process->sleep_ticks = TICKS_PER_SECOND * secs;
    list_add_back(sleeping_jobs, (task_t *)current_process);

    run_scheduler(FALSE, TRUE, FALSE);

    // Not that we can really be interupted but...
    return current_process->sleep_ticks == 0 ? 0 : (current_process->sleep_ticks / TICKS_PER_SECOND + 1);
}

int set_priority_impl(int pid, int new_priority)
{
    task_t *task = get_task_by_pid(pid);

    if(!task){
        // Can't find the PID anywhere. Return 0.
        return 0;
    }

    ASSERT(task->id == pid);
    if(task->id != current_process->id){
        // We're accessing the PID of another process.
        // This is OK. we can read that value. Just not change it.
        return task->priority;
    }

    if(new_priority < 1 || new_priority > 10){
        return 0;
    }

    // Set priority and return it -- this is the case where everything actually went as expected.
    task->priority = (uint32_t)new_priority;
    task->initial_priority = task->priority;
    task->time_slice_count = 0;
    return new_priority;
}

int join_impl(int pid)
{
    task_t *proc = get_task_by_pid(pid);
    if(!proc){
        return -1;
    }

    list_add_back(proc->waiting_processes, (void*)current_process->id);
    run_scheduler(FALSE, TRUE, FALSE);
    return 0;
}







