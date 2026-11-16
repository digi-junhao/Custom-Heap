#include <stdio.h>
#include "allocator.h"

int main(void)
{
    // Fill the heap with a 500, 40 and 400 byte block
    void *a = smalloc(500);
    void *b = smalloc(40);
    void *c = smalloc(400);

    printf("After allocating 500, 40 and 400 bytes:\n");
    allocator_list_dump();

    //Free the 40 byte block, then the 400 byte block.
    //Every time you free a block that block goes to the head so the 400 byte block is first
    sfree(b);
    sfree(c);

    printf("\nAfter freeing the 40 and 400 byte blocks:\n");
    allocator_list_dump();
    printf("Total free memory: %zu bytes\n", allocator_free_mem_size());

    // Ask for 40 bytes, which should fit perfectly
    void *d = smalloc(40);

    printf("\nAfter smalloc(40):\n");
    allocator_list_dump();

    //Allocate for 400 bytes again.
    void *e = smalloc(400);

    if (e != NULL)
    {
        printf("\nsmalloc(400) succeeded\n");
    }
    else
    {
        printf("\nsmalloc(400) FAILED, needed %zu bytes\n", allocator_req_mem(400));
    }

    printf("Free list: ");
    allocator_list_dump();

    sfree(a);
    sfree(d);
    sfree(e);
    return 0;
}