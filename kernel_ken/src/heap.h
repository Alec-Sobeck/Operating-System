#ifndef HEAP_H
#define HEAP_H

typedef struct heap heap_t;

#include "common.h"
#include "heapindex.h"
#include "paging.h"

/// Each memory block/hole has both a header and a footer.
/// A header just tells you how big the piece of memory is, and whether it's actually used by the kernel (block) or
/// available for allocation (hole)
typedef struct
{
    /// Magic number, used for error checking and identification.
    uint32_t magic;
    /// 1 if this is a hole. 0 if this is a block.
    uint8_t is_hole;
    /// size of the block in bytes, including the header and end footer.
    uint32_t size;
} header_t;

/// Each memory block/hole has both a header and a footer.
/// A footer just tells you where the header is (so you know where the block/hole starts)
typedef struct
{
    /// Magic number, same as in header_t.
    uint32_t magic;
    /// Pointer to the block header.
    header_t *header;
} footer_t;

struct heap {
    /// Keeps the memory blocks that are free in sorted order, for easy best-first access
    heap_index_t *index;
    /// The start of our allocated space.
    uint32_t start_address;
    /// The end of our allocated space. May be expanded up to max_address.
    uint32_t end_address;
    /// The maximum address the heap can be expanded to. Note that trying to expand past this
    /// will cause the kernel to error (if this ever happens, we probably have a memory leak though...)
    uint32_t max_address;
    /// Should extra pages requested by us be mapped as supervisor-only?
    uint8_t supervisor;
    /// Should extra pages requested by us be mapped as read-only?
    uint8_t readonly;
    page_directory_t *directory;
} ;


/// Create a new heap_t for a new process/thread.
/// @param [in] start the starting address of the heap (note that the index takes by some space at the
/// very start, so allocations will return memory addresses a bit past this)
/// @param [in] end the heap can, initially, allocate bytes upto this address without having to expand
/// @param [in] max the heap can, if need be, grow to this address over time. Any further growth will cause an error
/// @param [in] supervisor bit for pages
/// @param [in] readonly bit for pages
heap_t *heap_init(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly);

/// Compares two headers based on their size of their memory fragments.
/// @returns -1 if a_size < b_size, 0 if a_size = b_size, 1 if a_size > b_size
int8_t header_comparator(const  void *a, const void *b);

/// Allocate size bytes on the heap, possibly page aligning them
/// @param [in] heap the heap_t to operate on
/// @param [in] size the size of the allocation, in bytes, DO NOT ADD anything for headers/footers/etc. This is done
/// in the function
/// @param [in] page_aligned - 1 if the memory block returned needs to be page aligned, 0 if it doesn't matter
/// @return a void* which is always valid (not NULL). A memory allocation can only fail if the kernel runs out of
/// memory, but this will cause kernel panic instead.
void *heap_alloc(heap_t *heap, uint32_t size, uint8_t page_aligned);

/// Frees memory at the specified address
/// @param [in] heap the heap_t to operate on
/// @param [in] p the address of the memory block to free. NOTE: passing NULL will do nothing.
void heap_free(heap_t *heap, void *p);

/// Frees memory at the specified address
/// @param [in] p the address of the memory block to free. NOTE: passing NULL will do nothing.
void kfree(void *p);

/// Gets the amount of memory free in the heap (before forcing it to expand)
/// \param heap
/// \return
uint32_t heap_remaining_space(heap_t *heap);




#endif