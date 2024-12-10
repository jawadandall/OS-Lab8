// list/list.c
// Implementation for linked list.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "list.h"

// Allocate a new list
list_t *list_alloc() { 
    list_t* list = (list_t*)malloc(sizeof(list_t));
    if (!list) {
        fprintf(stderr, "Error: Unable to allocate memory for list.\n");
        exit(EXIT_FAILURE);
    }
    list->head = NULL;
    return list; 
}

// Allocate a new node with the given block
node_t *node_alloc(block_t *blk) {   
    node_t* node = malloc(sizeof(node_t));
    if (!node) {
        fprintf(stderr, "Error: Unable to allocate memory for node.\n");
        exit(EXIT_FAILURE);
    }
    node->next = NULL;
    node->blk = blk;
    return node; 
}

// Properly free the entire list, including nodes and blocks
void list_free(list_t *l){
    node_t *current = l->head;
    while(current != NULL){
        node_t *temp = current;
        current = current->next;
        free(temp->blk);  // Free the block
        free(temp);       // Free the node
    }
    free(l); // Free the list structure
}

// Free a single node
void node_free(node_t *node){
    free(node);
}

// Print the list (for debugging)
void list_print(list_t *l) {
    node_t *current = l->head;
    block_t *b;
    
    if (current == NULL){
        printf("list is empty\n");
    }
    while (current != NULL){
        b = current->blk;
        printf("PID=%d START:%d END:%d\n", b->pid, b->start, b->end);
        current = current->next;
    }
}

// Get the length of the list
int list_length(list_t *l) { 
    node_t *current = l->head;
    int i = 0;
    while (current != NULL){
        i++;
        current = current->next;
    }
    return i; 
}

// Add a block to the back of the list
void list_add_to_back(list_t *l, block_t *blk){  
    node_t* newNode = node_alloc(blk);
    newNode->next = NULL;
    if(l->head == NULL){
        l->head = newNode;
    }
    else{
        node_t *current = l->head;
        while(current->next != NULL){
            current = current->next;
        }
        current->next = newNode;
    }
}

// Add a block to the front of the list
void list_add_to_front(list_t *l, block_t *blk){  
    node_t* newNode = node_alloc(blk);
    newNode->next = l->head;
    l->head = newNode;
}

// Add a block at a specific index in the list
void list_add_at_index(list_t *l, block_t *blk, int index){
    if (index < 0) {
        fprintf(stderr, "Error: Negative index not allowed.\n");
        return;
    }
    
    node_t *newNode = node_alloc(blk);
    
    if(index == 0){
        newNode->next = l->head;
        l->head = newNode;
        return;
    }
    
    node_t *current = l->head;
    int i = 0;
    
    while(i < index - 1 && current != NULL){
        current = current->next;
        i++;
    }
    
    if(current == NULL){
        fprintf(stderr, "Error: Index out of bounds.\n");
        free(newNode->blk);
        free(newNode);
        return;
    }
    
    newNode->next = current->next;
    current->next = newNode;
}

// Add a block in ascending order based on address
void list_add_ascending_by_address(list_t *l, block_t *newblk) {
    node_t *newNode = node_alloc(newblk);
    node_t *current = l->head;
    node_t *prev = NULL;

    // If the list is empty or the new block should be the first node
    if (current == NULL || newblk->start < current->blk->start) {
        newNode->next = l->head;
        l->head = newNode;
        return;
    }

    // Traverse the list to find the correct position
    while (current != NULL && newblk->start >= current->blk->start) {
        prev = current;
        current = current->next;
    }

    // Insert the new node
    newNode->next = current;
    prev->next = newNode;
}

// Add a block in ascending order based on block size
void list_add_ascending_by_blocksize(list_t *l, block_t *newblk) {
    node_t *newNode = node_alloc(newblk);
    node_t *current = l->head;
    node_t *prev = NULL;
    int newblk_size = newblk->end - newblk->start + 1;

    // If the list is empty or the new block should be the first node
    if (current == NULL || newblk_size < (current->blk->end - current->blk->start + 1)) {
        newNode->next = l->head;
        l->head = newNode;
        return;
    }

    // Traverse the list to find the correct position
    while (current != NULL && newblk_size >= (current->blk->end - current->blk->start + 1)) {
        prev = current;
        current = current->next;
    }

    // Insert the new node
    newNode->next = current;
    prev->next = newNode;
}

// Add a block in descending order based on block size (fixed size calculation)
void list_add_descending_by_blocksize(list_t *l, block_t *blk){
    node_t *current;
    node_t *prev;
    node_t *newNode = node_alloc(blk);
    int newblk_size = blk->end - blk->start + 1; // Fixed size calculation
    int curblk_size;
    
    if(l->head == NULL){
        l->head = newNode;
        return;
    }
    
    current = l->head;
    prev = NULL;
    
    curblk_size = current->blk->end - current->blk->start + 1;
    
    // If the new block is larger than or equal to the first block
    if(newblk_size >= curblk_size){
        newNode->next = l->head;
        l->head = newNode;
        return;
    }
    
    // Traverse the list to find the correct position
    while(current != NULL && newblk_size < (current->blk->end - current->blk->start + 1)){
        prev = current;
        current = current->next;
    }
    
    // Insert the new node
    newNode->next = current;
    if(prev != NULL){
        prev->next = newNode;
    }
}

// Add a block to the freelist based on policy
void add_to_freelist(list_t *freelist, block_t *fragment, int policy) {
    if (policy == 1) { // FIFO
        list_add_to_back(freelist, fragment);
    } else if (policy == 2) { // BESTFIT
        list_add_ascending_by_blocksize(freelist, fragment);
    } else if (policy == 3) { // WORSTFIT
        list_add_descending_by_blocksize(freelist, fragment);
    }
}

// Remove a block from the back of the list (fixed removal)
block_t* list_remove_from_back(list_t *l){
    block_t *value = NULL;
    node_t *current = l->head;

    if(l->head != NULL){

        if(current->next == NULL) { // one node
            value = current->blk;
            l->head = NULL;
            node_free(current);
        }
        else {
            while (current->next->next != NULL){
                current = current->next;
            }
            value = current->next->blk;
            node_free(current->next);
            current->next = NULL;
        }
    }
    return value;
}

// Get a block from the front without removing
block_t* list_get_from_front(list_t *l) {
    if(l->head == NULL){
        return NULL;
    }
    return l->head->blk;
}

// Remove and return a block from the front of the list
block_t* list_remove_from_front(list_t *l) { 
    block_t *value = NULL;
    if(l->head == NULL){
        return value;
    }
    else{
        node_t *current = l->head;
        value = current->blk;
        l->head = l->head->next;
        node_free(current);
    }
    return value; 
}

// Remove a block at a specific index (no changes needed)
block_t* list_remove_at_index(list_t *l, int index) { 
    int i;
    block_t* value = NULL;
    
    bool found = false;
    
    if(l->head == NULL){
        return value;
    }
    else if (index == 0){
        return list_remove_from_front(l);
    }
    else if (index > 0){
        node_t *current = l->head;
        node_t *prev = current;
        
        i = 0;
        while(current != NULL && !found){
            if(i == index){
                found = true;
            }
            else {
                prev = current;
                current = current->next;
                i++;
            }
        }
        
        if(found && current != NULL) {
            value = current->blk; 
            prev->next = current->next;
            node_free(current);
        }
    }
    return value; 
}

// Compare two blocks for equality
bool compareBlks(block_t* a, block_t *b) {
    if(a->pid == b->pid && a->start == b->start && a->end == b->end)
        return true;
    return false;
}

// Check if a block has at least a certain size
bool compareSize(int a, block_t *b) {  // greater or equal
    if(a <= (b->end - b->start + 1))
        return true;
    return false;
}

// Check if a block has a specific PID
bool comparePid(int a, block_t *b) {
    if(a == b->pid)
        return true;
    return false;
}

// Check if a block exists in the list
bool list_is_in(list_t *l, block_t* value) { 
    node_t *current = l->head;
    while(current != NULL){
        if(compareBlks(value, current->blk)){
            return true;
        }
        current = current->next;
    }
    return false; 
}

// Get a block at a specific index
block_t* list_get_elem_at(list_t *l, int index) { 
    int i;
    block_t* value = NULL;
    if(l->head == NULL){
        return value;
    }
    else if (index == 0){
        return list_get_from_front(l);
    }
    else if (index > 0){
        node_t *current = l->head;
        
        i = 0;
        while(current != NULL){
            if(i == index){
                return(current->blk);
            }
            else {
                current = current->next;
                i++;
            }
        }
    }
    return value; 
}

// Get the index of a specific block
int list_get_index_of(list_t *l, block_t* value){
    int i = 0;
    node_t *current = l->head;
    if(l->head == NULL){
        return -1;
    }
    
    while (current != NULL){
        if (compareBlks(value,current->blk)){
            return i;
        }
        current = current->next;
        i++;
    }
    return -1; 
}

// Check if a block of a certain size exists in the list
bool list_is_in_by_size(list_t *l, int Size){ 
    node_t *current = l->head;
    while(current != NULL){
        if(compareSize(Size, current->blk)){
            return true;
        }
        current = current->next;
    }
    return false; 
}

// Check if a PID exists in the list
bool list_is_in_by_pid(list_t *l, int pid) { 
    node_t *current = l->head;
    while (current != NULL) {
        if (comparePid(pid, current->blk)) {
            return true;
        }
        current = current->next;
    }
    return false;
}

// Get the index of a block with at least a certain size
int list_get_index_of_by_Size(list_t *l, int Size){
    int i = 0;
    node_t *current = l->head;
    if(l->head == NULL){
        return -1;
    }
    
    while (current != NULL){
        if (compareSize(Size,current->blk)){
            return i;
        }
        current = current->next;
        i++;
    }
    return -1; 
}

// Get the index of a block with a specific PID
int list_get_index_of_by_Pid(list_t *l, int pid){
    int i = 0;
    node_t *current = l->head;
    if(l->head == NULL){
        return -1;
    }
    
    while (current != NULL){
        if (comparePid(pid,current->blk)){
            return i;
        }
        current = current->next;
        i++;
    }
    return -1; 
}

// Implement the coalesce function to merge adjacent blocks
void list_coalese_nodes(list_t *l){ 
    if (l->head == NULL) return;

    node_t *current = l->head;

    while(current != NULL && current->next != NULL){
        if(current->blk->end + 1 == current->next->blk->start){
            // Merge the two blocks
            current->blk->end = current->next->blk->end;
            node_t *temp = current->next;
            current->next = temp->next;
            free(temp->blk); // Free the merged block's block structure
            free(temp);      // Free the merged node
        }
        else{
            current = current->next;
        }
    }
}