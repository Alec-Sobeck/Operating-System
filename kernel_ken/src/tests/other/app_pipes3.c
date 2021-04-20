#include "app.h"
#include "kernel_ken.h"

// This test makes sure the cyclic buffer in pipes works correctly. It also tests that the pipe write/read
// are non-blocking by relying on those system calls returning 0 when the pipe is full or empty, respectively.

// It does this by writing some values to a pipe in the parent, then reading some of the values in the pipe
// in the child process, then writing some more values to the pipe in the parent, the reading all the values
// from the pipe in the child.

#define PIPE_BUFFER_SIZE 65536      // Change to the actual buffer size in the kernel...

#define assert(b) ((b) ? (void)0 : u_assert(__FILE__, __LINE__, #b))
void u_assert(const char *file, unsigned int line, const char *desc)
{
    // An assertion failed, kill the user process, I suppose.
    print("USER ASSERTION FAILED (");
    print(desc);
    print(") at ");
    print(file);
    print(": ");
    print_dec(line);
    print("\n");
    for(;;);
}

void my_app()
{
    print("Starting test\n");

    int pipe = open_pipe();
    int child_wait_sem = open_sem(0);
    int parent_wait_sem = open_sem(0);
    int ret = fork();

    const int NUMBER_VALUES_READ = 2;
    if(ret == 0){
        // Second: after parent fills up the pipe, the child reads some values from it.
        wait(child_wait_sem);
        print("Child starting first read \n");
        int val = 0xDEADBEEF;
        int k;
        for(k = 0; k < NUMBER_VALUES_READ; k++){
            int retval = read(pipe, &val, sizeof(int));
            print("[Child] Read the following value: "); print_dec(val); print("; return code: "); print_dec(retval); print("\n");
            // Use asserts to check that the read failed after reading the correct number of values.
            if(retval == 0) {
                assert(k == PIPE_BUFFER_SIZE / sizeof(int));
                break;
            }
            assert(val == k);
        }

        signal(parent_wait_sem);

        // Finally: when the parent is done, the child reads all the values from the pipe
        wait(child_wait_sem);
        print("Child starting second read \n");
        int i;
        for(i = 0; ; k++, i++){
            int retval = read(pipe, &val, sizeof(int));
            print("[Child] Read the following value: "); print_dec(val); print("; return code: "); print_dec(retval); print("\n");
            // Use asserts to check that the read failed after reading the correct number of values.
            if(retval == 0) {
                assert(k == PIPE_BUFFER_SIZE / sizeof(int) + NUMBER_VALUES_READ);
                break;
            }
            assert(val == k);
        }

    } else {
        // First: parent writes values to the pipe until it is full
        int j;
        print("Parent starting first write \n");
        for(j = 0; j < 100000; j++){
            int retval = write(pipe, &j, sizeof(int));
            print("[Parent] Write the following value: "); print_dec(j); print("; return code: "); print_dec(retval); print("\n");
            // Use asserts to check that the write failed after writing the correct number of values.
            if(retval == 0){
                assert(j >= PIPE_BUFFER_SIZE / sizeof(int));
                break;
            }
        }

        signal(child_wait_sem);

        // Third: The parent writes some more values to the pipe (the pipe is still partially full from before).
        wait(parent_wait_sem);
        print("Parent starting second write \n");
        int expected_j = j + NUMBER_VALUES_READ;
        for(; j < 100000; j++){
            int retval = write(pipe, &j, sizeof(int));
            print("[Parent] Write the following value: "); print_dec(j); print("; return code: "); print_dec(retval); print("\n");
            // Use asserts to check that the write failed after writing the correct number of values.
            if(retval == 0){
                assert(j >= PIPE_BUFFER_SIZE / sizeof(int));
                assert(j == expected_j);
                break;
            }
        }
        signal(child_wait_sem);
    }

    print("Process #"); print_dec(getpid()); print(" is done\n");



}

