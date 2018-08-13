/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _LIST_H
#define _LIST_H
#include <lib/include/types.h>

struct list_elem {
    struct list_elem * prev;
    struct list_elem * next;
}__attribute__((packed));


#define list_init(head) {\
    (head)->prev = NULL; \
    (head)->next = NULL; \
}

#define list_empty(head) (!((head)->next))

/*
 * Put an element at the tail of the list
 */
void list_append(struct list_elem * head, struct list_elem * elem);
/*
 * Put an element at the front of the list
 */ 
void list_prepend(struct list_elem * head, struct list_elem * elem);
/*
 * Pop an element at the tail of the list
 * return NULL if the list is empty
 */
struct list_elem * list_pop(struct list_elem * head);
/*
 * fetch an element at the front of the list
 * return NULL if the list is empty
 */
struct list_elem * list_fetch(struct list_elem * head);

#define LIST_FOREACH_START(head, elem) { \
    struct list_elem * __elem = (head)->next; \
    for(; __elem; __elem = __elem->next) { \
        (elem) = __elem;

#define LIST_FOREACH_END() }}


#endif
