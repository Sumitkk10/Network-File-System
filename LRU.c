#include "LRU.h"

struct LRUCache* createLRUCache(int size) {
    struct LRUCache* cache = (struct LRUCache*)malloc(sizeof(struct LRUCache));
    if (cache == NULL) {
        perror("Memory allocation for LRU failed");
        exit(EXIT_FAILURE);
    }

    cache->head = NULL;
    cache->tail = NULL;
    cache->size = size;

    return cache;
}

int countNodes(LRUCache* cache) {
    int count = 0;
    struct Node* node = cache->head;
    while (node != NULL) {
        count++;
        node = node->next;
    }
    return count;
}

void insertLRU(LRUCache* cache, char* path, int value) {
    // Check if the path is already in the cache
    
    struct Node* node = cache->head;
    while (node != NULL) {
        if (strcmp(node->path, path) == 0) {
            // Move the existing node to the front of the queue
            if(node->value != value)
                node->value = value;
            if (node->prev != NULL) {
                node->prev->next = node->next;
            } else {
                cache->head = node->next;
            }
            if (node->next != NULL) {
                node->next->prev = node->prev;
            } else {
                cache->tail = node->prev;
            }

            node->prev = NULL;
            node->next = cache->head;
            if (cache->head != NULL) {
                cache->head->prev = node;
            }
            cache->head = node;

            return;
        }
        node = node->next;
    }

    // Create a new node
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    if (newNode == NULL) {
        perror("Memory allocation for LRU failed");
        exit(EXIT_FAILURE);
    }

    // Set node values
    newNode->path = (char*) malloc(1000);
    strcpy(newNode->path, path);
    newNode->value = value;

    // Insert the new node at the front of the queue
    newNode->prev = NULL;
    newNode->next = cache->head;
    if (cache->head != NULL) {
        cache->head->prev = newNode;
    }
    cache->head = newNode;

    // Update the tail if the queue was empty
    if (cache->tail == NULL) {
        cache->tail = newNode;
    }

    // Check if the cache size exceeds the specified size
    if (cache->size > 0 && cache->size < countNodes(cache)) {
        // Remove the least recently used node from the end of the queue
        struct Node* lastNode = cache->tail;
        if (lastNode != NULL) {
            if (lastNode->prev != NULL) {
                lastNode->prev->next = NULL;
            } else {
                cache->head = NULL;
            }

            cache->tail = lastNode->prev;
            free(lastNode);
        }
    }
}

int find(LRUCache* cache, char* path) {
    struct Node* node = cache->head;
    while (node != NULL) {
        if (strcmp(node->path, path) == 0) {
            return node->value; 
        }
        node = node->next;
    }

    return -1;
}

void destroyLRUCache(LRUCache* cache) {
    struct Node* current = cache->head;
    while (current != NULL) {
        struct Node* temp = current;
        current = current->next;
        free(temp);
    }

    free(cache);
}
