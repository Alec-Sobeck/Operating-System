#ifndef KERNEL_LIB_H
#define KERNEL_LIB_H


#include "print.h"
#include "algorithm.h"

#define ksnprintf(buf,size,fmt,...) __snprintf__internal((uint32_t)buf, (uint32_t)size, (uint32_t)fmt, ##__VA_ARGS__)
#define kprintf(fmt, ...) __kernel__printf__internal((uint32_t)fmt, ##__VA_ARGS__)




#endif


