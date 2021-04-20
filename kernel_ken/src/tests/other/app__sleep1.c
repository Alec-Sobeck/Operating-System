#include "app.h"
#include "kernel_ken.h"

// For this test, I first fork a new process. Then I have the child call sleep(1) 10 times, and the parent call sleep(3) 10 times.
// Therefore, the child should exit after 10 seconds, and the parent after 30 seconds. 
void my_app()
{
    print("Running a test \n");

    int i;
    int ret = fork();
    for(i = 0; i < 10; i++){
        print("Process #"); print_dec(getpid()); print(" is sleeping\n");
        if(ret == 0){
            sleep(1);
        } else {
            sleep(3);
        }
    }

    print("Process #"); print_dec(getpid()); print(" is done running \n");
}

