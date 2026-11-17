#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_

//include libraries
#include <stdio.h>
#include <sys/mman.h>


void *get_mem_block(void *addr, size_t mem_size);
void *smalloc(int byte_count);
void sfree(void* memptr);

//task2 functions
void allocator_list_dump(void);
size_t allocator_free_mem_size(void);
size_t allocator_req_mem(size_t failed_allocation_size);

//task3 functions


#endif