#ifndef PIPE_H
#define PIPE_H

#include "common.h"

typedef struct
{
    uint32_t id;
    uint32_t head;
    uint32_t tail;
    uint32_t bytes_stored;
    uint32_t refcount;
    uint8_t buffer[PIPE_BUFFER_SIZE];
} pipe_t;


int open_pipe_impl();
int write_impl(int fildes, const void *buf, unsigned int nbyte);
int read_impl(int fildes, void *buf, unsigned int nbyte);
int close_pipe_impl(int fildes);

pipe_t *get_pipe_by_id(int pipe_id);
void reduce_pipe_refcounts(void *data);


#endif