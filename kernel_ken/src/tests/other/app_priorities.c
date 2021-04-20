#include "app.h"
#include "kernel_ken.h"

// This test calls setpriority() a bunch of times with different values to test the edge cases.
// Expected output is the following:
// Returned: 5
// Returned: 0
// Returned: 1
// Returned: 0
// Returned: 10
// Returned: 0
// Returned: 11 (<-- Something that isn't 3)
// Returned: 0
// Returned: 0
// Process #2 is exiting

void my_app()
{
    // Set priority of the process to 5 before we start
    print("Returned: "); print_dec(setpriority(getpid(), 5)); print("\n");

    // For this test only: this is actually an intended line that is helpful.
    if(fork() != 0){
        for(;;);
    }

    print("Returned: "); print_dec(setpriority(getpid(), -1)); print("\n");
    print("Returned: "); print_dec(setpriority(getpid(), 1)); print("\n");
    print("Returned: "); print_dec(setpriority(getpid(), 0)); print("\n");
    print("Returned: "); print_dec(setpriority(getpid(), 10)); print("\n");
    print("Returned: "); print_dec(setpriority(getpid(), 11)); print("\n");
    print("Returned: "); print_dec(setpriority(1, 3)); print("\n");
    print("Returned: "); print_dec(setpriority(0, 2)); print("\n");
    print("Returned: "); print_dec(setpriority(4, 2)); print("\n");

    print("Process #"); print_dec(getpid()); print(" is exiting\n");
}
