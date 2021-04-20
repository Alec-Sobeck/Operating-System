#include "app.h"
#include "kernel_ken.h"

// This test checks that a process cannot access a pipe/semaphore that has been closed without getting an error code. 

#define PIPE_BUFFER_SIZE 65536  

void my_app()
{
    int pipe = open_pipe();
    int sem = open_sem(0);
    print("Pipe id: "); print_dec(pipe); print("\n");
    print("Sem id: "); print_dec(sem); print("\n");

    int ret = fork();
    if(ret == 0){
        // Make sure we run second
        int sem_ret = wait(sem);
        // Sem was closed - not a normal wakeup. Rather, this is an error
        print("Child return code for wait(): "); print_dec(sem_ret); print("\n");

        // Should also error. Pipe is closed.
        int tmp;
        int pipe_ret = read(pipe, &tmp, sizeof(int));
        print("Child return code for read(): "); print_dec(pipe_ret); print("\n");

    } else {
	     int i;
        for(i = 0; i < (PIPE_BUFFER_SIZE / sizeof(int)); i++){
            write(pipe, &i, sizeof(int));
        }
        signal(sem);
    }

    // Test sem return codes - one should be an error
    int close_ret = close_sem(sem);
    if(ret == 0) {
        print("Child return code for sem close(): "); print_dec(close_ret); print("\n");
    } else {
        print("Parent return code for sem close(): "); print_dec(close_ret); print("\n");
    }

    // Test pipe return codes - one should be an error
    int pipe_ret = close_pipe(pipe);
    if(ret == 0) {
        print("Child return code for pipe close(): "); print_dec(pipe_ret); print("\n");
    } else {
        print("Parent return code for pipe close(): "); print_dec(pipe_ret); print("\n");
    }

}
