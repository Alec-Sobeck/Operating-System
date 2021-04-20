#include "app.h"
#include "kernel_ken.h"

// This test repeats the following 2000 times: create a process, give that process the CPU, then cause that newly created process to exit.
// This sequence of actions is deliberately chosen because it means that there should only ever be 2-3 processes in the system at any instant
// in time. This prevents the system from hitting the MAX_PROCESSES limit or running out of memory due to allocating too many processes.  

// The main purpose of this test is to identify any memory leaks in exit(). If the OS isn't freeing process resources correctly, 
// it will probably run out of memory at some point (in my kernel, this causes kernel panic because we exhaust 100% of the memory 
// in the system). It can also reveal other memory related errors such as paging bugs because we're doing a lot of kernel work.

void my_app()
{
    print("Starting test\n");

    int i;
    // Create 100 processes, then cause them to exit. This is mainly to check for memory leaks.
    for(i = 0; i < 2000; i++) {
        if (fork()) {
            // Parent: create another process, then yield to that process
            yield();
        }  else {
            // Child: Immediately exit
            exit();
        }
    }

    print("Parent has exited \n");

}

