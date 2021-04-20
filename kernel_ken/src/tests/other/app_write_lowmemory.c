#include "app.h"
#include "kernel_ken.h"

// This test writes to low memory addresses in the kernel. It should generate a page fault immediately at address 0x0 
// if the code is running in user mode and the page table is properly initialized. 

void my_app()
{
    print("Start test with pid = "); print_dec(getpid()); print("\n");

    unsigned int i;
    for(i = 0; ; i += sizeof(int)) {
        int *ptr = (int *)(i);
        *ptr = 10;
    }

    print("Test Done!\n");
}

