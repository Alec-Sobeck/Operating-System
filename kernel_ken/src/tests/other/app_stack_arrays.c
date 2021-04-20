#include "app.h"
#include "kernel_ken.h"

// This is the same test as app_heap_arrays.c except the array used is allocated on the stack, not the heap

void my_app()
{
    int arr[10];
    int ret = fork();
    int i;

    // Fill parent array with 0->9, fill child array with 9->0
    if(ret == 0){
        for(i = 0; i < 10; i++){
            arr[i] = 9-i;
        }
    } else {
        for(i = 0; i < 10; i++){
            arr[i] = i;
        }
    }
    yield();

    // Print array contents
    print("Process #"); print_dec(getpid()); print(" has array: ");
    for(i = 0; i < 10; i++){
        print_dec(arr[i]); print(" ");
    }
    print("\n");
    yield();

    // Overwrite a couple of values
    if(ret == 0){
        arr[7] = 42;
    } else {
        arr[2] = 9001;
    }
    yield();

    // Print arrays again
    print("Process #"); print_dec(getpid()); print(" has array: ");
    for(i = 0; i < 10; i++){
        print_dec(arr[i]); print(" ");
    }
    print("\n");

}

