
#include "allocator.h"

int main(void)
{
    // First fill the heap with three 200 byte blocks and a 300 byte block
    void *a = smalloc(200);
    void *b = smalloc(200);
    void *c = smalloc(200);
    void *d = smalloc(300);

    printf("After allocating 200, 200, 200 and 300 bytes:\n");
    allocator_list_dump();

    //Then free the memory of a and c, which are not next to each other
    sfree(a);
    sfree(c);

    printf("\nAfter freeing a and c:\n");
    allocator_list_dump();

    //Then free the memory of b, which is in between a and c
    sfree(b);

    printf("\nAfter freeing b:\n");
    allocator_list_dump();
    printf("Total free memory: %zu bytes\n", allocator_free_mem_size());

    //Finally ask for 500 bytes, which is bigger than any of the blocks that were freed
    void *e = smalloc(500);

    if (e != NULL)
    {
        printf("\nsmalloc(500) succeeded\n");
    }
    else
    {
        printf("\nsmalloc(500) FAILED, needed %zu bytes\n", allocator_req_mem(500));
    }

    printf("Free list: ");
    allocator_list_dump();

    sfree(d);
    sfree(e);
    return 0;
}