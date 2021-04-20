#ifndef PRINT_H
#define PRINT_H

#include "common.h"

int __user__printf__internal(uint32_t fmt,...);
int __kernel__printf__internal(uint32_t fmt,...);
int __snprintf__internal(uint32_t buf, uint32_t size, uint32_t fmt,...);

#endif