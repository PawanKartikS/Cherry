#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

void abort_runtime(void) {
  cleanup();
  fprintf(stderr, "list.c: abort_runtime() invoked\n");
  _Exit(1);
}

list_t *init_list(void) {
  list_t *list = alloc(sizeof(list_t));
  if (!list) return NULL;

  list->head = NULL;
  list->size = 0;
  return list;
}

int recursive_add(node_t **node, const void *buf) {
  if (*node) return recursive_add(&(*node)->next, buf);

  *node = alloc(sizeof(node_t));
  (*node)->data = (void *)buf;
  (*node)->next = NULL;

  return 1;
}

int add(list_t *list, const void *buf) {
  if (!list || !buf) return 0;

  list->size++;

  return recursive_add(&list->head, buf);
}

int add_front(list_t *list, const void *buf) {
  node_t *node = alloc(sizeof(node_t));
  if (!node || !list) return 0;

  node->data = (void *)buf;
  node->next = list->head;
  list->head = node;

  list->size++;
  return 1;
}

void *lookahead(const list_t *list) {
  if (!list || !list->head) abort_runtime();
  return list->head->next ? list->head->next->data : NULL;
}

void *recursive_peek(const node_t *head, const unsigned int i) {
  if (!head) return NULL;

  if (!i) {
    void *data = head->data;
    if (!data) abort_runtime();

    return data;
  }

  return recursive_peek(head->next, i - 1);
}

void *peek_front(const list_t *list) {
  if (!list) abort_runtime();

  return recursive_peek(list->head, 0);
}

void *peek_idx(const list_t *list, unsigned int idx) {
  if (!list || (idx >= list->size)) abort_runtime();

  return recursive_peek(list->head, idx);
}

void *peek_last(const list_t *list) {
  return !list || !list->head ? NULL
                              : recursive_peek(list->head, list->size - 1);
}

void *recursive_pop(node_t **head, node_t **buf, unsigned int i) {
  if (!*head) abort_runtime();

  if (!i) {
    void *data = (*head)->data;
    if (!data) abort_runtime();

    if (!buf) {
      *head = (*head)->next;
      return data;
    }

    (*buf)->next = (*head)->next;
    return data;
  }

  return recursive_pop(&(*head)->next, head, i - 1);
}

void *pop_front(list_t *list) {
  list->size--;
  return recursive_pop(&list->head, NULL, 0);
}

void *pop_last(list_t *list) {
  list->size--;
  return recursive_pop(&list->head, NULL, list->size);
}