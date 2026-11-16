#include "freelist.h"

common_header_t *list_head = NULL;

void init_free_list(void *mem, size_t mem_size)
{
    // converts the starting memory from type void to type common_header
    list_head = (common_header_t *)mem;
    list_head->size = (int)(mem_size - sizeof(common_header_t));
    list_head->next = NULL;
}

int free_list_is_empty(void)
{
    return list_head == NULL;
}

common_header_t *find_first_listnode(int byte_count, common_header_t **previous_head)
{
    // We must keep track of both the current and previous head
    common_header_t *current_head = list_head;
    *previous_head = NULL;

    // This while loop traverses the memory searching for the first freelist node that can store the byte size
    while (current_head != NULL)
    {
        if (byte_count <= (current_head->size))
        {
            break;
        }
        *previous_head = current_head;
        current_head = current_head->next;
    }

    return current_head;
}

common_header_t *split_node(common_header_t *current_head, int byte_count)
{
    // This calculates contiguous bytes of data of the new freelist node
    int remainder_size = current_head->size - byte_count - (int)sizeof(common_header_t);
    // This so we can link the new freelist node witht the next freelist node
    common_header_t *replacement = current_head->next;

    // This is a feature added so that if the size of the freelist node is less than a certain value(8)
    // that malloc function call will occupy additional unneeded bytes.
    if (remainder_size >= (int)sizeof(current_head))
    {
        // Pointer arithmetic to calculate the pointer value for the new freelist node
        // Moves towards higher memory addresses within the allocated memory region.
        common_header_t *remainder = (common_header_t *)((char *)current_head + sizeof(common_header_t) + byte_count);

        // Information about the new freelist node is updated
        remainder->size = remainder_size;
        remainder->next = current_head->next;

        // Pointer is copied so it will exist outside of if statement
        replacement = remainder;

        // current_head becomes new metadata header
        current_head->size = byte_count;
        current_head->next = NULL;
    }

    // otherwise keep current_head->size
    else
    {
        current_head->next = NULL;
    }

    return replacement;
}

void link_previous_node(common_header_t *previous_head, common_header_t *replacement)
{
    // links previous node to new node
    if (previous_head == NULL)
    {
        list_head = replacement;
    }
    else
    {
        previous_head->next = replacement;
    }
}

void insert_at_head(common_header_t *node)
{
    // The freed block points to the first freelist element
    node->next = list_head;
    // The freed block becomes the new first node
    list_head = node;
}

// function for finding best node to create memory
common_header_t *find_best_listnode(int byte_count, common_header_t **previous_head)
{
    //The temporary pointers so we can interrate through list
    //Temporary pointers become actual pointers if the coniditons are satisfied
    common_header_t *temp = list_head;
    common_header_t *temp_previous = NULL; 
    
    //The actual pointers of current and previous nodes that are returned
    common_header_t *final = NULL;         
    *previous_head = NULL;

    while (temp != NULL)
    {
        if (byte_count <= temp->size)
        {
            if (final == NULL || temp->size < final->size)
            {
                final = temp;
                *previous_head = temp_previous; // save the node before the best one
            }
        }
        temp_previous = temp;
        temp = temp->next;
    }
    return final; // NULL if no node fits
}

// function for inserting a freed block so the list stays sorted by address.
// function will also return the previous node
common_header_t *insert_in_order(common_header_t *node)
{
    //temporary pointers to nodes that will change as we itterate through the list
    common_header_t *temp = list_head;
    common_header_t *temp_previous = NULL;

    //When this while loop terminates the value of temp will be the first node with a higher address than the freed block
    while (temp != NULL && temp < node)
    {
        temp_previous = temp;
        temp = temp->next;
    }

    //The freed block is linked in between temp_previous and temp
    node->next = temp;

    if (temp_previous == NULL)
    {
        list_head = node;
    }
    else
    {
        temp_previous->next = node;
    }

    return temp_previous;
}

// function for joining two adjacent nodes
void merge_next(common_header_t *node)
{
    if (node == NULL || node->next == NULL)
    {
        return;
    }

    //Pointer arithmetic to calculate the address just after the end of the node
    common_header_t *test_node_bottom = (common_header_t *)((char *)node + sizeof(common_header_t) + node->size);

    //We then test if they are equal
    if (test_node_bottom == node->next)
    {
        node->size += node->next->size + (int)sizeof(common_header_t);
        node->next = node->next->next;
    }
}


