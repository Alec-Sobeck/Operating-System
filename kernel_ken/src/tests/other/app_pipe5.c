#include "app.h"
#include "kernel_ken.h"

// This test writes a small struct to a couple of pipes, to test if pipes work for something slightly
// more complicated than an integer. 

struct rational_number
{
    int numerator;
    int denominator;
};

void print_rational(struct rational_number *rational)
{
    print_dec(rational->numerator); print("/"); print_dec(rational->denominator);
}

void my_app()
{
    int sem = open_sem(0);
    int pipe1 = open_pipe();
    int pipe2 = open_pipe();

    if(fork() == 0){
        // Child must wait
        wait(sem);

        struct rational_number value;
        int ret = read(pipe1, &value, sizeof(struct rational_number));
        print("[Child] read value: "); print_rational(&value); print(" with return code: "); print_dec(ret); print("\n");

        ret = read(pipe1, &value, sizeof(struct rational_number));
        print("[Child] read value: "); print_rational(&value); print(" with return code: "); print_dec(ret); print("\n");

        ret = read(pipe2, &value, sizeof(struct rational_number));
        print("[Child] read value: "); print_rational(&value); print(" with return code: "); print_dec(ret); print("\n");

        ret = read(pipe2, &value, sizeof(struct rational_number));
        print("[Child] read value: "); print_rational(&value); print(" with return code: "); print_dec(ret); print("\n");

    } else {
        // Parent goes first

        struct rational_number value1 = {.numerator = 512, .denominator = 62};
        struct rational_number value2 = {.numerator = 999999, .denominator = 65};
        struct rational_number value3 = {.numerator = 2304982, .denominator = 908437};
        struct rational_number value4 = {.numerator = 8974215, .denominator = 26532};

        write(pipe1, &value1, sizeof(struct rational_number));
        write(pipe1, &value2, sizeof(struct rational_number));
        write(pipe2, &value3, sizeof(struct rational_number));
        write(pipe2, &value4, sizeof(struct rational_number));

        signal(sem);
    }

    print("Process #"); print_dec(getpid()); print(" is exiting\n");

}