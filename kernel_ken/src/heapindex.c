#include "heapindex.h"
#include "kheap.h"


heap_index_t *hindex_init_placement(uint32_t max_address, int8_t (*comparator)(const void *, const void *), heap_t *owner)
{
    heap_index_t *index = kmalloc(sizeof(*index));
    index->max_address = max_address;
    index->size = 0;
    index->max_size = 0;
    index->comparator = comparator;
    index->owner = owner;
    return index;
}

void *hindex_at(heap_index_t *hindex, uint32_t index)
{
    ASSERT(index < hindex->size && index >= 0);
    void **memory = (void**)hindex->max_address;
    return *(memory - index);
}

void hindex_erase(heap_index_t *hindex, uint32_t index)
{
    ASSERT(index < hindex->size);

    // Shift everything that's left by 1.
    void **memory = (void**)hindex->max_address;

    int i;
    for(i = index + 1; i < hindex->size; i++){
        *(memory - (i - 1)) = *(memory - i);
    }
    hindex->size--;
}

void hindex_try_grow(heap_index_t *hindex)
{
    if(hindex->size > hindex->max_size){
        hindex->max_size = hindex->size;
        alloc_frame(get_page(hindex->max_address - (sizeof(void*) * hindex->max_size), TRUE, hindex->owner->directory),
            hindex->owner->supervisor, !hindex->owner->readonly);
    }

}

void hindex_insert(heap_index_t *hindex, void *item)
{
    ASSERT(hindex->comparator);

    void **memory = (void **)hindex->max_address;
    // Find index to insert at using the comparator.
    int insert_index = 0;
    while(insert_index < hindex->size && hindex->comparator(*(memory - insert_index), item) != 1){
        insert_index++;
    }

    // Insert at end (we know there's space or we'd have errored before)
    if(insert_index == hindex->size){
        *(memory - insert_index) = item;
        hindex->size++;
        hindex_try_grow(hindex);
        return;
    }

    ASSERT(insert_index >= 0);

    // Else insert at the appropriate index and shift everything.
    int32_t i;
    for(i = hindex->size; i > insert_index; i--){
        *(memory - i) = *(memory - (i - 1));
    }
    *(memory - insert_index) = item;

    hindex->size++;
    hindex_try_grow(hindex);
}











