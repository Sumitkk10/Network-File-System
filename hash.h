#ifndef __HASH_H_
#define __HASH_H_

#include "headers.h"

// maintain a hash table where values are linked list
// first calc hash of string % huge prime num (1e9 + 7)
// coordinate compress it
// make a hash table out of this compressed values.

typedef struct node{
    char* a;
    int n;
    int index;
    pthread_rwlock_t lock;
    struct node* nxt;
}node;

typedef struct compress{
    char* a;
    long long int hashval;
}compress;

node** CreateHT(int itablesize);

void Insert(char* x, int n, int pos, node** hashtable, int ss);
void Delete(char* x, int pos, node** hashtable);
long long int hash(char* x, int* primes, int n);

bool isPrime(int x);
int findnextprime(int x);

int get(node** hashtable, char* cmp, int pos);

#endif
