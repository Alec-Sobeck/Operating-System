#ifndef USER_LIB_H
#define USER_LIB_H

#include "print.h"
#include "algorithm.h"

/// Functions the same as C's printf, but on a limited number of types in the formatter. Supported format types include
/// at least the following: %x %d %u %s %c %f %e and %%
#define printf(fmt, ...) __user__printf__internal((uint32_t)fmt, ##__VA_ARGS__)
/// Functions the same as C's snprintf, but on a limited number of types in the formatter. Supported format types include
/// at least the following: %x %d %u %s %c %f %e and %%
#define snprintf(buf,size,fmt,...) __snprintf__internal((uint32_t)buf, (uint32_t)size, (uint32_t)fmt, ##__VA_ARGS__)

/// Insertion sort an array of values. This functions on any generic type.
/// \param [in] items array of generic type to sort
/// \param [in] size the length of the elements array (or the number of elements to sort, if not the entire array)
/// \param [in] comparator compare two elements in the array such that if "a&lt;b" it returns -1, if "a==b" then it returns
/// 0, else it returns 1
void insertion_sort(void **items, uint32_t size, int (*comparator)(void *a, void *b));

/// Merge sort an array of values. This functions on any generic type.
/// \param [in] items array of generic type to sort
/// \param [in] size the length of the elements array (or the number of elements to sort, if not the entire array)
/// \param [in] comparator compare two elements in the array such that if "a&lt;b" it returns -1, if "a==b" then it returns
/// 0, else it returns 1
//void merge_sort(void **items, uint32_t size, int (*comparator)(void *a, void *b));



#endif
