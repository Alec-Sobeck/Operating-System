// paging.h -- Defines the interface for and structures relating to paging.
//             Written for JamesM's kernel development tutorials.

#ifndef PAGING_H
#define PAGING_H

#include "common.h"
#include "isr.h"

// I can't do this justice. Go read the tutorial at
// https://wiki.osdev.org/Paging
// The fields are laid out like in the page table entry figure.
// 31 --- 11        7 --- 0
// FRAME            DACWURP
typedef struct page
{
    uint32_t contents;
} page_t;

void page_set_frame(page_t *page, uint32_t frame);
uint32_t page_get_frame(page_t *page);
void page_set_present(page_t *page, uint32_t frame);
uint32_t page_get_present(page_t *page);
void page_set_rw(page_t *page, uint32_t frame);
uint32_t page_get_rw(page_t *page);
void page_set_user(page_t *page, uint32_t frame);
uint32_t page_get_user(page_t *page);
void page_set_accessed(page_t *page, uint32_t frame);
uint32_t page_get_accessed(page_t *page);
void page_set_dirty(page_t *page, uint32_t frame);
uint32_t page_get_dirty(page_t *page);


typedef struct page_table
{
    page_t pages[1024];
} page_table_t;

typedef struct page_directory
{
    /// Array of pointers to pagetables.
    page_table_t *tables[1024];
    /// Array of pointers to the pagetables above, but gives their *physical*
    /// location, for loading into the CR3 register.
    uint32_t tablesPhysical[1024];

    /// The physical address of tablesPhysical. This comes into play
    /// when we get our kernel heap allocated and the directory
    /// may be in a different location in virtual memory.
    uint32_t physicalAddr;
} page_directory_t;

uint32_t get_number_free_frames();

///   Sets up the environment, page directories etc and
///   enables paging.
void initialise_paging();

///   Causes the specified page directory to be loaded into the
///   CR3 register.
void switch_page_directory(page_directory_t *new);

///   Retrieves a pointer to the page required.
///   If make == 1, if the page-table in which this page should
///   reside isn't created, create it!
page_t *get_page(uint32_t address, int make, page_directory_t *dir);


/// Handler for page faults.
void page_fault(registers_t *regs);

void alloc_frame(page_t *page, int is_kernel, int is_writeable);
void free_frame(page_t *page);


/// Copy a page table and its contents (based on JamesM's tutorial #9)
/// This basically does a "deep copy" of the frame, creating a new copy that is
/// completely separate from the original (because we need a new memory space)
page_table_t *copy_table(page_table_t *src, uint32_t *physical_addr);

/// Copies a page directory. Heavily based on JamesM's tutorial #9, because that has almost the exact
/// functionality required.
page_directory_t *clone_directory(page_directory_t *src);


#endif
