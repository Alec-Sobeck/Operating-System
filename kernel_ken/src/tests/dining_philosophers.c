#include "app.h"
#include "kernel_ken.h"

// This is a standard implementation of the dining philosophers problem that will not deadlock.
// It's a good overall test to make sure semaphores are implemented correctly because it uses all the system calls except close()

#define NUM_PHILOSOPHERS 5

unsigned int rand(unsigned int *seed)
{
    return (((*seed = *seed * 214013L + 2531011L) >> 16) & 0x7fff);
}

int pickup_forks(int *forks, int pnum)
{
    if(pnum % 2 == 0) {
        wait(forks[(pnum + 1) % NUM_PHILOSOPHERS]);
        wait(forks[(pnum) % NUM_PHILOSOPHERS]);
    } else {
        wait(forks[(pnum) % NUM_PHILOSOPHERS]);
        wait(forks[(pnum + 1) % NUM_PHILOSOPHERS]);
    }
    return 0;
}

int return_forks(int *forks, int pnum)
{
    signal(forks[(pnum) % NUM_PHILOSOPHERS]);
    signal(forks[(pnum + 1) % NUM_PHILOSOPHERS]);
    return 0;
}

void my_app()
{
    int i;
    print("First threadid = "); print_dec(getpid()); print("\n");


    // Note: this only works assuming pids are sequential and the parent has the lowest pid
    int parentpid = getpid();

    int forks[NUM_PHILOSOPHERS];
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        forks[i] = open_sem(1);
    }

    // The parent counts as one of the threads, so fork 1 less time then NUM_PHIL.
    for(i = 0; i < NUM_PHILOSOPHERS - 1; i++){
        if(fork() == 0) {
            break;
        }
    }

    int id = getpid() - parentpid;
    unsigned int seed = (unsigned int) getpid();
    while (1) {
        // Think
        int rng = rand(&seed) % 3 + 1;
        print("Philosopher thread "); print_dec(id); print(" is thinking for "); print_dec(rng); print(" seconds. \n");
        sleep(rng);
        // Pickup forks
        pickup_forks(forks, id);
        // Eat
        rng = rand(&seed) % 3 + 1;
        print("Philosopher "); print_dec(id); print(" is eating for "); print_dec(rng); print(" seconds \n");
        sleep(rng);
        // Return forks
        return_forks(forks, id);
    }


}

