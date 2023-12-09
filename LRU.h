#ifndef __LRU_H_
#define __LRU_H_

#include "headers.h"

typedef struct Node {
    char* path; 
    int value;
    struct Node* next;
    struct Node* prev;
}Node;

typedef struct LRUCache {
    struct Node* head;
    struct Node* tail;
    int size;
}LRUCache;

struct LRUCache* createLRUCache(int size);

void insertLRU(LRUCache* cache, char* path, int value);

int countNodes(LRUCache* cache);

void destroyLRUCache(LRUCache* cache);

int find(LRUCache* cache, char* path);

#endif
