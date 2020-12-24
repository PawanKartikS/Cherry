#pragma once

typedef struct node {
  void *data;
  struct node *next;
} node_t;

typedef struct list {
  unsigned int size;
  node_t *head;
} list_t;

list_t *init_list(void);
int add(list_t *list, const void *buf);
int add_front(list_t *list, const void *buf);
void *lookahead(const list_t *list);
void *peek_front(const list_t *list);
void *peek_idx(const list_t *list, const unsigned int idx);
void *peek_last(const list_t *list);
void *pop_front(list_t *list);
void *pop_last(list_t *list);