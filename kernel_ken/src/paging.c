// paging.c -- Defines the interface for and structures relating to paging.
//             Written for JamesM's kernel development tutorials.

#include "paging.h"
#include "kheap.h"
#include "monitor.h"
#include "klib.h"

// The kernel's page directory
page_directory_t *kernel_directory=0;

// The current page directory;
page_directory_t *current_directory=0;

// A block of memory we can jump to when we have to clean up processes.
// Effectively an always available kernel stack, if you will.
uint32_t kernel_cleanup_stack = NULL;

// A bitset of frames - used or free.
uint32_t *frames;
uint32_t nframes;

char *src_copy_buffer = NULL;
char *dest_copy_buffer = NULL;

// Defined in kheap.c
extern uint32_t placement_address;
extern heap_t *kernel_heap;

// Macros used in the bitset algorithms.
#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

// Static function to set a bit in the frames bitset
static void set_frame(uint32_t frame_addr)
{
    uint32_t frame = frame_addr/0x1000;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFFSET_FROM_BIT(frame);
    frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clear_frame(uint32_t frame_addr)
{
    uint32_t frame = frame_addr/0x1000;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFFSET_FROM_BIT(frame);
    frames[idx] &= ~(0x1u << off);
}

// Static function to test if a bit is set.
uint32_t test_frame(uint32_t frame_addr)
{
    uint32_t frame = frame_addr/0x1000;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFFSET_FROM_BIT(frame);
    return (frames[idx] & (0x1u << off));
}

// Static function to find the first free frame.
uint32_t get_number_free_frames()
{
    uint32_t number_free = 0;
    uint32_t i, j;
    for (i = 0; i < INDEX_FROM_BIT(nframes); i++)
    {
        // A word equal to 0xFFFFFFFF means that all frames are used. No sense checking further
        if (frames[i] != 0xFFFFFFFF)
        {
            // at least one bit is free here.
            for (j = 0; j < 32; j++)
            {
                uint32_t toTest = (uint32_t)0x1 << j;
                if ( !(frames[i]&toTest) )
                {
                    number_free++;
                }
            }
        }
    }
    return number_free;
}

// Static function to find the first free frame.
uint32_t get_number_used_frames()
{
    uint32_t number_free = 0;
    uint32_t i, j;
    for (i = 0; i < INDEX_FROM_BIT(nframes); i++)
    {
        // A word equal to 0xFFFFFFFF means that all frames are used. No sense checking further
        if (frames[i] != 0xFFFFFFFF)
        {
            // at least one bit is free here.
            for (j = 0; j < 32; j++)
            {
                uint32_t toTest = (uint32_t)0x1 << j;
                if ( (frames[i]&toTest) )
                {
                    number_free++;
                }
            }
        } else {
            number_free += 32;
        }
    }
    return number_free;
}

// Static function to find the first free frame.
static uint32_t first_frame()
{
    uint32_t i, j;
    for (i = 0; i < INDEX_FROM_BIT(nframes); i++)
    {
        // A word equal to 0xFFFFFFFF means that all frames are used. No sense checking further
        if (frames[i] != 0xFFFFFFFF)
        {
            // at least one bit is free here.
            for (j = 0; j < 32; j++)
            {
                uint32_t toTest = (uint32_t)0x1 << j;
                if ( !(frames[i]&toTest) )
                {
                    return i*4*8+j;
                }
            }
        }
    }
    return uint32_t_MAX;
}

void page_set_frame(page_t *page, uint32_t frame)
{
    page->contents &= 0x00000FFF;
    page->contents |= (frame << 12);
}

uint32_t page_get_frame(page_t *page)
{
    return (page->contents & 0xFFFFF000) >> 12;
}

void page_set_present(page_t *page, uint32_t frame)
{
    if(frame){
        page->contents |= (0x1);
    } else {
        page->contents &= ~(0x1);
    }
}

uint32_t page_get_present(page_t *page)
{
    return (page->contents & 0x1);
}

void page_set_rw(page_t *page, uint32_t frame)
{
    if(frame){
        page->contents |= (0x2);
    } else {
        page->contents &= ~(0x2);
    }
}

uint32_t page_get_rw(page_t *page)
{
    return (page->contents & (0x2));

}

void page_set_user(page_t *page, uint32_t frame)
{
    if(frame){
        page->contents |= ((0x4));
    } else {
        page->contents &= ~((0x4));
    }

//    page->contents |= (0x4);
}

uint32_t page_get_user(page_t *page)
{
    return (page->contents & (0x4));

}

void page_set_accessed(page_t *page, uint32_t frame)
{
    if(frame){
        page->contents |= ((0x1u << 5));
    } else {
        page->contents &= ~((0x1u << 5));
    }


//    page->contents |= (0x20);
}

uint32_t page_get_accessed(page_t *page)
{
    return (page->contents & (0x1u << 5));

}

void page_set_dirty(page_t *page, uint32_t frame)
{
    if(frame){
        page->contents |= ((0x1u << 6));
    } else {
        page->contents &= ~((0x1u << 6));
    }


}

uint32_t page_get_dirty(page_t *page)
{
    return (page->contents & (0x1u << 6));

}

// Function to allocate a frame.
void alloc_frame(page_t *page, int is_kernel, int is_writeable)
{
    if (page_get_present(page))
    {
        return;
    }
    else
    {

        ASSERT(page_get_present(page) == 0);
        uint32_t idx = first_frame();
        if (idx == uint32_t_MAX)
        {
            PANIC("OUT OF MEMORY");
            // PANIC! no free frames!!
        }
        set_frame(idx*0x1000);

        page_set_present(page, 1);
        page_set_rw(page, (is_writeable)?1:0);
        page_set_user(page, (is_kernel)?0:1);
        page_set_frame(page, idx);
    }
}

// Function to deallocate a frame.
void free_frame(page_t *page)
{
    if(!page)
        return;

    uint32_t frame = page_get_frame(page);
    if (!page_get_present(page)) {
        return;
    } else {
        ASSERT(test_frame(frame*0x1000)); // Check we're freeing an allocated frame...
        clear_frame(frame*0x1000);
        page_set_present(page, 0);
        ASSERT(!test_frame(frame*0x1000)); // Check we're freeing an allocated frame...
    }
}

void initialise_paging()
{
    uint32_t mem_end_page = PHYSICAL_MEMORY_LIMIT;

    nframes = mem_end_page / 0x1000;
    frames = (uint32_t*)kmalloc(INDEX_FROM_BIT(nframes) * sizeof(uint32_t));
    memset(frames, 0, INDEX_FROM_BIT(nframes) * sizeof(uint32_t));

    // Let's make a page directory.
    kernel_directory = kmalloc_a(sizeof(page_directory_t));
    memset(kernel_directory, 0, sizeof(page_directory_t));
    kernel_directory->physicalAddr = (uint32_t)kernel_directory->tablesPhysical;
    current_directory = kernel_directory;

    src_copy_buffer = kmalloc_a(sizeof(char) * PAGE_SIZE);
    dest_copy_buffer = kmalloc_a(sizeof(char) * PAGE_SIZE);

    // JamesM's tutorial#7: We need to create some page tables before turning on virtual addressing and breaking kalloc
    uint32_t i;
    for(i = KHEAP_START; i < KHEAP_MAX; i += PAGE_SIZE){
         get_page(i, TRUE, kernel_directory);
    }

    // We need to identity map (phys addr = virt addr) from
    // 0x0 to the end of used memory, so we can access this
    // transparently, as if paging wasn't enabled.
    // NOTE that we use a while loop here deliberately.
    // inside the loop body we actually change placement_address
    // by calling kmalloc(). A while loop causes this to be
    // computed on-the-fly rather than once at the start.
    i = 0;
    // note: we need 1 more frame of memory because we're going to allocate a couple of things
    // when creating the kernel heap.
    while (i < placement_address + 5 * PAGE_SIZE)
    {
        // Kernel code is readable but not writeable from userspace.
        alloc_frame( get_page(i, 1, kernel_directory), 0, 0);
        i += 0x1000;
    }

    alloc_frame(get_page(KHEAP_MAX, 1, kernel_directory), FALSE, FALSE);

    // Also, while we're at it, we never allocated any of the frames in the kernel heap.
    for(i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SIZE; i += PAGE_SIZE) {
        alloc_frame(get_page(i, 1, kernel_directory), FALSE, FALSE);
    }

    // Before we enable paging, we must register our page fault handler.
    register_interrupt_handler(14, page_fault);

    // Now, enable paging!
    switch_page_directory(kernel_directory);

    kernel_heap = heap_init(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, KHEAP_MAX, FALSE, FALSE);
    kernel_heap->directory = kernel_directory;
    kernel_cleanup_stack = (uint32_t)kmalloc_a(KERNEL_STACK_SIZE);
    current_directory = clone_directory(kernel_directory);
    switch_page_directory(current_directory);
}

void switch_page_directory(page_directory_t *dir)
{
    current_directory = dir;
    asm volatile("mov %0, %%cr3":: "r"(dir->physicalAddr));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

page_t *get_page(uint32_t address, int make, page_directory_t *dir)
{
    // Turn the address into an index.
    address /= 0x1000;
    // Find the page table containing this address.
    uint32_t table_idx = address / 1024;
    if (dir->tables[table_idx]) // If this table is already assigned
    {
        return &dir->tables[table_idx]->pages[address%1024];
    }
    else if(make)
    {
        uint32_t tmp;
        dir->tables[table_idx] = (page_table_t*)kmalloc_ap(sizeof(page_table_t), &tmp);
        dir->tablesPhysical[table_idx] = tmp | 0x7; // PRESENT, RW, US.
        return &dir->tables[table_idx]->pages[address%1024];
    }
    else
    {
        return 0;
    }
}


void page_fault(registers_t *regs)
{
    // A page fault has occurred.
    // The faulting address is stored in the CR2 register.
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    // The error code gives us details of what happened.
    int present   = !(regs->err_code & 0x1); // Page not present
    int rw = regs->err_code & 0x2;           // Write operation?
    int us = regs->err_code & 0x4;           // Processor was in user-mode?
    int reserved = regs->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
    int id = regs->err_code & 0x10;          // Caused by an instruction fetch?

    // Output an error message.
    monitor_write("Page fault! ( ");
    if (present) {monitor_write("present ");}
    if (rw) {monitor_write("read-only ");}
    if (us) {monitor_write("user-mode ");}
    if (reserved) {monitor_write("reserved ");}
    if (id) {monitor_write("instructionfetch ");}
    monitor_write(") at 0x");
    monitor_write_hex(faulting_address);
    monitor_write("\n");
    PANIC("Page fault");
}

void copy_page(page_t *dest, page_t *src)
{
    // Basically just map the frames to a special section of kernel memory
    // then do a memcpy. Easy and we can keep virtual memory enabled.
    ASSERT(dest && src);
    ASSERT((uint32_t)dest_copy_buffer % PAGE_SIZE == 0);
    ASSERT((uint32_t)src_copy_buffer % PAGE_SIZE == 0);

    page_t *dest_copy_page = get_page((uint32_t)dest_copy_buffer, 1, kernel_directory);
    page_t *src_copy_page = get_page((uint32_t)src_copy_buffer, 1, kernel_directory);

    ASSERT(dest_copy_page && src_copy_page);

    page_set_frame(dest_copy_page, page_get_frame(dest));
    page_set_frame(src_copy_page, page_get_frame(src));

    // Flush the TLB. I assume there's a better way to do this, but I don't know how right now.
    uint32_t pagedir_addr;
    asm volatile("mov %%cr3, %0" : "=r" (pagedir_addr));
    asm volatile("mov %0, %%cr3" : : "r" (pagedir_addr));

    memcpy(dest_copy_buffer, src_copy_buffer, PAGE_SIZE);
}

page_table_t *copy_table(page_table_t *src, uint32_t *physical_addr)
{
    page_table_t *table = kmalloc_ap(sizeof(*table), physical_addr);
    memset(table, 0, sizeof(*table));
    ASSERT((uint32_t)table % PAGE_SIZE == 0);
    ASSERT(*physical_addr % PAGE_SIZE == 0);

    int i;
    for(i = 0; i < 1024; i++){
        if(!page_get_present(&src->pages[i])){
            continue;
        }

        alloc_frame(&table->pages[i], FALSE, FALSE);

        page_set_accessed(&table->pages[i], page_get_accessed(&src->pages[i] ));
        page_set_dirty(&table->pages[i], page_get_dirty(&src->pages[i] ));
        page_set_present(&table->pages[i], page_get_present(&src->pages[i] ) );
        page_set_rw(&table->pages[i], page_get_rw(&src->pages[i]));
        page_set_user(&table->pages[i], page_get_user(&src->pages[i] ));

        copy_page(&table->pages[i], &src->pages[i]);
    }

    return table;
}

uint32_t virtual_to_physical_address(uint32_t virtual_addr)
{
    return page_get_frame(get_page(virtual_addr, 0, current_directory)) * PAGE_SIZE + (virtual_addr & 0xFFF);
}

page_directory_t *clone_directory(page_directory_t *src)
{
    // CR3 needs physical addresses to our blanked out tables
    page_directory_t *directory = kmalloc_a(sizeof(*directory));
    memset((uint8_t*)directory, 0, sizeof(*directory));
    directory->physicalAddr = virtual_to_physical_address((uint32_t)directory->tablesPhysical);
    ASSERT((uint32_t)directory % PAGE_SIZE == 0);
    ASSERT(directory->physicalAddr % PAGE_SIZE == 0);

    int i;
    for(i = 0; i < 1024; i++){
        if(src->tables[i] == 0){
            continue;
        }

        // Idea: anything in both src and kernel_directory can be linked against
        if(kernel_directory->tables[i] == src->tables[i]){
            directory->tables[i] = src->tables[i];
            directory->tablesPhysical[i] = src->tablesPhysical[i];
        } else {
            ASSERT(!kernel_directory->tables[i]);
            // Not in both: copy instead of linking.
            uint32_t phys;
            directory->tables[i] = copy_table(src->tables[i], &phys);
            directory->tablesPhysical[i] = phys | 0x07; // Set Present, RW, user-mode bits
        }

    }
    return directory;
}
