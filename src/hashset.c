/*
 * hashset.c
 *
 *  Created on: Sep 30, 2014
 *      Author: jinxing
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <resolv.h>
#include <math.h>
#include "hashset.h"

#define DEBUG_MSG(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#define HASH_TO_INDEX(hash, size) ((hash) % (size))
#define BUF_LEN	128

static elem_t* new_elem(char *);
static void free_elem(elem_t *);
static size_t hashset_add_elem(hashset_t *, elem_t *);
static int hashset_expand(hashset_t *, size_t );

static unsigned long sdbm_hash(const char *str){
	unsigned long hash = 0;
	int c;
	while((c = *str++))
		hash = c + (hash << 6) + (hash << 16) - hash;
	return hash;
}

static size_t g_calloc_cnt = 0;
static size_t g_calloc_total = 0;
static void *my_calloc(size_t n, size_t size){
	void *r = calloc(n, size);
	if(!r){
		perror("allocate failed");
		abort();
	}
	g_calloc_cnt += 1;
	g_calloc_total += size * n;
	return r;
}

static long g_free_cnt = 0;
static void my_free(void *block){
	free(block);
	g_free_cnt += 1;
}

static int get_prime(size_t num){
	size_t i = 0, c = 0;
	while(1){
		c = (size_t)sqrt(num);
		for(i=2; i<=c && num % i != 0; i++);
		if(i > c) break;
		num++;
	}
	return num;
}

static elem_t* new_elem(char *key){
	elem_t *elem = NULL;
	elem = my_calloc(1, sizeof(elem_t)+strlen(key)+1);
	elem->key = (char *)elem+sizeof(elem_t);
	strcpy(elem->key, key);

	elem->next = NULL;
	return elem;
}

static void free_elem(elem_t *elem){
	my_free(elem);
}

hashset_t* new_hashset(size_t size){
	hashset_t *nt = my_calloc(1, sizeof(hashset_t));
	size = get_prime(size);
	nt->size = size;
	nt->count = 0;
	nt->elems = my_calloc(size, sizeof(elem_t*));
	return nt;
}

void free_hashset(hashset_t* set){
	hashset_iterator_t it;
	elem_t *e = NULL;
	hashset_it(set, &it);
	while((e=hashset_it_next(&it)))
		free_elem(e);
	my_free(set->elems);
	my_free(set);
	return ;
}

static size_t hashset_add_elem(hashset_t *set, elem_t *elem){
	size_t index = 0, deep = 0;
	elem_t *p = 0;
	unsigned long hash = sdbm_hash(elem->key);

	elem->next = NULL;
	index = HASH_TO_INDEX(hash, set->size);
	p = set->elems[index];
	while(p){
		if(strcmp(p->key, elem->key) == 0){
			free_elem(elem);
			return index;
		}
		deep += 1;
		p = p->next;
	}

	if(deep > log2(set->size)){
		size_t new_size = get_prime(set->size*1.5);
		hashset_expand(set, new_size);
		index = HASH_TO_INDEX(hash, set->size);
	}

	elem->next = set->elems[index];
	set->elems[index] = elem;
	set->count++;
	return index;
}

size_t hashset_add_key(hashset_t *set, char* key){
	return hashset_add_elem(set, new_elem(key));
}

static int hashset_expand(hashset_t *set, size_t size){
	DEBUG_MSG("expand to %ld\n", size);
	hashset_t *nset = new_hashset(size);
	hashset_iterator_t it;
	elem_t *e = NULL;
	hashset_it(set, &it);
	while((e=hashset_it_next(&it)))
		hashset_add_elem(nset, e);

	set->size = nset->size;
	set->count = nset->count;
	my_free(set->elems);
	set->elems = nset->elems;
	my_free(nset);
	return 0;
}

elem_t* hashset_find(hashset_t *set, char *key){
	elem_t *p = 0;
	p = set->elems[HASH_TO_INDEX(sdbm_hash(key), set->size)];
	while(p){
		if(strcmp(p->key, key) == 0)
			return p;
		p = p->next;
	}
	return NULL;
}

void hashset_it(hashset_t *set, hashset_iterator_t *it){
	it->set = set;
	it->index = 0;
	it->curr = NULL;
	it->next = it->set->elems[0];
	return ;
}

elem_t* hashset_it_next(hashset_iterator_t *it){
	it->curr = it->next;
	while(1){
		if(it->curr){
			it->next = it->curr ? it->curr->next : NULL;
			break;
		}
		if(++(it->index) >= it->set->size){
			it->next = NULL;
			it->curr = NULL;
			break;
		}
		it->curr = it->set->elems[it->index];
	}

	return it->curr;
}

void hashset_map(hashset_t* set, void* func_args, void (*func)(void *, elem_t *)){
	hashset_iterator_t it;
	hashset_it(set, &it);
	elem_t *e = NULL;
	while((e=hashset_it_next(&it))){
		func(func_args, e);
	}
}

void hashset_print(hashset_t *set){
	elem_t *p = 0;
	size_t i = 0;
	size_t cfct_cnt = 0;
	size_t max_depth = 0, curr_depth =0;
	char *max_depth_key = NULL, *curr_key = NULL;
	for (i = 0; i < set->size; i++){
		p = set->elems[i];
		curr_depth = 0;
		while(p && p->key){
//			printf("key: %s, index: %d\n", p->key, i);
			curr_key = p->key;
			curr_depth++;
			p = p->next;
		}
		if(curr_depth){
			cfct_cnt++;
			if( curr_depth > max_depth){
				max_depth = curr_depth;
				max_depth_key = curr_key;
			}
		}
	}

	DEBUG_MSG("table count: %6ld/%-6ld, conflict rate: %-3.2lf%, usage: %-3.2lf%, "
			"max depth: %-2ld(key: %s)\n",
			set->count, set->size, cfct_cnt*100.0/set->size, set->count*100.0/set->size, max_depth, max_depth_key);
	DEBUG_MSG("table count: %ld, table size: %ld, mem: %.2lfK\n",
			set->count, set->size, sizeof(elem_t)*set->count/1024.0);
}
