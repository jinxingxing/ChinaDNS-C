/*
 * hashset.h
 *
 *  Created on: Sep 30, 2014
 *      Author: jinxing
 */

#ifndef HASHSET_H_
#define HASHSET_H_

typedef struct elem_t_ {
	char *key;
	struct elem_t_ *next;
}elem_t;

typedef struct hashset_t_ {
	size_t size;
	size_t count;
	elem_t **elems;
} hashset_t;

typedef struct hashset_iterator_t_ {
	hashset_t *set;
	size_t index;
	elem_t *curr;
	elem_t *next;
}hashset_iterator_t;

hashset_t* new_hashset(size_t );
void free_hashset(hashset_t* );

size_t hashset_add_key(hashset_t *, char* );
elem_t* hashset_find(hashset_t *, char *);
void hashset_it(hashset_t *, hashset_iterator_t *);
elem_t* hashset_it_next(hashset_iterator_t *);
void hashset_map(hashset_t *, void* func_args, void (*func)(void *, elem_t *));
void hashset_print(hashset_t *);

#endif /* HASHSET_H_ */
