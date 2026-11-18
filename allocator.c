#include "allocator.h"
#include "freelist.h"

// structure for storing information about each arena
// stores the first address of the arena(this will NOT change), the size of the arena in bytes 
// and the first node of the arena's freelist(this will change).

typedef struct
{
    char *start;           
    size_t size;          
    common_header_t *head; 
} arena_t;

// index 0 is tiny, 1 is small and 2 is medium
arena_t arenas[NUM_ARENAS];
//0 if arenas are created 1 if they are not
int arenas_created = 0;

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

// function for creating the three arenas, each with its own freelist
// returns 1 if all three were created and 0 if mmap failed
int create_arenas(void)
{
    size_t arena_sizes[NUM_ARENAS] = {TINY_SIZE, SMALL_SIZE, MEDIUM_SIZE};

    for (int i = 0; i < NUM_ARENAS; i++)
    {
        //creates memory space for each arena with respective size
        void *temp = get_mem_block(NULL, arena_sizes[i]);

        if (temp == NULL)
        {
            //returns 0 so function that called it knows it has failed
            return 0;
        }

        //initialises the memory space as a list_head
        init_free_list(temp, arena_sizes[i]);

        arenas[i].start = (char *)temp;
        arenas[i].size = arena_sizes[i];
        arenas[i].head = list_head;
    }

    return 1;
}

// function for choosing which arena a request goes into
int choose_arena(int byte_count)
{
    int total_size = byte_count + (int)sizeof(common_header_t);

    if (total_size <= TINY_MAX)
    {
        return 0;
    }
    else if (total_size <= SMALL_MAX)
    {
        return 1;
    }
    else
    {
        return 2;
    }
}

// function for finding which arena a block belongs to, using its address
// returns -1 if the address is not inside any of the arenas
int find_arena(void *ptr)
{
    char *address = (char *)ptr;

    for (int i = 0; i < NUM_ARENAS; i++)
    {
        // the block is in this arena if its address is between the start and the end of the arena
        if (address >= arenas[i].start && address < arenas[i].start + arenas[i].size)
        {
            return i;
        }
    }
    return -1;
}

void *smalloc(int byte_count)
{
    // the arenas on the first memory allocation call
    if (!arenas_created)
    {
        if (!create_arenas())
        {
            return NULL;
        }
        arenas_created = 1;
    }
    int zone = choose_arena(byte_count);

    // The freelist functions all use list_head, so we point it at this arena's freelist
    list_head = arenas[zone].head;

    common_header_t *previous_head = NULL;
    common_header_t *current_head = find_best_listnode(byte_count, &previous_head);

    // statement is true if no suitable node has been found
    if (current_head == NULL)
    {
        return NULL;
    }

    common_header_t *replacement = split_node(current_head, byte_count);

    link_previous_node(previous_head, replacement);
    //updates list_head in case it has changed
    arenas[zone].head = list_head;

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

    common_header_t *meta_head = (common_header_t *)((char *)memptr - sizeof(common_header_t));

    // The block has to go back into the freelist of the arena it came from
    int zone = find_arena(meta_head);
    if (zone == -1)
    {
        return;
    }
    list_head = arenas[zone].head;

    // This will insert the block into the list in address order
    common_header_t *previous = insert_in_order(meta_head);

    // This will join the block with any adjacent free blocks
    merge(meta_head, previous);
    //Updates list head in case it has changed
    arenas[zone].head = list_head;
}

//this function allows us to visualise the block of the free memory list
void allocator_list_dump(void)
{
    char *names[NUM_ARENAS] = {"TINY", "SMALL", "MEDIUM"};
    // prints the freelist of each arena on its own line
    for (int i = 0; i < NUM_ARENAS; i++)
    {
        printf("%s: ", names[i]);
        common_header_t *temp = arenas[i].head;
        while (temp != NULL)
        {
            printf("[SIZE: %d] ----> ", (temp->size));
            temp = temp->next;
        }
        printf("[NULL]\n");
    }
}

size_t allocator_free_mem_size(void)
{
    size_t N, F, L;
    allocator_stats(&N, &F, &L);
    return F;
}

size_t allocator_req_mem(size_t failed_allocation_size)
{
    return failed_allocation_size + sizeof(common_header_t);
}

// function used by the stress test to find the stats of one arena
// zone 0 is tiny, 1 is small and 2 is medium
void allocator_zone_stats(int zone, size_t *N, size_t *F, size_t *L)
{
    list_head = arenas[zone].head;
    freelist_stats(N, F, L);
}

// function used by the stress test to find the number of free blocks (N),
// the total free memory (F) and the largest free block (L) across all three arenas
void allocator_stats(size_t *N, size_t *F, size_t *L)
{
    *N = 0;
    *F = 0;
    *L = 0;

    for (int i = 0; i < NUM_ARENAS; i++)
    {
        size_t zone_N, zone_F, zone_L;
        allocator_zone_stats(i, &zone_N, &zone_F, &zone_L);
        *N += zone_N;
        *F += zone_F;

        //the largest free block is the largest of the three arenas
        if (zone_L > *L)
        {
            *L = zone_L;
        }
    }
}
