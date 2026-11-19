#include <stdio.h>
#include "allocator.h"

#define MAX_PAIRS 1000

int main(void)
{
    void *large[MAX_PAIRS];
    void *tiny[MAX_PAIRS];
    int pairs = 0;

    //Allocate a large block followed by a tiny block, over and over, until a large block no longer fits
    while (pairs < MAX_PAIRS)
    {
        large[pairs] = smalloc(20000);
        if (large[pairs] == NULL)
        {
            break;
        }
        tiny[pairs] = smalloc(100);
        pairs++;
    }

    size_t N, F, L;
    allocator_stats(&N, &F, &L);
    printf("Allocated %d pairs of 20000 byte and 100 byte blocks\n", pairs);
    printf("Free blocks: %zu, total free: %zu bytes, largest free block: %zu bytes\n\n", N, F, L);

    //Free every large block, but keep all the tiny blocks
    for (int i = 0; i < pairs; i++)
    {
        sfree(large[i]);
    }

    allocator_stats(&N, &F, &L);
    printf("After freeing all the large blocks:\n");
    printf("Free blocks: %zu, total free: %zu bytes, largest free block: %zu bytes\n\n", N, F, L);

    //Ask for 40000 bytes, which is bigger than one of the freed large blocks
    void *big = smalloc(40000);

    if (big != NULL)
    {
        printf("smalloc(40000) succeeded\n");
    }
    else
    {
        printf("smalloc(40000) FAILED, needed %zu bytes\n", allocator_req_mem(40000));
    }

    //Free everything that is left
    for (int i = 0; i < pairs; i++)
    {
        sfree(tiny[i]);
    }
    sfree(big);

    return 0;
}