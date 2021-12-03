#include "list.h"
#include "assert.h"

#include <stdio.h>
#include <string.h>

/* A function to free an element of the linked list.
 * This is used when we remove elements from the list
 * so we don't get memory leaks
 */
void myfree(void *data)
{
  free(data);
}

/* A function to compare elements of the list.
 * This is used so we can compare if one element
 * is equal to another element. The return of 0
 * will indicate they are equal and all other
 * values will be not-equal.
 */
int match(void *vstr1, void* vstr2)
{
  char *str1 = (char *) vstr1;
  char *str2 = (char *) vstr2;
  int compare;
  if(vstr1 == NULL && vstr2 == NULL) return 0;
  if(vstr1 != NULL && vstr2 == NULL) return -1;
  if(vstr1 == NULL && vstr2 != NULL) return 1;
  compare = strcmp(str1,str2);
  return compare;
}

void test_list_iterator()
{
  struct List list;
  struct ListIterator iterator;

  int count;

  list_init(&list,match,myfree);
  list_iter_init(&list, &iterator);

  assert(list_iter_has_next(&iterator) == 0);
  assert(list_iter_remove(&iterator) == 0);

  // Make some data to put elements to put in the list
  char *elem0 = strdup("0");
  char *elem1 = strdup("1");
  char *elem2 = strdup("2");
  char *elem3 = strdup("3");
  char *elem4 = strdup("4");
  char *elem5 = strdup("5");
  char *elem6 = strdup("6");
  char *elem;

  list_iter_init(&list, &iterator);

  assert(list_iter_remove(&iterator) == 0);
  assert(list_size(&list) == 0);

  assert(list_add_last(&list,(void *)elem0) == 0);
  assert(list_add_last(&list,(void *)elem1) == 0);
  assert(list_add_last(&list,(void *)elem2) == 0);
  assert(list_add_last(&list,(void *)elem3) == 0);
  assert(list_add_last(&list,(void *)elem4) == 0);
  assert(list_add_last(&list,(void *)elem5) == 0);
  assert(list_add_last(&list,(void *)elem6) == 0);

  list_iter_init(&list, &iterator);
  count = 0;
  while(list_iter_has_next(&iterator))
  {
    elem = list_iter_get(&iterator);
    list_iter_next(&iterator);
    assert(strcmp(elem, (char *) list_get_index(&list,count)) == 0);
    printf("elem is %s\n", elem);
    count++;
  }

  assert(count == 7);

  list_iter_init(&list, &iterator);
  count = 0;
  while(list_iter_has_next(&iterator))
  {
    elem = list_iter_get(&iterator);

    printf("removing elem %s\n", elem);
    list_iter_remove(&iterator);

    list_iter_next(&iterator);
    count++;
  }

  assert(count == 7);
  assert(list_size(&list) == 0);

  assert(list.first == list.last);

  elem0 = strdup("0");
  elem1 = strdup("1");
  elem2 = strdup("2");
  elem3 = strdup("3");
  elem4 = strdup("4");
  elem5 = strdup("5");
  elem6 = strdup("6");

  assert(list_add_last(&list,(void *)elem0) == 0);
  assert(list_add_last(&list,(void *)elem1) == 0);
  assert(list_add_last(&list,(void *)elem2) == 0);
  assert(list_add_last(&list,(void *)elem3) == 0);
  assert(list_add_last(&list,(void *)elem4) == 0);
  assert(list_add_last(&list,(void *)elem5) == 0);
  assert(list_add_last(&list,(void *)elem6) == 0);

  /* remove the middle */
  list_iter_init(&list, &iterator);
  int middle = 3; // remove element 3
  int end = 6; // remove element 6, the 7th index but 3 was removed
  count = 0;
  while(list_iter_has_next(&iterator))
  {
    elem = list_iter_get(&iterator);

    if(count == middle)
    {
      printf("removing middle elem %s\n", elem);
      list_iter_remove(&iterator);
    }
    else if(count == end)
    {
      printf("removing end elem %s\n", elem);
      list_iter_remove(&iterator);
    }

    list_iter_next(&iterator);
    count++;
  }

  assert(list_size(&list) == 5);

  list_iter_init(&list, &iterator);
  while(list_iter_has_next(&iterator))
  {
    elem = list_iter_get(&iterator);
    printf("resulting elem %s\n", elem);
    list_iter_next(&iterator);
    count++;
  }

  assert(strcmp(list_get_last(&list), "5") == 0);
  assert(strcmp(list_get_index(&list, 3), "4") == 0);

  // remove from the tail each time until we reach the end
  list_iter_init(&list, &iterator);
  list_iter_next(&iterator);
  list_iter_next(&iterator);
  list_iter_next(&iterator);
  list_iter_next(&iterator);
  //list_iter_next(&iterator);
  list_iter_remove(&iterator);

  assert(list_size(&list) == 4);

  list_iter_init(&list, &iterator);
  list_iter_next(&iterator);
  list_iter_next(&iterator);
  list_iter_next(&iterator);
  //list_iter_next(&iterator);
  //list_iter_next(&iterator);
  list_iter_remove(&iterator);

  assert(list_size(&list) == 3);

  list_iter_init(&list, &iterator);
  list_iter_next(&iterator);
  list_iter_next(&iterator);
  //list_iter_next(&iterator);
  //list_iter_next(&iterator);
  //list_iter_next(&iterator);
  list_iter_remove(&iterator);

  assert(list_size(&list) == 2);

  list_iter_init(&list, &iterator);
  list_iter_next(&iterator);
  ////list_iter_next(&iterator);
  //list_iter_next(&iterator);
  //list_iter_next(&iterator);
  list_iter_remove(&iterator);

  assert(list_size(&list) == 1);

  list_iter_init(&list, &iterator);
  //list_iter_next(&iterator);
  ////list_iter_next(&iterator);
  //list_iter_next(&iterator);
  //list_iter_next(&iterator);
  list_iter_remove(&iterator);

  assert(list_size(&list) == 0);

  elem0 = strdup("0");

  assert(list_add_last(&list,(void *)elem0) == 0);

  // a common case where we remove the only element
  list_iter_init(&list, &iterator);
  while(list_iter_has_next(&iterator))
  {
    elem = list_iter_get(&iterator);
    printf("removing only elem %s\n", elem);
    list_iter_remove(&iterator);
    list_iter_get(&iterator);
  }

  assert(list_size(&list) == 0);

}

int main(int argc, char *argv[])
{
  // TODO maybe worth while to test with a list size of 1
  struct List list;

  // Make some data to put elements to put in the list
  char *elem0 = strdup("0");
  char *elem1 = strdup("1");
  char *elem2 = strdup("2");
  char *elem3 = strdup("3");
  char *elem4 = strdup("4");
  char *elem5 = strdup("5");
  char *elem6 = strdup("swap");

  // a pointer to a char that we'll use for utility
  char *telem;

  // intialize the list, we have to pass in a function
  // to free each element as well as a function to
  // compare elements
  list_init(&list,match,myfree);

  // check the size is zero
  assert(list_size(&list) == 0);

  // test removal with no elements
  assert(list_remove_first(&list) == -1);
  assert(list_remove_last(&list) == -1);

  // add elements at the end or "tail"
  assert(list_add_last(&list,(void *)elem0) == 0);
  assert(list_add_last(&list,(void *)elem1) == 0);
  assert(list_add_last(&list,(void *)elem2) == 0);
  assert(list_add_last(&list,(void *)elem3) == 0);
  assert(list_add_last(&list,(void *)elem4) == 0);
  assert(list_add_last(&list,(void *)elem5) == 0);

  // get the first element or the "head"
  telem = (char *) list_get_first(&list);
  assert(strcmp(telem,"0") == 0);

  // get the last element or the "tail"
  telem = (char *) list_get_last(&list);
  assert(strcmp(telem,"5") == 0);

  assert(list_size(&list) == 6);

  // find the index of elements in the list
  assert(list_index_of(&list,"0") == 0);
  assert(list_index_of(&list,"2") == 2);
  assert(list_index_of(&list,"5") == 5);
  assert(list_index_of(&list,"777") == -1);

  // get the elements
  assert(list_get(&list,"0"));
  assert(list_get(&list,"2"));
  assert(list_get(&list,"5"));
  assert(list_get(&list,"777") == NULL);

  // see if the list contains elements
  assert(list_contains(&list,"Larry") == 0);
  assert(list_contains(&list,NULL) == 0);
  assert(list_contains(&list,"0") == 1);
  assert(list_contains(&list,"5") == 1);

  // remove elements that are not in the list
  // will have a non-zero value
  assert(list_remove_index(&list, -1) == -1);
  assert(list_remove_index(&list, 6) == -1);

  // removing elements that are in the list
  // will return a zero value for success
  assert(list_remove_index(&list, 5) == 0);
  assert(list_remove(&list,"3") == 0);
  assert(list_remove(&list,"4") == 0);
  assert(list_remove_index(&list,0) == 0);
  assert(list_remove(&list,"1") == 0);

  // add and remove from the first and last
  int size = list_size(&list);
  assert(list_add_first(&list, strdup("first_removal_test")) == 0);
  assert(list_add_last(&list, strdup("last_removal_test")) == 0);
  assert(list_size(&list) == size+2);
  assert(list_index_of(&list,"first_removal_test") == 0);
  assert(list_index_of(&list,"last_removal_test") > 0);
  assert(list_remove_first(&list) == 0);
  assert(list_remove_last(&list) == 0);
  assert(list_index_of(&list,"first_removal_test") == -1);
  assert(list_index_of(&list,"last_removal_test") == -1);
  assert(list_size(&list) == size);

  // set a specific element in the list to data
  assert(list_set(&list,0,(void *)elem6) == 0);
  assert(list_contains(&list,elem6) == 1);
  assert(list_contains(&list,"2") == 0);

  assert(list_size(&list) == 1);

  // destroy the list, which will also free
  // all the elements so we don't have memory
  // leaks
  assert(list_destroy(&list) == 1);

  test_list_iterator();

  return 0;
}
