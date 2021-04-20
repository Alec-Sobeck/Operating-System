#include "ulib.h"
#include "kernel_ken.h"

void insertion_sort(void **items, uint32_t size, int (*comparator)(void *a, void *b))
{
    uint32_t i = 1;

    while(i < size){
        uint32_t j = i;
        while(j > 0 && comparator(items[j - 1], items[j]) == 1) {
            void *t = items[j];
            items[j] = items[j - 1];
            items[j - 1] = t;
            j--;
        }
        i++;
    }
}
