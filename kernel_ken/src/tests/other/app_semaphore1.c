#include "app.h"
#include "kernel_ken.h"

// In this test, two processes take turns running using the wait()/signal() system calls. The parent runs first, 
// then the child, the parent, etc. After taking turns five times, the parent closes the child semaphore while
// the child is still waiting on it. I would expect this to wake the child process up and return an error code
// (or do something else sensible; kernel_ken.h leaves this implementation detail up to us). Any subsequent 
// calls to signal/wait/close on the closed semaphore should also return an error code of 0. I would expect an output like this:

// [Parent Process] Child Semaphore ID: 2; Signal returned: 2
// [Child Process] Child Semaphore ID: 2; Wait returned: 2
// [Parent Process] Child Semaphore ID: 2; Signal returned: 2
// [Child Process] Child Semaphore ID: 2; Wait returned: 2
// [Parent Process] Child Semaphore ID: 2; Signal returned: 2
// [Child Process] Child Semaphore ID: 2; Wait returned: 2
// [Parent Process] Child Semaphore ID: 2; Signal returned: 2
// [Child Process] Child Semaphore ID: 2; Wait returned: 2
// [Parent Process] Child Semaphore ID: 2; Signal returned: 2
// [Parent Process] Close returned: 2
// [Child Process] Child Semaphore ID: 2; Signal returned: 0
// [Child Process] Child Semaphore ID: 2; Signal returned: 0
// [Child Process] Child Semaphore ID: 2; Signal returned: 0
// [Child Process] Child Semaphore ID: 2; Signal returned: 0
// [Child Process] Child Semaphore ID: 2; Signal returned: 0
// [Child Process] Child Semaphore ID: 2; Signal returned: 0
// [Child Process] Close returned: 0

void my_app()
{
    int parent_sem = open_sem(1);
    int child_sem = open_sem(0);
    int ret = fork();

    // The two processes take turns running, by setting the other's semaphore.
    // The first process to run is the parent, then the child will get to run, etc
    // After taking turns roughly 5 times, the parent closes the child_sem - from this point on, any system calls involving
    //  that semaphore should return 0 [error code]
    if(ret == 0){
        // Child
        int i;
        for(i = 0; i < 10; i++){
            int wait_retval = wait(child_sem);
            print("[Child Process] Child Semaphore ID: "); print_dec(child_sem); print("; Wait returned: "); print_dec(wait_retval); print("\n");
            signal(parent_sem);
        }
        int close_retval = close_sem(child_sem);
        print("[Child Process] Close returned: "); print_dec(close_retval); print("\n");
    } else {
        // Parent
        int i;
        for(i = 0; i < 5 ; i++){
            wait(parent_sem);
            int signal_retval = signal(child_sem);
            print("[Parent Process] Child Semaphore ID: "); print_dec(child_sem); print("; Signal returned: "); print_dec(signal_retval); print("\n");
        }
        int sem_close = close_sem(child_sem);
        print("[Parent Process] Close returned:"); print_dec(sem_close); print("\n");
    }
}

