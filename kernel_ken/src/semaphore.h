#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "common.h"
#include "queue.h"

typedef struct
{
    uint32_t id;
    int counter;
    queue_t *wait_queue;
    uint32_t refcount;

} sem_t;

int open_sem_impl(int n);
int wait_impl(int s);
int signal_impl(int s);
int close_sem_impl(int s);

sem_t *get_semaphore_by_id(int sem_id);
void reduce_sem_refcounts(void *data);


#endif