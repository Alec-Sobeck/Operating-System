Project for CS6605 - Advanced Operating Systems

The term project for this course is to write an operating system kernel from scratch. This particular operating system runs on qemu (i386 emulator).

The kernel API includes: 
* Functions to print to the screen
* A heap to dynamically allocate memory
* Threading primatives such as semaphores
* Multiprocess support using the fork and join threading model
* Interprocess communication via pipes 

In addition, various example programs are included such as:
* The dining philosphers problem
* A maze solver that uses A* 
* A blocking and non-blocking producer-consumer problem

This kernel is partially based on James Molloy's tutorial (http://www.jamesmolloy.co.uk/tutorial_html/)



