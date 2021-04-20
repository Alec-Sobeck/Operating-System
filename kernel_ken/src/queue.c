#include "queue.h"
#include "kheap.h"

queue_t* queue_init()
{
    queue_t *queue = kmalloc(sizeof(*queue));
    queue->internal_list = list_init();
    return queue;
}

int queue_enqueue(queue_t *queue, void *value)
{
    if (!queue)
        return 1;

    list_add_back(queue->internal_list, value);

    return 0;
}

void *queue_dequeue(queue_t *queue)
{
    if(queue_is_empty(queue))
        return NULL;

    return list_remove_front(queue->internal_list);
}

int queue_is_empty(queue_t *queue)
{
    if (!queue)
        return 1;
    if (queue->internal_list->front == NULL)
        return 1;

    return 0;
}

int queue_destroy(queue_t *queue)
{
    if (!queue)
        return 0;

    list_destroy(queue->internal_list);

    kfree(queue);
    return 0;
}







