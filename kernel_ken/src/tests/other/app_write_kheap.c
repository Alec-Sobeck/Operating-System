#include "app.h"
#include "kernel_ken.h"

// This test writes to an arbitrary address in the kernel heap. It should generate a page fault if the
// code is running in user mode and the page table is properly initialized. 

void my_app()
{
    print("Starting test with pid = "); print_dec(getpid()); print("\n");

    int *ptr = (int *)(0xC0000050U);
    *ptr = 10;

    print("Test Done!\n");
}


