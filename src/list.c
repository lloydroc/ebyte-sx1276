#include "list.h"

void list_init(struct List *list, int (*match)(void *,void*), void (*destroy)(void *data))
{
  list->size = 0;
  list->match = match;
  list->destroy = destroy;
  list->first = NULL;
  list->last = NULL;
}

size_t list_destroy(struct List *list)
{
  size_t num_removed = 0;
  while(list_size(list) > 0)
  {
    list_remove_index(list, 0);
    num_removed++;
  }

  memset(list, 0, sizeof(struct List));
  return num_removed;
}

int list_add_first(struct List *list, void *data)
{
  struct ListElement *new_element;
  size_t element_size = sizeof(struct ListElement);

  new_element = (struct ListElement *) malloc(element_size);
  if(new_element == NULL)
  {
    return -1;
  }

  new_element->data = (void *)data;
  new_element->next = list->first;
  list->first = new_element;

  if(list_size(list) == 0)
  {
    list->last = new_element;
  }

  list->size++;
  return 0;
}

int list_add_last(struct List *list, void *data)
{
  struct ListElement *new_element;
  size_t element_size = sizeof(struct ListElement);

  new_element = (struct ListElement *) malloc(element_size);
  if(new_element == NULL)
  {
    return -1;
  }

  new_element->data = (void *)data;
  new_element->next = NULL;

  if(list_size(list) == 0)
  {
    list->first= new_element;
  }
  else
  {
    list->last->next = new_element;
  }

  list->last = new_element;

  list->size++;
  return 0;
}

void* list_get_first(struct List *list)
{
  if(list->size == 0)
  {
    return NULL;
  }

  return list->first->data;
}

void* list_get_last(struct List *list)
{
  if(list->size == 0)
  {
    return NULL;
  }

  return list->last->data;
}

void* list_get_index(struct List *list, int index)
{
  struct ListElement *current = list->first;
  size_t len = list->size;

  if(len == 0)
  {
    return NULL;
  }

  if(index < 0 || index >= len)
  {
    return NULL;
  }

  for(int i = 0; i < len; i++)
  {
    if(i == index)
    {
      return current->data;
    }
    current = current->next;
  }

  return NULL;
}

void list_foreach(struct List *list, void (*fun)(void *elem))
{
  if(fun == NULL)
    return;

  if(list == NULL)
    return;

  if(list_size(list) == 0)
    return;

  struct ListElement *current = list->first;
  do
  {
    fun(current->data);
    current = current->next;
  } while(current != NULL);

}

int list_contains(struct List *list, void *data)
{
  return list_index_of(list,data) != -1;
}

int list_index_of(struct List *list, void *data)
{
  struct ListElement *current = list->first;
  int match_result;
  int index = 0;

  if(current == NULL)
  {
    return -1;
  }

  if(list->match == NULL)
  {
    return -1;
  }

  do
  {
    match_result = list->match(data,current->data);
    if(!match_result)
    {
      return index;
    }
    index++;
    current = current->next;
  } while(current != NULL);

  return -1;
}

void* list_get(struct List *list, void *data)
{
  struct ListElement *current = list->first;
  int match_result;

  if(current == NULL)
  {
    return NULL;
  }

  if(list->match == NULL)
  {
    return NULL;
  }

  do
  {
    match_result = list->match(data, current->data);
    if(!match_result)
    {
      return current->data;
    }
    current = current->next;
  } while(current != NULL);

  return NULL;
}

int list_set(struct List *list, int index, void *data)
{
  struct ListElement *current = list->first;
  int count = 0;

  if(index < 0 || index >= list->size)
  {
    return -1;
  }

  while(current != NULL)
  {
    if(count == index)
    {
      list->destroy(current->data);
      current->data = data;
      return 0;
    }
    current = current->next;
    count++;
  }

  return -1;
}

int list_remove(struct List *list, void *data)
{
  struct ListElement *current = list->first;
  struct ListElement *prev = list->first;
  int match_result;

  if(current == NULL)
  {
    return -1;
  }

  if(list->match == NULL)
  {
    return -1;
  }

  do
  {
    match_result = list->match(data,current->data);
    if(match_result)
    {
      prev = current;
      current = current->next;
      continue;
    }

    // we've found a match
    if(list->destroy != NULL)
    {
      list->destroy(current->data);
    }

    if(list->size == 1)
    {
      list->first = NULL;
      list->last = NULL;
    }
    // removing the first
    else if(current == list->first)
    {
      list->first = current->next;
    }
    // removing the last
    else if(current == list->last)
    {
      list->last = prev;
      prev->next = NULL;
    }
    // removing the middle
    else
    {
      prev->next = current->next;
    }

    free(current);
    list->size--;

    return 0;
  } while(current != NULL);

  return -1;
}

int list_remove_index(struct List *list, int index)
{
  struct ListElement *current = list->first;
  struct ListElement *prev = list->first;
  int count = 0;

  if(current == NULL)
  {
    return -1;
  }

  if(index < 0 || index >= list->size)
  {
    return -1;
  }

  do
  {
    if(count != index)
    {
      count++;
      prev = current;
      current = current->next;
      continue;
    }

    // index matches
    if(list->destroy != NULL)
    {
      list->destroy(current->data);
    }

    if(list->size == 1)
    {
      list->first = NULL;
      list->last = NULL;
    }
    // removing the first
    else if(current == list->first)
    {
      list->first = current->next;
    }
    // removing the last
    else if(current == list->last)
    {
      list->last = prev;
      prev->next = NULL;
    }
    // removing the middle
    else
    {
      prev->next = current->next;
    }
    free(current);
    list->size--;
    return 0;

  } while(current != NULL);

  return -1;
}

int list_remove_first(struct List *list)
{
  return list_remove_index(list,0);
}

int list_remove_last(struct List *list)
{
  return list_remove_index(list,list->size-1);
}

size_t list_size(struct List *list)
{
  return list->size;
}

void list_iter_init(struct List *list, struct ListIterator *iterator)
{
  iterator->list = list;
  iterator->previous = NULL;
  iterator->current = list->first;
  iterator->skip_next = false;

  if(iterator->list->size)
    iterator->next = list->first->next;
  else
    iterator->next = NULL;
}

int list_iter_has_next(struct ListIterator *iterator)
{
  if(iterator->current == NULL)
    return 0;
  return 1;
}

void* list_iter_get(struct ListIterator *iterator)
{
  if(iterator->current ==  NULL)
    return NULL;

  return iterator->current->data;
}

void list_iter_next(struct ListIterator *iterator)
{
  if(iterator->current == NULL)
    return;

  if(iterator->skip_next == true)
  {
    iterator->skip_next = false;
    return;
  }

  iterator->previous = iterator->current;
  iterator->current = iterator->next;

  if(iterator->current != NULL)
    iterator->next = iterator->next->next;
}

int list_iter_remove(struct ListIterator *iterator)
{
  if(iterator->list->size == 0)
  {
    return 0;
  }

  if(iterator->current == 0)
  {
    return 0;
  }

  /* we're removing the first element */
  if(iterator->current == iterator->list->first)
  {
    iterator->list->first = iterator->current->next;
  }

  /* we're removing the last element */
  if(iterator->current == iterator->list->last)
  {
    iterator->list->last = iterator->previous;
  }

  iterator->list->destroy(iterator->current->data);
  free(iterator->current);

  iterator->current = iterator->next;

  if(iterator->current)
    iterator->next = iterator->current->next;

  if(iterator->previous)
    iterator->previous->next = iterator->current;

  iterator->skip_next = true;
  iterator->list->size--;

  return 0;
}
