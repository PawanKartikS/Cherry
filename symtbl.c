#include "symtbl.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "parse.h"
#include "token.h"
#include "util.h"

symtbl_t *init_symtbl(void) {
  symtbl_t *symtbl = alloc(sizeof(symtbl_t));
  assert(symtbl);

  symtbl->depth = 0;
  symtbl->frames = init_list();
  symtbl->fsigs = init_list();
  symtbl->retstack = init_list();
  symtbl->vscope = init_list();
  return symtbl;
}

int init_frame(symtbl_t *symtbl) {
  if (!symtbl) return 0;

  frame_t *frame = alloc(sizeof(frame_t));
  assert(frame);

  frame->entries = init_list();
  frame->defer_stack = init_list();
  return add(symtbl->frames, frame);
}

int init_globals(symtbl_t *symtbl) {
  if (!symtbl) return 0;

  int j = 0;
  const char *s[] = {"string", "numeric", "identifier"};

  for (int i = string; i <= identifier; i++, j++) {
    double d = i;
    token_t *tk = ptr_to_token(numeric, &d);
    assert(register_sym(symtbl, s[j], tk->tk, numeric, 1));
  }

  return 1;
}

fsig_t *get_fsig(const symtbl_t *symtbl, const char *func) {
  if (!symtbl) return NULL;

  for (node_t *sig = symtbl->fsigs->head; sig; sig = sig->next) {
    fsig_t *fsig = sig->data;
    if (!strcmp(fsig->func, func)) return fsig;
  }

  return NULL;
}

entry_t *get_symentry(const symtbl_t *symtbl, const char *sym) {
  frame_t *frame = peek_last(symtbl->frames);
  for (node_t *entry = frame->entries->head; entry; entry = entry->next) {
    if (!strcmp(((entry_t *)(entry->data))->sym, sym)) return entry->data;
  }

  return NULL;
}

int init_funcargs(symtbl_t *symtbl, const list_t *sargs, const list_t *fargs) {
  if (!symtbl || !sargs || !fargs) return 0;
  entry_t e[sargs->size];

  for (unsigned int i = 0; i < sargs->size; i++) {
    token_t *alias = peek_idx(sargs, i);
    token_t *val = peek_idx(fargs, i);

    strcpy(e[i].sym, alias->tk);
    if (val->type == identifier) {
      entry_t *t = get_symentry(symtbl, val->tk);
      assert(t);

      e[i].val = t->val;
      e[i].vtype = t->vtype;
      e[i].is_const = t->is_const;
    } else {
      if (val->type == numeric) {
        e[i].val = alloc(sizeof(double));
        *(double *)(e[i].val) = *(double *)val->tk;
      } else {
        e[i].val = val->tk;
      }

      e[i].vtype = val->type;
      e[i].is_const = 0;
    }
  }

  assert(init_frame((symtbl_t *)symtbl));
  for (unsigned int i = 0; i < sargs->size; i++) {
    assert(register_sym(symtbl, e[i].sym, e[i].val, e[i].vtype, e[i].is_const));
  }

  return 1;
}

int pop_frame(symtbl_t *symtbl) {
  if (!symtbl) return 0;

  return pop_last(symtbl->frames) != NULL;
}

int register_func(symtbl_t *symtbl, const char *func, const list_t *args,
                  const void *node) {
  if (!symtbl) return 0;
  if (is_reserved(func)) {
    fprintf(stderr, "symtbl.c: function name is a reserved keyword [%s]\n",
            func);
    return 0;
  }

  fsig_t *sig = alloc(sizeof(fsig_t));
  assert(sig);

  sig->func = (char *)func;
  sig->args = (list_t *)args;
  sig->node = (node_t *)node;
  return add(symtbl->fsigs, sig);
}

int register_sym(symtbl_t *symtbl, const char *sym, const void *val,
                 const unsigned int vtype, const unsigned int is_const) {
  if (!symtbl) return 0;
  if (is_reserved(sym)) {
    fprintf(stderr, "symtbl.c: sym is a reserved keyword [%s]\n", sym);
    return 0;
  }

  switch (vtype) {
    case none:
    case string:
    case identifier:
    case numeric:
    case glist:
    case gstack:
      break;
    default:
      fprintf(stderr, "symtbl.c: invalid vtype [%s - %d]\n", sym, vtype);
      return 0;
  }

  /**
   * Get a pointer to a slot into the symtbl. This pointer is NULL if we're
   * declaring a new variable.
   */
  entry_t *e = get_symentry(symtbl, sym);
  if (!e) {
    e = alloc(sizeof(entry_t));
  }

  if (e->is_const) {
    fprintf(stderr, "symtbl.c: sym is marked const, can't modify [%s]\n", sym);
    return 0;
  }

  strcpy(e->sym, sym);
  e->val = (void *)val;
  e->vtype = vtype;
  e->is_const = is_const;
  e->depth = symtbl->depth;

  frame_t *lframe = peek_last(symtbl->frames);
  return add(symtbl->vscope, e) && add(lframe->entries, e);
}

int scope_cleanup(symtbl_t *symtbl) {
  if (!symtbl) return 0;
  if (symtbl->vscope->size == 0) return 1;

  entry_t *e = peek_last(symtbl->vscope);
  if (!e) return 0;

  while (e) {
    if (symtbl->depth < e->depth) {
      /**
       * Do not free up this var, instead null the sym. This will prevent
       * get_symentry() from being able to query it. Any mem alloc'd will
       * be freed before quitting.
       */
      e->sym[0] = '\0';

      pop_last(symtbl->vscope);
      e = symtbl->vscope->size != 0 ? peek_last(symtbl->vscope) : NULL;
    } else {
      return 1;
    }
  }

  return 1;
}