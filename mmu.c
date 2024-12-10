#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "list.h"
#include "util.h"

// Function to convert a string to uppercase
void TOUPPER(char * arr){
    for(int i=0;i<strlen(arr);i++){
        arr[i] = toupper(arr[i]);
    }
}

// Function to get input from file and set policy
void get_input(char *args[], int input[][2], int *n, int *size, int *policy) 
{
    FILE *input_file = fopen(args[1], "r");
    if (!input_file) {
        fprintf(stderr, "Error: Invalid filepath\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    parse_file(input_file, input, n, size); // Ensure parse_file correctly sets *n and *size
  
    fclose(input_file);
  
    TOUPPER(args[2]);
  
    if((strcmp(args[2],"-F") == 0) || (strcmp(args[2],"-FIFO") == 0))
        *policy = 1;
    else if((strcmp(args[2],"-B") == 0) || (strcmp(args[2],"-BESTFIT") == 0))
        *policy = 2;
    else if((strcmp(args[2],"-W") == 0) || (strcmp(args[2],"-WORSTFIT") == 0))
        *policy = 3;
    else {
        printf("usage: ./mmu <input file> -{F | B | W }\n(F=FIFO | B=BESTFIT | W=WORSTFIT)\n");
        exit(EXIT_FAILURE);
    }
}

// Function to allocate memory based on policy
void allocate_memory(list_t *freelist, list_t *alloclist, int pid, int blocksize, int policy) {
    node_t *prev = NULL, *current = freelist->head, *selected = NULL, *selected_prev = NULL;

    // Traverse the freelist to find a suitable block based on the policy
    while (current) {
        block_t *blk = current->blk;  // Access the block inside the node
        int blk_size = blk->end - blk->start + 1;

        if (blk_size >= blocksize) {
            if (!selected || 
                (policy == 1) ||  // FIFO: Select the first suitable block
                (policy == 2 && blk_size < (selected->blk->end - selected->blk->start + 1)) ||  // BESTFIT: Smallest block
                (policy == 3 && blk_size > (selected->blk->end - selected->blk->start + 1))) { // WORSTFIT: Largest block
                selected = current;
                selected_prev = prev;
                
                if(policy == 1) { // For FIFO, select the first suitable block and break
                    break;
                }
            }
        }

        prev = current;
        current = current->next;
    }

    // If no suitable block is found
    if (!selected) {
        printf("Error: Memory Allocation failed for %d blocks\n", blocksize);
        return;
    }

    block_t *selected_blk = selected->blk;

    // Remove the selected block from the freelist
    if (selected_prev) {
        selected_prev->next = selected->next;
    } else {
        freelist->head = selected->next;
    }

    // Allocate the block
    block_t *allocated_block = malloc(sizeof(block_t));
    if (!allocated_block) {
        fprintf(stderr, "Error: Unable to allocate memory for allocated_block.\n");
        exit(EXIT_FAILURE);
    }
    allocated_block->pid = pid;
    allocated_block->start = selected_blk->start;
    allocated_block->end = allocated_block->start + blocksize - 1;

    list_add_ascending_by_address(alloclist, allocated_block);

    // Handle leftover fragment
    if (allocated_block->end < selected_blk->end) {
        block_t *fragment = malloc(sizeof(block_t));
        if (!fragment) {
            fprintf(stderr, "Error: Unable to allocate memory for fragment.\n");
            exit(EXIT_FAILURE);
        }
        fragment->pid = 0;
        fragment->start = allocated_block->end + 1;
        fragment->end = selected_blk->end;

        add_to_freelist(freelist, fragment, policy);
    }

    free(selected_blk);
    free(selected);
}

// Function to deallocate memory based on PID
void deallocate_memory(list_t *alloclist, list_t *freelist, int pid, int policy) {
    node_t *prev = NULL, *current = alloclist->head;

    // Traverse the alloclist to find the block with the matching PID
    while (current && current->blk->pid != pid) {
        prev = current;
        current = current->next;
    }

    // If block is not found
    if (!current) {
        printf("Error: Can't locate memory used by PID: %d\n", pid);
        return;
    }

    block_t *blk = current->blk;

    // Remove the block from alloclist
    if (prev) {
        prev->next = current->next;
    } else {
        alloclist->head = current->next;
    }

    // Reset PID and add to freelist
    blk->pid = 0;
    add_to_freelist(freelist, blk, policy);

    free(current);
}

// Function to coalesce free memory blocks
list_t* coalese_memory(list_t * list){
    list_t *temp_list = list_alloc();
    block_t *blk;
    
    // Remove all blocks from the original list and add them to temp_list in ascending order
    while((blk = list_remove_from_front(list)) != NULL) {  
        list_add_ascending_by_address(temp_list, blk);
    }
    
    // Coalesce adjacent blocks
    list_coalese_nodes(temp_list);
          
    return temp_list;
}

// Function to print the list with a message
void print_list(list_t * list, char * message){
    node_t *current = list->head;
    block_t *blk;
    int i = 0;
  
    printf("%s:\n", message);
  
    while(current != NULL){
        blk = current->blk;
        printf("Block %d:\t START: %d\t END: %d", i, blk->start, blk->end);
      
        if(blk->pid != 0)
            printf("\t PID: %d\n", blk->pid);
        else  
            printf("\n");
      
        current = current->next;
        i += 1;
    }
}

/* DO NOT MODIFY */
int main(int argc, char *argv[]) 
{
    int PARTITION_SIZE, inputdata[200][2], N = 0, Memory_Mgt_Policy;
  
    list_t *FREE_LIST = list_alloc();   // list that holds all free blocks (PID is always zero)
    list_t *ALLOC_LIST = list_alloc();  // list that holds all allocated blocks
    int i;
  
    if(argc != 3) {
        printf("usage: ./mmu <input file> -{F | B | W }\n(F=FIFO | B=BESTFIT | W=WORSTFIT)\n");
        exit(EXIT_FAILURE);
    }
  
    get_input(argv, inputdata, &N, &PARTITION_SIZE, &Memory_Mgt_Policy);
  
    // Allocate the initial partition of size PARTITION_SIZE
    block_t * partition = malloc(sizeof(block_t));   // create the partition meta data
    if (!partition) {
        fprintf(stderr, "Error: Unable to allocate memory for initial partition.\n");
        exit(EXIT_FAILURE);
    }
    partition->pid = 0; // Ensure PID is set to 0 for free block
    partition->start = 0;
    partition->end = PARTITION_SIZE + partition->start - 1;
                                
    list_add_to_front(FREE_LIST, partition);          // add partition to free list
                                
    for(i = 0; i < N; i++) { // loop through all the input data and simulate a memory management policy
        printf("************************\n");
        if(inputdata[i][0] != -99999 && inputdata[i][0] > 0) {
            printf("ALLOCATE: %d FROM PID: %d\n", inputdata[i][1], inputdata[i][0]);
            allocate_memory(FREE_LIST, ALLOC_LIST, inputdata[i][0], inputdata[i][1], Memory_Mgt_Policy);
        }
        else if (inputdata[i][0] != -99999 && inputdata[i][0] < 0) {
            printf("DEALLOCATE MEM: PID %d\n", abs(inputdata[i][0]));
            deallocate_memory(ALLOC_LIST, FREE_LIST, abs(inputdata[i][0]), Memory_Mgt_Policy);
        }
        else {
            printf("COALESCE/COMPACT\n");
            FREE_LIST = coalese_memory(FREE_LIST);
        }   
      
        printf("************************\n");
        print_list(FREE_LIST, "Free Memory");
        print_list(ALLOC_LIST, "Allocated Memory");
        printf("\n\n");
    }
  
    list_free(FREE_LIST);
    list_free(ALLOC_LIST);
  
    return 0;
}