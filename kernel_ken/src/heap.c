#include "heap.h"
#include "monitor.h"
#include "kheap.h"
#include "klib.h"


/// Expands the heap by the specified number of bytes
/// @param [in] heap the heap_t to operate on
/// @param [in] increase a uint32_t specifying the number of bytes that the heap needs to grow by (at minimum).
/// the heap will grow a bit more than this though, in order to be page aligned.
void heap_expand(heap_t *heap, uint32_t increase)
{
    // Make sure the heap doesn't grow past the maximum allowed size.
    // This probably shouldn't cause kernel panic, but I'm not going to fix it unless it becomes a real issue

    if(heap->end_address + increase > heap->max_address) {
        kprintf("Your process is trying to allocate too much memory. \n");
        PANIC("User Process allocated too much memory");
    }

    // Compute new max and make sure it's page aligned.
    uint32_t new_max = heap->end_address + increase;
    if(new_max % PAGE_SIZE != 0){
        new_max = ((new_max / PAGE_SIZE) * PAGE_SIZE) + PAGE_SIZE;
    }

    ASSERT(new_max > heap->end_address);
    ASSERT(new_max >= heap->end_address + increase);

    uint32_t i;
    for(i = heap->end_address; i < new_max; i += PAGE_SIZE) {
        alloc_frame(get_page(i, TRUE, heap->directory), heap->supervisor, !heap->readonly);
    }

    heap->end_address = new_max;
    assert(heap->end_address % PAGE_SIZE == 0);

}

/// Contracts the heap by the specified number of bytes
/// @param [in] heap the heap_t to operate on
/// @param [in] decrease a uint32_t specifying the number of bytes that the heap needs to shrink by (at maximum).
/// it may not always be possible to shrink it by exactly this amount, in some weird edge cases.
void heap_contract(heap_t *heap, uint32_t decrease)
{
    // Here be a check for numerical underflow
    ASSERT(decrease <= (heap->end_address - heap->start_address));

    // Page align if needed.
    uint32_t new_size = (heap->end_address - heap->start_address) - decrease;
    if(new_size < HEAP_MIN_SIZE) {
        new_size = HEAP_MIN_SIZE;
    }
    if(new_size % PAGE_SIZE != 0){
        new_size = ((new_size / PAGE_SIZE) * PAGE_SIZE) + PAGE_SIZE;
    }

    uint32_t new_max = heap->start_address + new_size;
    ASSERT(new_max > heap->start_address);
    ASSERT(new_max <= heap->end_address);

    uint32_t i;
    for(i = new_max; i <= heap->end_address; i += PAGE_SIZE) {
        free_frame(get_page(i, FALSE, heap->directory));
    }
    heap->end_address = new_max;
}

/// Finds the smallest memory hole that can fit [size] bytes.
/// @param [in] heap the heap_t to operate on
/// @param [in] size a uint32_t specifying the size of the memory block we need to allocate. This
/// should include the footer_t and header_t sizes already.
/// @param [in] page_aligned a "bool" (TRUE/FALSE) - if 1 then make sure the block returned allows us to also align
/// the result on a page boundry.
int32_t heap_best_fit(heap_t *heap, uint32_t size, uint8_t page_aligned)
{
    for(uint32_t i = 0; i < heap->index->size; i++){
        header_t *header = hindex_at(heap->index, i);

        if(page_aligned && header->size >= size) {
            // Things get weird because the header has to come before the page alignment in this setup.
            uint32_t space_remaining = 0;
            // ... so it becomes necessary to include header_t size in the check for page alignment
            if(((uint32_t)header + sizeof(header_t)) % PAGE_SIZE != 0){
                space_remaining = PAGE_SIZE - (((uint32_t)header + sizeof(header_t)) % PAGE_SIZE);
            }
            // Check if the size (after aligning the memory block) is still enough
            uint32_t remaining_size = header->size - space_remaining;
            if(remaining_size >= size){
                // If so: we're done.
                return i;
            }
        } else if(size <= header->size) {
            // Block has enough space.
            return i;
        }
    }
    return -1;
}

int8_t header_comparator(const void *a, const void *b)
{
    const header_t *h_a = a;
    const header_t *h_b = b;
    return (h_a->size < h_b->size) ? -1 : (h_a->size > h_b->size) ? 1 : 0;
}

heap_t *heap_init(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly)
{
    // Everything has to be page aligned, I guess. So error quickly and error loudly.
    ASSERT(start % PAGE_SIZE == 0);
    ASSERT(end % PAGE_SIZE == 0);
    ASSERT(end <= max);

    heap_t *heap = kmalloc(sizeof(*heap));
    heap->start_address = start;
    heap->end_address = end;
    heap->max_address = max;
    heap->supervisor = supervisor;
    heap->readonly = readonly;
    heap->directory = NULL;
    // Create the index
    heap->index = hindex_init_placement(max, header_comparator, heap);

    // Write the first header/footer to the heap
    header_t header;
    header.magic = HEAP_MAGIC;
    header.is_hole = TRUE;
    header.size = (heap->end_address - heap->start_address);
    *((header_t*)heap->start_address) = header;
    footer_t footer;
    footer.magic = HEAP_MAGIC;
    footer.header = (header_t*)heap->start_address;
    *((footer_t*)(heap->end_address - sizeof(footer_t))) = footer;

    hindex_insert(heap->index, (void *) heap->start_address);

    ASSERT(((header_t*)heap->start_address)->magic == HEAP_MAGIC);

    return heap;
}

void write_contents(heap_t *heap)
{
    char a[1000];
    monitor_write("END=");
    monitor_write(itoa((uint32_t)heap->end_address, a, 16));

    monitor_write("\n");

    uint32_t i;
    for(i = 0; i < heap->index->size; i++){
        header_t *header = hindex_at(heap->index, i);
        kprintf("hole => %d => %x \n", header->size, header);
    }
} 

void *heap_alloc(heap_t *heap, uint32_t size, uint8_t page_aligned)
{
    ASSERT(heap->directory);
    // Force all blocks to be a multiple of 4. This forces all blocks to start on a word and end on a word.
    uint32_t word_aligned_size = size;
    if(word_aligned_size % WORD_SIZE != 0){
        word_aligned_size += (WORD_SIZE - (word_aligned_size % WORD_SIZE));
    }
    ASSERT(word_aligned_size % WORD_SIZE == 0);
    ASSERT(word_aligned_size >= size);
    ASSERT(word_aligned_size <= size + WORD_SIZE);
    size = word_aligned_size;

    uint32_t full_size = size + sizeof(header_t) + sizeof(footer_t);

    int32_t block_index = heap_best_fit(heap, full_size, page_aligned);

    if(block_index == -1){
        // Try to expand the heap since this allocation failed.
        uint32_t old_end = heap->end_address;
        uint32_t old_size = heap->end_address - heap->start_address;
        // Page aligned might require another page of memory.
        // Let's just request it in advance. I'm not very bothered by the small wastage.
        heap_expand(heap, full_size + (!!page_aligned) * PAGE_SIZE * 2);

        uint32_t new_size = heap->end_address - heap->start_address;
        footer_t *final_footer = (footer_t*)(old_end - sizeof(footer_t));
        ASSERT(final_footer->magic == HEAP_MAGIC);
        ASSERT(new_size >= old_size);

        int new_header = FALSE;
        if(heap->index->size == 0){
            new_header = TRUE;
        } else {
            header_t *final_header = final_footer->header;

            ASSERT(final_footer->magic == HEAP_MAGIC);
            ASSERT(final_header->magic == HEAP_MAGIC);
            ASSERT(final_header->is_hole);

            if(final_header->is_hole){
                final_header->size += (new_size - old_size);

                footer_t *added_footer = (footer_t *)(heap->end_address - sizeof(footer_t));
                added_footer->header = final_header;
                added_footer->magic = HEAP_MAGIC;

            } else {
               new_header = TRUE;
            }
        }

        if(new_header){
            // Create a header/footer if we didn't handle the new memory already
            header_t *header = (header_t*)old_end;
            header->magic = HEAP_MAGIC;
            header->is_hole = TRUE;
            header->size = new_size - old_size;
            footer_t *footer = (footer_t*)(heap->end_address - sizeof(footer_t));
            footer->magic = HEAP_MAGIC;
            footer->header = header;

            ASSERT(header->size > 20);
            ASSERT(header->magic == HEAP_MAGIC);
            hindex_insert(heap->index, header);
        }

        // Retry the allocation
        block_index = heap_best_fit(heap, full_size, page_aligned);

        // This should work this time. If not, I've done something terribly wrong or we're completely out of memory
        ASSERT(block_index != -1);
    }

    header_t *hole = (header_t*) hindex_at(heap->index, block_index);
    uint32_t hf_size = sizeof(footer_t) + sizeof(header_t);

    ASSERT(hole->size >= full_size);
    // Check if there's enough room to split the block
    if(hole->size - full_size <= hf_size){
        // Not enough. Just add the <= 20 bytes to the back of whatever the user requested.
        full_size = hole->size;
        size = full_size - hf_size;
    }
    hindex_erase(heap->index, block_index);

    uint32_t block_start = (uint32_t)hole;
    uint32_t remaining_size = hole->size;

    // If page_aligned is TRUE, we might've fragmented the block into 3 pieces.
    // Thus, we have to check for that.
    if(page_aligned && ((uint32_t)hole + sizeof(header_t)) % PAGE_SIZE != 0) {
        uint32_t space_left = PAGE_SIZE - (((uint32_t)hole + sizeof(header_t)) % PAGE_SIZE);

        if(space_left > hf_size){
            // Write a hole before the block, in this particular case
            // (we may have to triple split the block depending on what's at the end).

            block_start = (uint32_t)hole + space_left;
            // Simple: just rewrite the header/footer.
            header_t *hole_before_header = hole;
            hole_before_header->size = space_left;
            hole_before_header->magic = HEAP_MAGIC;
            hole_before_header->is_hole = TRUE;
            footer_t *hole_before_footer = (footer_t*)((uint32_t)hole + space_left - sizeof(footer_t));
            hole_before_footer->magic = HEAP_MAGIC;
            hole_before_footer->header = hole_before_header;
            // and of course, put it in the index.
            ASSERT(hole_before_header->size > 20);
            ASSERT(hole_before_header->size <= 1000000000);

            hindex_insert(heap->index, hole_before_header);
            remaining_size -= hole_before_header->size;
        }
    }

    // If there's a trivial amount of space left over, just make the block given to the user a tiny bit bigger
    // this space can't be put in the above hole because it'd wreck the page alignment.
    remaining_size -= full_size;
    if(remaining_size <= hf_size) {
        size += remaining_size;
        full_size += remaining_size;
        remaining_size = 0;
    }

    // This just got allocated
    header_t *block_header = (header_t*)(block_start);
    block_header->magic = HEAP_MAGIC;
    block_header->size = full_size;
    block_header->is_hole = FALSE;
    footer_t *block_footer = (footer_t*)(block_start + full_size - sizeof(footer_t));
    block_footer->magic = HEAP_MAGIC;
    block_footer->header = block_header;

    // This is whatever's left to the right side of the block
    if(remaining_size != 0) {
        header_t *after_header = (header_t *) ((uint32_t) block_footer + sizeof(footer_t));

        after_header->magic = HEAP_MAGIC;
        after_header->is_hole = TRUE;
        after_header->size = remaining_size;
        footer_t *after_footer = (footer_t *) ((uint32_t) after_header - sizeof(footer_t) + remaining_size);
        after_footer->magic = HEAP_MAGIC;
        after_footer->header = after_header;

        ASSERT((uint32_t) after_footer <= heap->end_address);
        ASSERT(after_header->size >= 20);

        hindex_insert(heap->index, after_header);
    }

    ASSERT(block_header->magic == HEAP_MAGIC);
    ASSERT(block_footer->magic == HEAP_MAGIC);
    if(page_aligned){
        ASSERT(((uint32_t)block_header + sizeof(header_t)) % PAGE_SIZE == 0);
    }

    return (void*)((uint32_t)block_header + sizeof(header_t));
}

void heap_free(heap_t *heap, void *p)
{
    if(!p){
        return;
    }

    header_t *header = (header_t*)((uint32_t)(p) - sizeof(header_t));
    footer_t *footer = (footer_t*)((uint32_t)(p) - sizeof(header_t) - sizeof(footer_t) + header->size);

    ASSERT(!header->is_hole);
    ASSERT(header->magic == HEAP_MAGIC);
    ASSERT(footer->magic == HEAP_MAGIC);

    header->is_hole = TRUE;

    // First: merge left.
    // Note that we don't want to merge left if we're at the start of memory...
    if((uint32_t)header != heap->start_address){
        footer_t *left_footer = (footer_t*)((uint32_t)header - sizeof(footer_t));
        // This should always be true based on my modifications to the algorithm.
        ASSERT(left_footer->magic == HEAP_MAGIC);
        if(left_footer->header->is_hole){
            left_footer->header->size += header->size;
            footer->header = left_footer->header;
            header = footer->header;

            // It is necessary to remove this from the index, and maybe replace it later, because its size changed
            uint32_t i;
            for(i = 0; i < heap->index->size; i++){
                if(hindex_at(heap->index, i) == (void*)header){
                    break;
                }
            }
            // If this fails, it indicates an error in the algorithm (missed a footer/header).
            ASSERT(i != heap->index->size);
            hindex_erase(heap->index, i);
        }
    }

    // Now merge right. (assuming we're not at the heap end)
    if((uint32_t)footer + sizeof(footer_t) != heap->end_address) {
        header_t *right_header = (header_t*)((uint32_t)footer + sizeof(footer_t));
        ASSERT(right_header->magic == HEAP_MAGIC);
        if(right_header->magic == HEAP_MAGIC && right_header->is_hole) {
            header->size += right_header->size;
            footer = (footer_t*)((uint32_t)(right_header) + right_header->size - sizeof(footer_t));
            footer->header = header;

            // It is necessary to remove this from the index, and maybe replace it later, because its size changed
            uint32_t i;
            for(i = 0; i < heap->index->size; i++){
                if(hindex_at(heap->index, i) == (void*)right_header){
                    break;
                }
            }
            // If this fails, it indicates an error in the algorithm (missed a footer/header).
            ASSERT(i != heap->index->size);
            hindex_erase(heap->index, i);
        }
    }

    // At this point, whatever is assigned to "header" is either the 1 block we just freed, or
    // that block merged with its left and/or right neighbours.
    // If it happens that our footer is at the end of memory, we can try to shrink the heap a
    // bit. Other than that, the rest is simple. Just add that block to the index.
    char should_add = TRUE;
    if((uint32_t)footer + sizeof(footer_t) == heap->end_address){
        uint32_t old_size = heap->end_address - heap->start_address;
        heap_contract(heap, header->size - sizeof(header_t) - sizeof(footer_t));
        uint32_t new_size = heap->end_address - heap->start_address;
        ASSERT(new_size <= old_size);
        uint32_t actual_decrease = old_size - new_size;

        if(actual_decrease == header->size){
            // block no longer exists.
            should_add = FALSE;
        } else {
            // Block still exists
            header->size -= actual_decrease;
            footer_t *footer = (footer_t*)((uint32_t)header + header->size - sizeof(footer_t));
            footer->magic = HEAP_MAGIC;
            footer->header = header;
            ASSERT((uint32_t)footer + sizeof(footer_t) == heap->end_address);
        }
    }


    if(should_add){
        hindex_insert(heap->index, header);
    }


}

uint32_t heap_remaining_space(heap_t *heap)
{
    uint32_t free_space = 0;

    uint32_t i;
    for(i = 0; i < heap->index->size; i++){
        header_t *hole = hindex_at(heap->index, i);
        free_space += hole->size;
    }

    return free_space;
}

