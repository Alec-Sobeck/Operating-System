#include "pipe.h"
#include "kheap.h"
#include "linked_list.h"
#include "task.h"

#define PIPE_ERROR -1

extern uint32_t pipe_id_generator;
extern list_t *pipe_list;
extern task_t *current_process;

static void pipe_destroy(pipe_t *pipe)
{
    kfree(pipe);
}

static pipe_t *get_and_remove_pipe_by_id(int pipe_id)
{
    struct linked_list_node *node = pipe_list->front;
    while(node){
        pipe_t *value = node->value;
        if(value->id == pipe_id){
            list_remove_node(pipe_list, node);
            return value;
        }
        node = node->next;
    }
    return NULL;
}

static void pipe_decrement_refcount(pipe_t *pipe)
{
    pipe->refcount--;
    if(pipe->refcount == 0){
        get_and_remove_pipe_by_id(pipe->id);
        kfree(pipe);
    }
}

static int current_owns_pipe(int fildes)
{
    list_enumerator_t iter = list_get_enumerator(current_process->pipes);
    while(list_has_next(&iter)) {
        pipe_t *pipe = get_pipe_by_id((uint32_t)list_next_value(&iter));
        if(pipe->id == fildes){
            return TRUE;
        }
    }
    return FALSE;
}

pipe_t *get_pipe_by_id(int pipe_id)
{
    struct linked_list_node *node = pipe_list->front;
    while(node){
        pipe_t *value = node->value;
        if(value->id == pipe_id){
            return value;
        }
        node = node->next;
    }
    return NULL;
}

void reduce_pipe_refcounts(void *data)
{
    pipe_t *pipe = get_pipe_by_id((uint32_t)data);
    if(!pipe){
        return;
    }
    pipe_decrement_refcount(pipe);
}

int open_pipe_impl()
{
    pipe_t *pipe = kmalloc(sizeof(*pipe));
    pipe->id = pipe_id_generator++;
    pipe->head = 0;
    pipe->tail = 0;
    pipe->bytes_stored = 0;
    pipe->refcount = 1;
    memset(pipe->buffer, 0, PIPE_BUFFER_SIZE * sizeof(uint8_t));
    list_add_back(pipe_list, pipe);
    list_add_back(current_process->pipes, (void*)pipe->id);

    return pipe->id;
}

int write_impl(int fildes, const void *buf, unsigned int nbyte)
{
    if(!current_owns_pipe(fildes)){
        return PIPE_ERROR;
    }

    pipe_t *pipe = get_pipe_by_id(fildes);
    if(!pipe){
        return PIPE_ERROR;
    }

    if(PIPE_BUFFER_SIZE - pipe->bytes_stored < nbyte) {
        return 0; // Not enough space.
    }

    // It's safe to write everything.
    const uint8_t *bytes = buf;
    for(uint32_t i = 0; i < nbyte; i++){
        pipe->buffer[pipe->head] = bytes[i];
        pipe->head = (pipe->head + 1) % PIPE_BUFFER_SIZE;

    }
    pipe->bytes_stored += nbyte;
    return nbyte;
}

int read_impl(int fildes, void *buf, unsigned int nbyte)
{
    if(!current_owns_pipe(fildes)){
        return PIPE_ERROR;
    }

    pipe_t *pipe = get_pipe_by_id(fildes);
    if(!pipe){
        return PIPE_ERROR;
    }

    if(nbyte > pipe->bytes_stored) {
        // Trying to read more than is in the pipe.
        // Read off only what we can, then return.
        nbyte = pipe->bytes_stored;
    }

    uint8_t *bytes = buf;
    for(uint32_t i = 0; i < nbyte; i++){
        bytes[i] = pipe->buffer[pipe->tail];
        pipe->tail = (pipe->tail + 1) % PIPE_BUFFER_SIZE;
    }
    pipe->bytes_stored -= nbyte;

    return nbyte;
}

int close_pipe_impl(int fildes)
{
    if(!current_owns_pipe(fildes)){
        return PIPE_ERROR;
    }

    pipe_t *pipe = get_and_remove_pipe_by_id(fildes);
    if(!pipe){
        return PIPE_ERROR;
    }

    pipe_destroy(pipe);

    return fildes;
}
