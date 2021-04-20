#include "semaphore.h"
#include "kheap.h"
#include "linked_list.h"
#include "task.h"
#include "klib.h"

#define SEM_ERROR 0

extern list_t *ready_queue;
extern task_t *current_process;
extern uint32_t sem_id_generator;
extern list_t *semaphores_list;

static void sem_destroy(sem_t *sem)
{
    queue_destroy(sem->wait_queue);
    kfree(sem);
}

sem_t *get_semaphore_by_id(int sem_id)
{
    struct linked_list_node *node = semaphores_list->front;
    while(node){
        sem_t *value = node->value;
        if(value->id == sem_id){
            return value;
        }
        node = node->next;
    }
    return NULL;
}

static sem_t *get_and_remove_semaphore_by_id(int sem_id)
{
    struct linked_list_node *node = semaphores_list->front;
    while(node){
        sem_t *value = node->value;
        if(value->id == sem_id){
            list_remove_node(semaphores_list, node);
            return value;
        }
        node = node->next;
    }
    return NULL;
}

static void sem_decrement_refcount(sem_t *sem)
{
    sem->refcount--;
    if(sem->refcount == 0){
        get_and_remove_semaphore_by_id(sem->id);
        sem_destroy(sem);
    }
}

static int current_owns_sem(int sem_id)
{
    list_enumerator_t iter = list_get_enumerator(current_process->semaphores);
    while(list_has_next(&iter)) {
        sem_t *sem = get_semaphore_by_id((uint32_t)list_next_value(&iter));
        if(sem->id == sem_id){
            return TRUE;
        }
    }
    return FALSE;
}

void reduce_sem_refcounts(void *data)
{
    sem_t *sem = get_semaphore_by_id((uint32_t)data);
    if(!sem){
        return;
    }
    sem_decrement_refcount(sem);
}

int open_sem_impl(int n)
{
    if(n < 0){
        return SEM_ERROR;
    }

    sem_t *sem = kmalloc(sizeof(*sem));
    sem->id = sem_id_generator++;
    sem->counter = n;
    sem->refcount = 1;
    sem->wait_queue = queue_init();
    list_add_back(semaphores_list, sem);
    list_add_back(current_process->semaphores, (void*)sem->id);
    return sem->id; // I'm pretty sure this can't fail unless we run out of memory?
}

int wait_impl(int s)
{
    if(!current_owns_sem(s)) {
        return SEM_ERROR;
    }

    sem_t *sem = get_semaphore_by_id(s);
    if(!sem) {
        return SEM_ERROR;
    }
    ASSERT(s == sem->id);

    sem->counter--;

    if(sem->counter < 0){
        current_process->state = state_waiting;
        queue_enqueue(sem->wait_queue, (void*)current_process->id);
        run_scheduler(FALSE, TRUE, FALSE);
    }

    sem_t *sem2 = get_semaphore_by_id(s);
    if(!sem2){
        return SEM_ERROR;
    }

    return s;
}

int signal_impl(int s)
{
    if(!current_owns_sem(s)) {
        return SEM_ERROR;
    }


    sem_t *sem = get_semaphore_by_id(s);
    if(!sem){
        return SEM_ERROR;
    }

    sem->counter++;

    if(!queue_is_empty(sem->wait_queue)){
        uint32_t pid = (uint32_t)queue_dequeue(sem->wait_queue);

        task_t *task = get_task_by_pid(pid);
        ASSERT(task);

        task->state = state_ready;
        list_add_back(ready_queue, task);

        run_scheduler(TRUE, TRUE, FALSE);
    }

    return s;

}

int close_sem_impl(int s)
{
    if(!current_owns_sem(s)) {
        return SEM_ERROR;
    }

    // Note: we will return 0 if someone tries to close the semaphore a second time. I'm OK with this because
    // this is technically an error
    sem_t *sem = get_and_remove_semaphore_by_id(s);
    if(!sem){
        return NULL;
    }

    while(!queue_is_empty(sem->wait_queue)) {
        uint32_t pid = (uint32_t)queue_dequeue(sem->wait_queue);

        task_t *task = get_task_by_pid(pid);
        ASSERT(task);

        task->state = state_ready;
        list_add_back(ready_queue, task);
    }

    sem_destroy(sem);
    return s;

}




