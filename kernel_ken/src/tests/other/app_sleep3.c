#include "app.h"
#include "kernel_ken.h"

// This test checks that the timing of the sleep() system call is still accurate even if the scheduler is 
// run 100's of times in a second. In this test, the child process should sleep for 10 seconds. The parent
// process should exit almost immediately.

void my_app()
{
    print("Running a test \n");

    if(fork() == 0){
        print("Process #"); print_dec(getpid()); print(" is sleeping for 10 seconds\n");
        sleep(10);
    } else {
	       int i;
        for(i = 0; i < 200; i++){
            yield();
        }
    }

    print("Process #"); print_dec(getpid()); print(" is done running \n");
}
