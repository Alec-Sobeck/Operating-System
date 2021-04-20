#include "main.h"
#include "common.h"
#include "kernel_ken.h"
#include "app.h"

uint32_t initial_esp;
int main(struct multiboot *mboot_ptr, uint32_t initial_stack)
{
    initial_esp = initial_stack;
    initialise_tasking();
    my_app();
    // A few things to note: the first process, the one with pid = 1, is stuck in an infinite loop
    // and it has priority 11. It's kind of just there so the scheduler has something to do if it runs out of jobs.
    // It'll never actually make it here, so we don't have to worry about that really.
    //
    // If the user_app() function calls exit() then it'll also never make it here.
    // This is basically just incase the user forgets to call exit()
    exit();
    return 0;
}
