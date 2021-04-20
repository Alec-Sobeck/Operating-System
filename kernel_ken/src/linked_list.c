#include "linked_list.h"
#include "kheap.h"
#include "common.h"

list_t *list_init()
{
    list_t *list = kmalloc(sizeof(*list));
    list->front = NULL;
    list->back = NULL;
    return list;
}

struct linked_list_node *list_node_init(void *value)
{
    struct linked_list_node *node = kmalloc(sizeof(*node));
    node->value = value;
    node->next = NULL;
    node->previous = NULL;
    return node;
}

int list_destroy(list_t *list)
{
    if (!list)
        return 0;

    if (list->front) {
        struct linked_list_node *previous = list->front;
        struct linked_list_node *current = previous->next;

        while (current) {
            kfree(previous);
            previous = current;
            current = current->next;
        }
        kfree(previous);
    }
    kfree(list);
    return 0;
}

int list_add_back(list_t *list, void *value)
{
    if (!list)
        return -1;

    struct linked_list_node *node = list_node_init(value);

    if (!list->front) {
        list->front = node;
        list->back = node;
    } else {
        node->previous = list->back;
        list->back->next = node;
        list->back = node;
    }

    return 0;
}

int list_add_front(list_t *list, void *value)
{
    if (!list)
        return -1;

    struct linked_list_node *node = list_node_init(value);

    if (!list->front) {
        list->front = node;
        list->back = node;
    } else {
        node->next = list->front;
        list->front->previous = node;
        list->front = node;
    }

    return 0;
}

void *list_remove_front(list_t *list)
{
    if(list_is_empty(list))
        return NULL;

    struct linked_list_node *front = list->front;

    list->front = list->front->next;
    if (list->front)
        list->front->previous = NULL;

    if (!list->front)
        list->back = NULL;

    void *res = front->value;
    kfree(front);

    return res;
}

void list_remove_node(list_t *list, struct linked_list_node *node)
{
    if(node->previous == NULL && node->next == NULL){
        // Only 1 thing in the list.
        list->back = NULL;
        list->front = NULL;
    } else if(node->previous == NULL){
        // Remove head
        list->front = list->front->next;
        list->front->previous = NULL;
    } else if(node->next == NULL) {
        // Remove end
        list->back = list->back->previous;
        list->back->next = NULL;
    } else {
        node->previous->next = node->next;
        node->next->previous = node->previous;
    }
    kfree(node);
}

void *list_remove_back(list_t *list)
{
    if(list_is_empty(list))
        return NULL;

    struct linked_list_node *back = list->back;

    list->back = list->back->previous;
    if (list->back)
        list->back->next = NULL;

    if (!list->back)
        list->front = NULL;

    void *res = back->value;
    kfree(back);

    return res;
}

int list_is_empty(list_t *list)
{
    if (!list)
        return 1;
    if (!list->front)
        return 1;

    return 0;
}

int list_contains(list_t *list, void *value, int (*cmp)(void*, void*))
{
    struct linked_list_node *node = list->front;
    while (node) {
        if (cmp(node->value, value) == 0) {
            return 1;
        }
        node = node->next;
    }

    return 0;
}

void *list_get(list_t *list, void *value, int(*cmp)(void*, void*))
{
    struct linked_list_node *node = list->front;
    while (node) {
        if (cmp(node->value, value) == 0) {
            return node->value;
        }
        node = node->next;
    }

    return 0;
}

int list_size(list_t *list)
{
    if(!list)
        return 0;
    int size = 0;
    struct linked_list_node *node = list->front;
    while(node){
        node = node->next;
        size++;
    }
    return size;
}

int list_remove(list_t *list, void *value, int (*cmp)(void*, void*))
{
    struct linked_list_node *node = list->front;
    while (node) {
        if (cmp(node->value, value) == 0) {

            list_remove_node(list, node);
            return 0;
        }
        node = node->next;
    }

    return 1;
}

void list_foreach(list_t *list, void (func)(void*))
{
    struct linked_list_node *node = list->front;
    while(node){
        func(node->value);
        node = node->next;
    }

}


void list_clear(list_t *list)
{
    if (!list)
        return;

    if (list->front) {
        struct linked_list_node *previous = list->front;
        struct linked_list_node *current = previous->next;

        while (current) {
            kfree(previous);
            previous = current;
            current = current->next;
        }
        kfree(previous);
    }

    list->front = NULL;
    list->back = NULL;
}


list_enumerator_t list_get_enumerator(list_t *list)
{
    list_enumerator_t enumerator;
    enumerator.current = list->front;
    return enumerator;
}

void *list_next_value(list_enumerator_t *enumerator)
{
    if(!enumerator || !enumerator->current) {
        return NULL;
    }
    void *val = enumerator->current->value;
    enumerator->current = enumerator->current->next;
    return val;
}

int list_has_next(list_enumerator_t *enumerator)
{
    if(!enumerator || !enumerator->current){
        return FALSE;
    }
    return TRUE;
}
