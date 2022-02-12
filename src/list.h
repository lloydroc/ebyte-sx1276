#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct ListElement
{
  void *data;
  struct ListElement *next;
};

struct List
{
  size_t size;
  int (*match)(void *data1, void *data2);
  void (*destroy)(void *data);
  struct ListElement *first;
  struct ListElement *last;
};

struct ListIterator
{
  struct List *list;
  struct ListElement *previous;
  struct ListElement *current;
  struct ListElement *next;
  bool skip_next;
};

void list_init(struct List *list, int (*match)(void *,void*), void (*destroy)(void *data));

size_t list_destroy(struct List *list);

int list_add_first(struct List *list, void *data);

int list_add_last(struct List *list, void *data);

void* list_get_first(struct List *list);

void* list_get_last(struct List *list);

void* list_get_index(struct List *list, int index);

void* list_get(struct List *list, void *data);

void list_foreach(struct List *list, void (*fun)(void *elem));

int list_contains(struct List *list, void *data);

int list_index_of(struct List *list, void *data);

int list_set(struct List *list, int index, void *data);

int list_remove(struct List *list, void *data);

int list_remove_index(struct List *list, int index);

int list_remove_first(struct List *list);

int list_remove_last(struct List *list);

size_t list_size(struct List* list);

void list_iter_init(struct List *list, struct ListIterator *iterator);

int list_iter_has_next(struct ListIterator *iterator);

void* list_iter_get(struct ListIterator *iterator);

void list_iter_next(struct ListIterator *iterator);

int list_iter_remove(struct ListIterator *iterator);

#endif
