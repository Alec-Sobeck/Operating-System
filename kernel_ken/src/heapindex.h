#ifndef ORDERED_ARRAY_H
#define ORDERED_ARRAY_H


// The heap index is an ordered array that grows backwards from a max address. This means all the pointer
// arithmetic uses negative indices as opposed to positive indices.

typedef struct heap_index heap_index_t;


#include "common.h"
#include "heap.h"

struct heap_index {
    uint32_t max_address;
    /// Largest number of elements ever stored in the index.
    uint32_t max_size;
    /// The number of elements currently used (anything past this is considered garbage and cannot
    /// be accessed, even though it is technically allocated to the array)
    uint32_t size;
    /// Compare two generic values. This should return -1 if "a < b", 0 if "a = b", else 1 for a > b
    int8_t (*comparator)(const void *a, const void *b);
    heap_t *owner;
} ;

/// Init heap_index_t WITHOUT using kmalloc, at the specified address. Mostly used for before we
/// have a heap. Note that this can't check that that memory is corrected allocated, the calling function must do that.
/// @param [in] max_size the maximum number of elements this array can ever contain (initially 0 are used), and the
/// array grows as needed.
/// @param [in] comparator compares two generic values for sorting purposes, with the following rules:
/// should return -1 if a < b, 0 if a = b, 1 if a > b
/// @param [in] placement_address the array will use the memory from placement_address to
/// (placement_address + max_size * sizeof(void*)).
/// @return an heap_index_t with the specified parameters
heap_index_t *hindex_init_placement(uint32_t max_address, int8_t (*comparator)(const void *, const void *), heap_t *owner);

/// Access element at index, with bounds checks.
void *hindex_at(heap_index_t *hindex, uint32_t index);

/// Remove element at index, with bounds checks.
/// @param [in] hindex heap_index_t to operate on
/// @param [in] index uint32_t specifying the array index to remove. Must be in range [0, size) else this will error
void hindex_erase(heap_index_t *hindex, uint32_t index);

/// Insert element in sorted order.
/// @param [in] hindex heap_index_t to operate on
/// @param [in] item value to insert, with position based on comparator
void hindex_insert(heap_index_t *hindex, void *item);


#endif
