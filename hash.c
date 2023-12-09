#include "hash.h"

bool isPrime(int x){
    for(int i = 2; i * i <= x; ++i){
        if(x % i == 0)
            return false;
    }
    return true;
}

int findnextprime(int x){
    for(int i = x + 1; ; ++i){
        if(isPrime(i))
            return i;
    }
}

long long int hash(char* x, int* primes, int n){
    long long int ans = 1, mod = 100003;
    for(int i = 0; i < n; ++i)
        ans = (ans * primes[x[i] - 'a']) % mod;
    return ans;
}

node** CreateHT(int itablesize){
    node** temp = (node**) malloc(sizeof(node*) * itablesize);
    for(int i = 0; i < itablesize; ++i){
        temp[i] = (node*) malloc(sizeof(node));
        temp[i]->nxt = NULL;
    }
    return temp;
}

void Insert(char* x, int tot, int pos, node** hashtable, int ss){
    node* cur = (node*) malloc(sizeof(node));
    cur->n = tot;
	cur->a = (char*) malloc(strlen(x) + 1);
    strcpy(cur->a, x);
	cur->index = ss;
    cur->nxt = hashtable[pos]->nxt;
    pthread_rwlock_init(&(cur->lock), NULL);
    hashtable[pos]->nxt = cur;
}

void Delete(char* x, int pos, node** hashtable) {
    node* cur = hashtable[pos]->nxt;
    node* pre = NULL;

    while (cur != NULL) {
        if (!strcmp(cur->a, x)) {
            if (pre == NULL) {
                hashtable[pos] = cur->nxt;
            } else {
                pre->nxt = cur->nxt;
            }

			printf("DEL: %s\n", cur->a);
			
            return;
        }

        pre = cur;
        cur = cur->nxt;
    }
}

int get(node** hashtable, char* cmp, int pos){
	node* here = hashtable[pos]->nxt;
	while(here != NULL){
		if(!strcmp(here->a, cmp))
			return here->index;
		here = here->nxt;
	}
	return -1;
}
