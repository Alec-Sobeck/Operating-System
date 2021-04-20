// kheap.c -- Kernel heap functions, also provides
//            a placement malloc() for use before the heap is 
//            initialised.
//            Written for JamesM's kernel development tutorials.

#include "kheap.h"
#include "klib.h"

// end is defined in the linker script.
extern uint32_t end;
extern page_directory_t *kernel_directory;

heap_t *kernel_heap = NULL;
uint32_t placement_address = (uint32_t)&end;

void *kmalloc_int(uint32_t sz, int align, uint32_t *phys)
{
    // Note: due to a significant number of bugs involving memory that should've been zeroed out but wasn't,
    // ALL kernel allocations now zero out their blocks before returning them.
    //
    if(kernel_heap){
        void *address = heap_alloc(kernel_heap, sz, align);
        if(phys){
            // According to JamesM's tutorials, we can get a physical address with get_page like this:
            page_t *page = get_page((uint32_t)address, FALSE, kernel_directory);
            *phys = page_get_frame(page) * PAGE_SIZE + ((uint32_t)address & 0xFFF);
        }
        memset(address, 0, sz);
        return address;
    } else {
        if (align == 1 && (placement_address % PAGE_SIZE != 0) ) {
            // Align the placement address;
            placement_address &= 0xFFFFF000;
            placement_address += 0x1000;
        }
        if (phys) {
            *phys = placement_address;
        }
        uint32_t tmp = placement_address;
        placement_address += sz;
        memset((void*)tmp, 0, sz);
        return (void*)tmp;
    }
}

void *kmalloc_a(uint32_t sz)
{
    return kmalloc_int(sz, 1, 0);
}

void *kmalloc_p(uint32_t sz, uint32_t *phys)
{
    return kmalloc_int(sz, 0, phys);
}

void *kmalloc_ap(uint32_t sz, uint32_t *phys)
{
    return kmalloc_int(sz, 1, phys);
}

void *kmalloc(uint32_t sz)
{
    return kmalloc_int(sz, 0, 0);
}

void kfree(void *p)
{
    heap_free(kernel_heap, p);
}

