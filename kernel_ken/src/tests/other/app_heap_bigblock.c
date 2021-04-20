#include "app.h"
#include "kernel_ken.h"

// This allocates a big block of memory on the user heap and writes some values to it. This is to test
// that the heap is able to grow  if necessary.

void my_app()
{
    const int MEMSIZE = 10000000;
    unsigned char* a = alloc(MEMSIZE, 0);
    print("Allocated a memory block at: "); print_hex((unsigned int)a); print("\n");
    unsigned int i;
    for(i = 0; i < MEMSIZE; i++){
        a[i] = 0xEC;
    }
    free(a);

    print("Test finished \n");
}

