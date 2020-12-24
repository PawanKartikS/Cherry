#include "util.h"

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

static node_t *allocs = NULL;
static node_t *avail = NULL;
static unsigned int total_allocs = 0;
static unsigned long total_mem_alloc = 0;

/**
 * Because Cherry uses malloc() extensively in different parts of the codebase,
 * it is difficult to track and implement cleanup procedures. That is why we use
 * alloc(), a wrapper over malloc() that keeps track of all the heap allocs in
 * form of a linked list. Once we're done executing the program, we simply call
 * cleanup() that calls free() to clear up every heap alloc in O(N) where N =
 * num of allocs. This allows Cherry to quit gracefully without any memory
 * leaks.
 *
 * Caution: Do not attempt to manually call free() on any mem alloc'd by
 * alloc().
 */

void *alloc(const unsigned long size) {
  void *ptr = NULL;

  if (avail) {
    node_t *p = avail->data;
    ptr = avail->data;

    free(p);

    p = avail;
    avail = avail->next;

    /**
     * Since we've allocated memory to a node in the free list, we can dispose
     * this and move to the next node.
     */
    free(p);
  }

  ptr = malloc(size);

  if (!ptr) {
    fprintf(stderr, "util.c: malloc() fail!\n");
    cleanup();
    _Exit(1);
  }

  total_allocs++;
  total_mem_alloc += size;

  node_t *n = malloc(sizeof(node_t));
  n->data = ptr;
  n->next = allocs;
  allocs = n;

  return ptr;
}

void cleanup(void) {
  node_t *ptr = allocs;

  /**
   * Free up 2 lists -
   * 1. Alloc'd memory   (list)
   * 2. Available memory (list)
   */

  while (ptr) {
    node_t *p = ptr;
    ptr = ptr->next;

    free(p->data);
    free(p);

    total_allocs--;
  }

  if (total_allocs != 0)
    fprintf(stderr, "util.c: total_allocs != 0 [%d]\n", total_allocs);

  printf("util.c: clearing heap, total mem alloc'd for runtime: %ld bytes\n",
         total_mem_alloc);

  ptr = avail;
  while (ptr) {
    node_t *p = ptr;
    ptr = ptr->next;

    free(p);
  }
}

/* Mark the specified resource as free */
void mark_free(const void *fptr) {
  node_t *p = allocs;
  while (p) {
    if (p->data == fptr) break;
    p = p->next;
  }

  if (!p) {
    /* We could not find the entry? Internal error perhaps? */
    fprintf(stderr, "util.c: mark_free() fail\n");
    return;
  }

  /**
   * Free the memory associated because this object is no longer required.
   * However, we let it stay in the list because it might be used later and
   * might require to be cleaned up when we quit.
   */
  free(p->data);
  p->data = NULL;

  /* Insert this ptr into free */
  node_t *fmem = malloc(sizeof(node_t));
  fmem->data = p->data;
  fmem->next = avail;
  avail = fmem;
}