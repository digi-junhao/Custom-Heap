#include "allocator.h"
#include "freelist.h"

void *get_mem_block(void *addr, size_t mem_size)
{
    void *h_ptr = mmap(
        addr,                        // starting address hint (NULL lets kernel choose)
        mem_size,                    // number of bytes requested
        PROT_READ | PROT_WRITE,      // memory should be readable and writable
        MAP_PRIVATE | MAP_ANONYMOUS, // private mapping, not backed by a file
        -1,                          // no file descriptor needed
        0                            // offset must be zero for MAP_ANONYMOUS
    );

    if (h_ptr == MAP_FAILED)
    {
        return NULL; // return NULL if mmap fails
    }
    return h_ptr; // otherwise return the allocated memory
}

void *smalloc(int byte_count)
{

    if (free_list_is_empty())
    {
        void *temp = get_mem_block(NULL, MEM_SIZE);

        if (temp == NULL)
        {
            return NULL;
        }

        init_free_list(temp, MEM_SIZE);
    }

    common_header_t *previous_head = NULL;
    common_header_t *current_head = find_best_listnode(byte_count, &previous_head);

    //statement is true if no suitable node has been found
    if (current_head == NULL)
    {
        return NULL;
    }

    common_header_t *replacement = split_node(current_head, byte_count);

    link_previous_node(previous_head, replacement);

    // This gives address to the first memory location via pointer arithmetic
    // If you didnt convert curret_head pointer to type char it would move 16x16 memory
    return (char *)current_head + sizeof(common_header_t);
}

void sfree(void *memptr)
{
    if (memptr == NULL)
    {
        return;
    }

    common_header_t* meta_head = (common_header_t*)((char*)memptr - sizeof(common_header_t));

    // This will insert the block into the list in address order
    common_header_t* previous = insert_in_order(meta_head);

    // This will join the block with any adjacent free blocks
    merge(meta_head, previous);
}

void allocator_list_dump(void)
{
    common_header_t *temp = list_head;
    while (temp != NULL)
    {
        printf("[SIZE: %d] ----> ", (temp->size));
        temp = temp->next;
    }
    printf("[NULL]\n");
}

size_t allocator_free_mem_size(void)
{
    int count = 0;
    common_header_t *temp = list_head;
    while (temp != NULL)
    {
        count += temp->size;
        temp = temp->next;
    }
    return count;
}

size_t allocator_req_mem(size_t failed_allocation_size)
{
    return failed_allocation_size + sizeof(common_header_t);
}

// allocator.c
// function used by the stress test to find the number of free blocks (N),
// the total free memory (F) and the largest free block (L)
void allocator_stats(size_t *N, size_t *F, size_t *L)
{
    freelist_stats(N, F, L);
}
