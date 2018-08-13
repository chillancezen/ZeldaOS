/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <lib/include/list.h>
#include <kernel/include/printk.h>


void
list_append(struct list_elem * head, struct list_elem * elem)
{
    ASSERT(!elem->prev);
    ASSERT(!elem->next);
    if (!head->next)
        head->next = elem;
    if (head->prev) {
        elem->prev = head->prev;
        head->prev->next = elem;
    }
    head->prev = elem;
}

void
list_prepend(struct list_elem * head, struct list_elem * elem)
{
    ASSERT(!elem->prev);
    ASSERT(!elem->next);
    if(head->next) {
        elem->next = head->next;
        head->next->prev = elem;
    }
    head->next = elem;
    if(!head->prev)
        head->prev = elem;
}

struct list_elem *
list_pop(struct list_elem * head)
{
    struct list_elem * _elem = NULL;
    if (list_empty(head))
        return NULL;
    ASSERT(head->prev);
    _elem = head->prev;
    head->prev = _elem->prev;
    if (_elem->prev) {
        _elem->prev->next = NULL;
    } else {
        head->next = NULL;
    }
    _elem->prev = NULL;
    ASSERT(!_elem->next);
    return _elem;
}

struct list_elem *
list_fetch(struct list_elem * head)
{
    struct list_elem * _elem = NULL;
    if(list_empty(head))
        return NULL;
    ASSERT(head->prev);
    _elem = head->next;
    head->next = _elem->next;
    if (_elem->next) {
        _elem->next->prev = NULL;
    } else {
        head->prev = NULL;
    }
    _elem->next = NULL;
    ASSERT(!_elem->prev);
    return _elem;
}

