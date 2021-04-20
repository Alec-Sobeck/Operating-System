#include <stdint.h>
#include "app.h"
#include "kernel_ken.h"
#include "ulib.h"
#include "syscall.h"

/// Convient macro to run a test in another process.
/// It waits until the process that started the test is dead, then it starts the next one
#define RUNTEST(fn, name) \
    int fn ## forkret = fork(); \
    if(fn ## forkret == 0) { \
        setpriority(getpid(), 7);\
        yield();\
        fn();\
        exit();\
    } else {\
        printf("Starting test: %s \n", name);\
        syscall_join_impl(fn ## forkret);\
        printf("Success: %s has exited \n", name);\
    }

//
uint32_t rand_internal_old(uint32_t *seed)
{
    return (*seed = *seed * 214013L + 2531011L);
}

/// My index starts as 1 page and should grow for every 1024 holes in memory
/// Thus, I create 1500 holes and make sure I don't get a page fault
/// (passing this test indicates memory was properly allocated to grow the index)
void test_grow_index()
{
    for(int i = 0; i < 1500; i++){
        alloc(4, 1);
    }
}

/// Create 2000 processes and close them. Typically, this has made any weird bugs in the paging code
/// or any memory leaks crash the kernel/emulator.
void test_exit_leaks()
{
    for(int i = 0; i < 2000; i++) {
        if (fork()) {
            yield();
        }  else {
            exit();
        }
    }
}

/// Just a basic test to make sure something allocated on the heap is copied to a new process created via a fork
void test_dircopy_1()
{
    fork();

    int *arr = alloc(sizeof(int) * 10, FALSE);
    for(int i = 0; i < 10; i++){
        arr[i] = i;
    }

    fork();

    for(int i = 0; i < 10; i++){
        assert(arr[i] == i);
    }

    yield();

    for(int i = 0; i < 10; i++){
        assert(arr[i] == i);
    }

    sleep(1);

    for(int i = 0; i < 10; i++){
        assert(arr[i] == i);
    }

}

/// Basic test for heap allocation and block merging on free.
void test_heap1()
{
    void *addr1 = alloc(PAGE_SIZE * 4.5, 0);
    void *addr2 = alloc(PAGE_SIZE * 3.5, 1);
    void *addr3 = alloc(PAGE_SIZE * 1, 0);
    void *addr4 = alloc(PAGE_SIZE * 2, 1);
    void *addr5 = alloc(PAGE_SIZE * 23.5, 0);
    void *addr6 = alloc(PAGE_SIZE * 12, 0);
    void *addr7 = alloc(PAGE_SIZE * 5, 1);
    void *addr8 = alloc(PAGE_SIZE * 2, 0);
    void *addr9 = alloc(PAGE_SIZE * 1, 1);
    void *addr10 = alloc(PAGE_SIZE * 1.5, 0);

    free(addr1);
    free(addr7);
    free(addr3);
    free(addr5);
    free(addr9);

    void *addr11 = alloc(18450, 1);

    free(addr6);
    free(addr4);
    free(addr10);
    free(addr2);
    free(addr8);
    free(addr11);

    void *final = alloc(50 * PAGE_SIZE, 0);

    assert((uint32_t)final == (uint32_t)addr1);
}

/// Test that writing to newly allocated user pages doesn't crash.
void test_write_big_heap()
{
    const int MEMSIZE = 10000000;
    uint8_t* a = alloc(MEMSIZE, 0);
    for(int i = 0; i < MEMSIZE; i++){
        a[i] = 0xEC;
    }
    free(a);
}

/// Simple test -
///  - Fills the pipe to capacity with numbers 1..N
///  - Signals child
///  - Child reads out the numbers in the order they were put in
///  - (Assertions added to automatically test that we stopped at the right points and read the right values)
void test_pipes1()
{
    int pipe = open_pipe();
    int sem = open_sem(0);
    int ret = fork();

    if(ret == 0){
        wait(sem);
        int val = 0xDEAD;
        for(int i = 0; ; i++){
            int retval = read(pipe, &val, sizeof(int));
            if(retval == 0) {
                assert(i == PIPE_BUFFER_SIZE / sizeof(int));
                break;
            }
            assert(val == i);
        }
    } else {
        for(int i = 0; i < 100000; i++){
            int retval = write(pipe, &i, sizeof(int));
            if(retval == 0){
                assert(i >= PIPE_BUFFER_SIZE / sizeof(int));
                break;
            }
        }

        signal(sem);
    }

}

/// Same idea as pipe1 but tests writing into the pipe that was filled then partially emptied.
void test_pipes2()
{
    int pipe = open_pipe();
    int child_wait_sem = open_sem(0);
    int parent_wait_sem = open_sem(0);
    int ret = fork();

    const int NUM_READ = 10000;

    if(ret == 0){
        wait(child_wait_sem);
        int val = 0xDEADBEEF;
        int k;
        for(k = 0; k < NUM_READ; k++){
            int retval = read(pipe, &val, sizeof(int));
            if(retval == 0) {
                assert(k == PIPE_BUFFER_SIZE / sizeof(int));
                break;
            }
            assert(val == k);
        }

        signal(parent_wait_sem);

        wait(child_wait_sem);
        for(int i = 0; ; k++, i++){
            int retval = read(pipe, &val, sizeof(int));
            if(retval == 0) {
                assert(k == PIPE_BUFFER_SIZE / sizeof(int) + NUM_READ);
                break;
            }
            assert(val == k);
        }

    } else {
        int j;
        for(j = 0; j < 100000; j++){
            int retval = write(pipe, &j, sizeof(int));
            if(retval == 0){
                assert(j >= PIPE_BUFFER_SIZE / sizeof(int));
                break;
            }
        }

        signal(child_wait_sem);

        wait(parent_wait_sem);

        int expected_j = j + NUM_READ;
        for(; j < 100000; j++){
            int retval = write(pipe, &j, sizeof(int));
            if(retval == 0){
                assert(j >= PIPE_BUFFER_SIZE / sizeof(int));
                assert(j == expected_j);
                break;
            }
        }
        signal(child_wait_sem);
    }

}

/// Parent opens pipe, writes a bunch of stuff to it, signals the semaphore, then closes the semaphore.
/// -- this should cause the child to wake up but with an error return code on the semaphore because
/// it was closed while the child was still waiting on it.
/// -- also the child will try to close the semaphore again. It should error again.
void test_pipes_close1()
{
    int pipe = open_pipe();
    int sem = open_sem(0);
    int ret = fork();

    if(ret == 0){
        // Make sure we run second
        setpriority(getpid(), 6);
        yield();
        int sem_ret = wait(sem);
        assert(sem_ret == 0); // Sem was closed - not a normal wakeup. Rather, this is an error

        int tmp;
        assert(-1 == read(pipe, &tmp, sizeof(int))); // Should also error. Pipe is closed.

    } else {

        setpriority(getpid(), 1);
        for(int i = 0; i < 100000; i++){
            int retval = write(pipe, &i, sizeof(int));
            if(retval == 0){
                assert(i >= PIPE_BUFFER_SIZE / sizeof(int));
                break;
            }
        }

        signal(sem);
    }
    // Test sem return codes - one should be an error
    int sem_ret = close_sem(sem);
    if(ret == 0)
        assert(sem_ret == 0);
    else
        assert(sem_ret == sem);

    // Test pipe return codes - one should be an error
    int pipe_ret = close_pipe(pipe);
    if(ret == 0){
        assert(pipe_ret == -1);
    } else {
        assert(pipe_ret == pipe);
    }


}

void add_num_backoff(int pipe, void *buff, uint32_t len, uint32_t num_fails)
{
    // Exponential backoff because our stack is only like 8K
    int ret = write(pipe, buff, len);
    if(ret == 0){
        //printf("Sleeping for %d sec \n", 3 * (num_fails + 1));
        sleep(3 * (num_fails + 1));
        add_num_backoff(pipe, buff, len, num_fails + 1);
    }
}

/// Producer consumer with 1 P, 1 C thread
#define PC1_NUM_ITER 50000
void test_pc1()
{
    int pipe = open_pipe();
    int sem = open_sem(0);
    uint32_t seed = 1;
    int ret = fork();

    if(ret == 0){
        int i = 0;
        while(i < PC1_NUM_ITER){
            assert(wait(sem) == sem);
            int value = 0xEFFF;
            assert(read(pipe, &value, sizeof(int)) == sizeof(int));
            uint32_t next = rand_internal_old(&seed);
            assert(next == value);
            i++;
        }
    } else {
        int i = 0;
        while(i < PC1_NUM_ITER) {
            uint32_t next = rand_internal_old(&seed);
            add_num_backoff(pipe, &next, sizeof(uint32_t), 0);
            assert(signal(sem) == sem);
            i++;
        }
    }
}

/// Producer consumer with 5 P, 1 C thread
#define PC2_NUM_ITER 50000
void test_pc2()
{
    int pipe = open_pipe();
    int sem = open_sem(0);

    int ret = fork();

    const int BUFF_SIZE = 8;
    const int PRODUCER_COUNT = 5;

    if(ret == 0){
        // Seeds for every process, so we can automatically check nums later
        uint32_t lots_of_space = (PRODUCER_COUNT + 5 + (uint32_t)getpid());
        uint32_t *seeds = alloc(sizeof(uint32_t) * lots_of_space, 0);
        for(int i = 0; i < lots_of_space; i++){
            seeds[i] = (uint32_t)i;
        }

        assert(getpid() + PRODUCER_COUNT < lots_of_space);
        int i = 0;
        while(i < PRODUCER_COUNT * PC2_NUM_ITER){
            assert(wait(sem) == sem);

            char buff[BUFF_SIZE];
            assert(read(pipe, buff, BUFF_SIZE) == BUFF_SIZE);

            uint32_t *buffer_cast = (uint32_t *)buff;
            int pid = (int)buffer_cast[0];
            uint32_t value = buffer_cast[1];


            uint32_t next = rand_internal_old(seeds + pid);
            assert(next == value);
            i++;
        }
    } else {
        // Create N - 1 producers (we already have 1)
        for(int i = 0; i < (PRODUCER_COUNT - 1); i++){
            if(fork() == 0) break;
        }

        uint32_t seed = (uint32_t)getpid();
        int i = 0;
        while(i < PC2_NUM_ITER) {
            uint32_t next = rand_internal_old(&seed);

            char buff[BUFF_SIZE];
            uint32_t *buffer_cast = (uint32_t *)buff;
            buffer_cast[0] = (uint32_t)getpid();
            buffer_cast[1] = next;

            add_num_backoff(pipe, buff, BUFF_SIZE, 0);

            assert(signal(sem) == sem);
            i++;

        }
    }

}

/// Producer consumer with 5 P, 1 C threads. This version doesn't make use of any semaphores.
#define PC3_NUM_ITER 20000
void test_pc3()
{
    int pipe = open_pipe();

    int ret = fork();

    const int BUFF_SIZE = 8;
    const int PRODUCER_COUNT = 5;

    if(ret == 0){
        // Seeds for every process, so we can automatically check nums later
        uint32_t lots_of_space = (PRODUCER_COUNT + 5 + (uint32_t)getpid());
        uint32_t *seeds = alloc(sizeof(uint32_t) * lots_of_space, 0);
        for(int i = 0; i < lots_of_space; i++){
            seeds[i] = (uint32_t)i;
        }

        assert(getpid() + PRODUCER_COUNT < lots_of_space);
        int i = 0;
        while(i < PRODUCER_COUNT * PC3_NUM_ITER){

            char buff[BUFF_SIZE];
            while(read(pipe, buff, BUFF_SIZE) == 0); // No semaphore

            uint32_t *buffer_cast = (uint32_t *)buff;
            int pid = (int)buffer_cast[0];
            uint32_t value = buffer_cast[1];


            uint32_t next = rand_internal_old(seeds + pid);
            assert(next == value);
            i++;
        }
    } else {
        // Create N - 1 producers (we already have 1)
        for(int i = 0; i < (PRODUCER_COUNT - 1); i++){
            if(fork() == 0) break;
        }

        uint32_t seed = (uint32_t)getpid();
        int i = 0;
        while(i < PC3_NUM_ITER) {
            uint32_t next = rand_internal_old(&seed);

            char buff[BUFF_SIZE];
            uint32_t *buffer_cast = (uint32_t *)buff;
            buffer_cast[0] = (uint32_t)getpid();
            buffer_cast[1] = next;

            add_num_backoff(pipe, buff, BUFF_SIZE, 0);

            i++;

        }
    }

}

/// Basic Semaphore test - make sure things signal correctly
/// and waiting on/closing an already closed semaphore is an error
void test_sem1()
{
    int sem = open_sem(0);
    int ret = fork();

    if(ret == 0){
        // child
        for(int i = 1; i <= 10; i++){
            int sem_ret = wait(sem);
            assert((i <= 5) ? (sem_ret == sem) : (sem_ret == 0));
        }
        int sem_close = close_sem(sem);
        assert(sem_close == 0);
    } else {
        // parent
        for(int i = 0; i < 5 ; i++){
            sleep(1);
            assert(signal(sem) == sem);
        }
        int sem_close = close_sem(sem);
        assert(sem_close == sem);
    }
}

/// Parent opens pipe, writes a bunch of stuff to it, signals the semaphore, then closes the semaphore.
/// -- this should cause the child to wake up but with an error return code on the semaphore because
/// it was closed while the child was still waiting on it.
/// -- also the child will try to close the semaphore again. It should error again.
void test_sem_close1()
{
    int pipe = open_pipe();
    int sem = open_sem(0);
    int ret = fork();

    if(ret == 0){
        // Make sure we run second
        setpriority(getpid(), 6);
        yield();
        int sem_ret = wait(sem);
        assert(sem_ret == 0); // Sem was closed - not a normal wakeup. Rather, this is an error
    } else {

        setpriority(getpid(), 1);
        for(int i = 0; i < 100000; i++){
            int retval = write(pipe, &i, sizeof(int));
            if(retval == 0){
                assert(i >= PIPE_BUFFER_SIZE / sizeof(int));
                break;
            }
        }

        signal(sem);
    }
    int sem_ret = close_sem(sem);
    if(ret == 0)
        assert(sem_ret == 0);
    else
        assert(sem_ret == sem);

}

void run_tests()
{

    RUNTEST(test_grow_index, "Grow Heap Index");
    RUNTEST(test_exit_leaks, "Exit() Memory Leaks");
    RUNTEST(test_dircopy_1, "Basic Page Directory Copy #1");
    RUNTEST(test_heap1, "Basic Heap Test #1");
    RUNTEST(test_write_big_heap, "User Heap Expansion");
    RUNTEST(test_pipes1, "Test Pipes #1");
    RUNTEST(test_pipes2, "Test Pipes #2");
    RUNTEST(test_pipes_close1, "Test Resource Close #1");
    RUNTEST(test_pc1, "Producer-Consumer #1");
    RUNTEST(test_sem1, "Sem Test #1");
    RUNTEST(test_sem_close1, "Sem Close #1");
#ifdef STRESS_TEST
    RUNTEST(test_merging, "Merge Sort #1");
#endif
    RUNTEST(test_pc3, "Producer-Consumer #3");
    RUNTEST(test_pc2, "Producer-Consumer #2");

    printf("All tests done!\n");

}

