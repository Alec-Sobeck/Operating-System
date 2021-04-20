// common.h -- Defines typedefs and some global functions.
//             From JamesM's kernel development tutorials.

#ifndef COMMON_H
#define COMMON_H

#include "stdint.h"

#define PAGE_SIZE 0x1000
#define WORD_SIZE 4
#define TRUE 1
#define FALSE 0
#define NULL 0
#define uint32_t_MAX 4294967295


//#define DEBUG_MEMORY
//#define STRESS_TEST

#ifndef STRESS_TEST
    #define PHYSICAL_MEMORY_LIMIT   16000000
#else
    #define PHYSICAL_MEMORY_LIMIT   1600000000
#endif

#define PIPE_BUFFER_SIZE 65536

#define UHEAP_START             0x30000000
#define UHEAP_INITIAL_SIZE      0xA000
#define UHEAP_MAX               0x9FFFFFFC

#define KHEAP_START             0xC0000000
#define KHEAP_INITIAL_SIZE      0xA000
#define KHEAP_MAX               0xCFFFFFFC
#define KSTACK_START            0xE0000000
#define KSTACK_SIZE             0x10000
#define HEAP_MAGIC              0x123890AB
#define HEAP_MIN_SIZE           0xA000
#define KERNEL_STACK_SIZE       0x10000

#define TIME_SLICE_PER_AGE      40
#define TIME_QUANTUM            50
#define TICKS_PER_SECOND        (1000 / TIME_QUANTUM)
#define PRIORITY_MIN            10
#define PRIORITY_NORMAL         5
#define PRIORITY_MAX            1

#define COLOUR_BLACK 0
#define COLOUR_BLUE 1
#define COLOUR_GREEN 2
#define COLOUR_CYAN 3
#define COLOUR_RED 4
#define COLOUR_MAGENTA 5
#define COLOUR_BROWN 6
#define COLOUR_LIGHT_GRAY 7
#define COLOUR_DARK_GRAY 8
#define COLOUR_COLOUR_LIGHT_BLUE 9
#define COLOUR_LIGHT_GREEN 10
#define COLOUR_LIGHT_CYAN 11
#define COLOUR_LIGHT_RED 12
#define COLOUR_LIGHT_MAGENTA 13
#define COLOUR_LIGHT_BROWN 14
#define COLOUR_WHITE 15

#include "algorithm.h"

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);

#define PANIC(msg) panic(msg, __FILE__, __LINE__);
/// Use this macro in kernel mode!
#define ASSERT(b) ((b) ? (void)0 : panic_assert(__FILE__, __LINE__, #b))
/// Use this macro in user mode!
#define assert(b) ((b) ? (void)0 : user_assert(__FILE__, __LINE__, #b))

extern void panic(const char *message, const char *file, uint32_t line);
extern void panic_assert(const char *file, uint32_t line, const char *desc);
extern void user_assert(const char *file, uint32_t line, const char *desc);

#endif // COMMON_H
