#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_

//include libraries
#include <stdio.h>
#include <sys/mman.h>

//define constants
//updated 10MB of memory
#define MEM_SIZE (10 * 1024 * 1024)



void *get_mem_block(void *addr, size_t mem_size);
void *smalloc(int byte_count);
void sfree(void* memptr);

//task2 functions
void allocator_list_dump(void);
size_t allocator_free_mem_size(void);
size_t allocator_req_mem(size_t failed_allocation_size);

//task4 functions
void allocator_stats(size_t *N, size_t *F, size_t *L);


#endif