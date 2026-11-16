#ifndef _FREELIST_H_
#define _FREELIST_H_

//include libraries
#include <stddef.h>

//define constants
#define MIN_REMAINDER 8

//structure for both freelist nodes and metadata headers
typedef struct __common_header_t
{
    int size;                       // size of the memory block
    struct __common_header_t *next; // pointer to the next node of the freelist
} common_header_t;

extern common_header_t *list_head;

void init_free_list(void *mem, size_t mem_size);
int free_list_is_empty(void);
common_header_t *find_first_listnode(int byte_count, common_header_t **previous_head);
common_header_t *split_node(common_header_t *current_head, int byte_count);
void link_previous_node(common_header_t *previous_head, common_header_t *replacement);
void insert_at_head(common_header_t *node);

#endif