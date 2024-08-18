#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "list.h"

List* list_create()
{
    List *list = (List*) malloc(sizeof(List));
    list->head = list->tail = NULL;
    list->size = 0;
    return list;
}

void list_free(List *list)
{
    if(list != NULL)
    {
        ListElem *curr = list->head;
        ListElem *next;
        while(curr != NULL)
        {
            next = curr->next;
            free(curr->key);
            free(curr);
            curr = next;
        }
        free(list);
    }
}

int list_find(List *list, const char *key)
{
    ListElem *curr = list->head;
    //printf("key name: %s\n", key);
    while(curr != NULL)
    {
        //printf("element name: %s\n", curr->key);
        if(strcmp(curr->key, key) == 0)
            return curr->val;
        curr = curr->next;
    }
    return -1;
}

void list_add(List *list, char *key, int val) // Changing list_add from void *val to int val
{
    // Allow duplicates
    // assert(list_find(list, key) == NULL);

    ListElem *elem = (ListElem*) malloc(sizeof(ListElem));
    elem->key = malloc(strlen(key) + 1);
    strncpy(elem->key, key, sizeof(key));
    //printf("elem->key %s\n", elem->key);
    elem->val = val;
    elem->next = NULL;

    if(list->tail == NULL) {
        list->head = list->tail = elem;
    } else {
        list->tail->next = elem;
        list->tail = elem;
    }

    list->size++;
}

void list_del(List *list, const char *key)
{
    // Remove the element with key 'key'
    ListElem *curr, *prev;

    curr = list->head;
    prev = NULL;
    while(curr != NULL)
    {
        if(strcmp(curr->key, key) == 0)
        {
            // Found it: now delete it

            if(curr == list->tail)
                list->tail = prev;

            if(prev == NULL)
                list->head = list->head->next;
            else
                prev->next = curr->next;

            list->size--;

            free(curr);
            return;
        }

        prev = curr;
        curr = curr->next;
    }
}

uint32_t list_size(const List *list)
{
    return list->size;
}


void list_change(List *list, const char *key, int amt)
{
    if(list == NULL)
        return;

    ListElem *curr = list->head;
    while(curr != NULL)
    {
        if(strcmp(curr->key, key) == 0)
        {
            curr->val = amt;
            return;
        }
        curr = curr->next;
    }

    return;
}