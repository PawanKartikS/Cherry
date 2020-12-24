#pragma once

#include "list.h"
#include "node.h"
#include "token.h"

/* TODO: Use hashing rather than glist. */
typedef struct entry {
  char sym[512];
  /**
   * If this is a generic container, val is a list and vtype is glist.
   * Every element inside the list is arg_t as args are parsed by pop_arg in
   * args.c.
   */
  void *val;
  unsigned int depth, vtype, is_const;
} entry_t;

typedef struct frame {
  list_t *entries, *defer_stack;
} frame_t;

typedef struct fsig {
  char *func;
  void *node;
  list_t *args;
} fsig_t;

typedef struct symtbl {
  unsigned int depth;
  list_t *frames, *fsigs, *retstack, *vscope;
} symtbl_t;

symtbl_t *init_symtbl(void);
int init_frame(symtbl_t *symtbl);
int init_globals(symtbl_t *symtbl);
fsig_t *get_fsig(const symtbl_t *symtbl, const char *func);
entry_t *get_symentry(const symtbl_t *symtbl, const char *sym);
int init_funcargs(symtbl_t *symtbl, const list_t *sargs, const list_t *fargs);
int pop_frame(symtbl_t *symtbl);
int register_func(symtbl_t *symtbl, const char *func, const list_t *args,
                  const void *node);
int register_sym(symtbl_t *symtbl, const char *sym, const void *val,
                 const unsigned int vtype, const unsigned int is_const);
int scope_cleanup(symtbl_t *symtbl);