#ifndef QUEUE_H
#define QUEUE_H

#include "linked_list.h"

/// Represents a FIFO queue data structure.
typedef struct
{
    list_t *internal_list;
} queue_t;

/// Create a new queue data structure
queue_t* queue_init();
int queue_destroy(queue_t* queue);
int queue_enqueue(queue_t* queue, void *value);
void *queue_dequeue(queue_t* queue);
int queue_is_empty(queue_t* queue);




#endif