#include "app.h"
#include "kernel_ken.h"

// For this test, I create 4 processes, each with different priorities. If aging works correctly, 
// processes 3, 4, and 5 should get to run on the CPU once in a while. Since the priorities of 
// the processes are such that (Priority for Process 2) > (Priority for process 3) > (Priority for process 4) 
// > (Priority for process 5), I would expect an output where process 3 runs about twice as often as process 4, 
// and about triple as much as process 5. For example, my kernel outputs the following:
// Proc 3 ran
// Proc 3 ran
// Proc 4 ran
// Proc 3 ran
// Proc 5 ran
// Proc 3 ran
// Proc 4 ran
// Proc 3 ran
// Proc 3 ran
// Proc 5 ran
// Proc 4 ran

void my_app()
{
    print("Running user program \n");
 
    int parent_id = getpid();

    // Create 4 total processes
    fork();
    fork();

    // Change process priorities so that they're all different.
    if(getpid() == 2){
        setpriority(getpid(), 3);
    }
    if(getpid() == 3){
        setpriority(getpid(), 4);
    }
    if(getpid() == 4){
        setpriority(getpid(), 5);
    }
    if(getpid() == 5){
        setpriority(getpid(), 6);
    }

    int count = 0;
    while(1){
        count++;
        // This if statement is to reduce the number of lines written to the console, so it's possible to actually read them
        // without it, we'd be writting over 100 lines/second, which is too fast to read.
        if(count % 20000 == 0 && getpid() != parent_id){
            print("Proc "); print_dec(getpid()); print(" ran \n");
        }
        yield();
    }

    print("Proc #"); print_dec(getpid()); print(" is done running \n");
}
