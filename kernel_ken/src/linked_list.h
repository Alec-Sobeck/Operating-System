#ifndef LINKED_LIST_H
#define LINKED_LIST_H

struct linked_list_node
{
    struct linked_list_node *next;
    struct linked_list_node *previous;
    void *value;
};

typedef struct
{
    struct linked_list_node *front;
    struct linked_list_node *back;
} list_t;

typedef struct
{
    struct linked_list_node *current;
} list_enumerator_t;

list_t *list_init();
int list_destroy(list_t *list);
int list_add_back(list_t *list, void *value);
int list_add_front(list_t *list, void *value);
void *list_remove_front(list_t *list);
void *list_remove_back(list_t *list);
int list_is_empty(list_t *list);
int list_contains(list_t *list, void *value, int(*cmp)(void*, void*));
int list_remove(list_t *list, void *value, int(*cmp)(void*, void*));
int list_size(list_t *list);
void list_remove_node(list_t *list, struct linked_list_node *node);
void *list_get(list_t *list, void *value, int(*cmp)(void*, void*));
void list_foreach(list_t *list, void (func)(void*));
void list_clear(list_t *list);
// Note: changes to the list invalidate any enumerators.
list_enumerator_t list_get_enumerator(list_t *list);
void *list_next_value(list_enumerator_t *enumerator);
int list_has_next(list_enumerator_t *enumerator);


#endif