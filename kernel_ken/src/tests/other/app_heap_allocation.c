#include "app.h"
#include "kernel_ken.h"


// This tests that the heap is able to allocate and free blocks correctly. It also tests that blocks are correctly merged in the free list. 
// If the test succeeds, the same memory address should be printed out twice (once for the first allocation in the test program, and
// once for the last allocation). This is because the final block can only be placed at the start of the heap if all blocks were merged 
// properly; otherwise there isn't enough space to put it at the start. 

#define assert(b) ((b) ? (void)0 : u_assert(__FILE__, __LINE__, #b))
void u_assert(const char *file, unsigned int line, const char *desc) 
{
    // An assertion failed, kill the user process, I suppose.
    print("USER ASSERTION FAILED ("); 
    print(desc);
    print(") at "); 
    print(file);
    print(": ");
    print_dec(line);
    print("\n");
    for(;;);
}

#define PAGE_SIZE 4096
void my_app()
{
    void *addr1 = alloc(PAGE_SIZE * 4.5, 0);
    void *addr2 = alloc(PAGE_SIZE * 3.5, 1);
    void *addr3 = alloc(PAGE_SIZE * 1, 0);
    void *addr4 = alloc(PAGE_SIZE * 2, 1);
    void *addr5 = alloc(PAGE_SIZE * 23.5, 0);
    void *addr6 = alloc(PAGE_SIZE * 12, 0);
    void *addr7 = alloc(PAGE_SIZE * 5, 1);
    void *addr8 = alloc(PAGE_SIZE * 2, 0);
    void *addr9 = alloc(PAGE_SIZE * 1, 1);
    void *addr10 = alloc(PAGE_SIZE * 1.5, 0);

    free(addr1);
    free(addr7);
    free(addr3);
    free(addr5);
    free(addr9);

    void *addr11 = alloc(18450, 1);

    free(addr6);
    free(addr4);
    free(addr10);
    free(addr2);
    free(addr8);
    free(addr11);

    void *final = alloc(50 * PAGE_SIZE, 0);

    // Print the addresses. This should return addresses that are in the user heap.
    print_hex((unsigned int)final);
    print("\n");
    print_hex((unsigned int)addr1);
    print("\n");

    // If all blocks were freed correctly, "final" should be allocated at the same address as "addr1"
    // In addition, final and addr1 should be non-null (this process is only requesting ~250K of memory on the heap, 
    // which is more than reasonable - so alloc() shouldn't be failing)
    assert((unsigned int)final == (unsigned int)addr1);
    assert(final != 0);
    assert(addr1 != 0);
}
