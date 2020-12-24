#include "args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"
#include "util.h"

token_t *pop_arg(list_t *args, const symtbl_t *symtbl, const int reqtype) {
  if (!symtbl) return 0;

  token_t *tk = pop_token(args, 0);
  if (!tk) return NULL;

  /* reqtype is -1 if we aren't sure of what type we'd be getting. */
  if (reqtype != -1 && tk->type != identifier && tk->type != reqtype) {
    fprintf(stderr, "args.c: invalid arg [%d instead of %d]\n", tk->type,
            reqtype);
    cleanup();
    _Exit(1);
  }

  if (tk->type != identifier) return tk;

  entry_t *e = get_symentry(symtbl, tk->tk);
  if (!e) return NULL;

  if (reqtype != -1 && e->vtype != reqtype) {
    fprintf(stderr, "args.c: invalid arg - [%s]\n", e->sym);
    cleanup();
    _Exit(1);
  }

  tk->type = e->vtype;
  tk->tk = e->val;
  return tk;
}

int ret_res(symtbl_t *symtbl, const void *val, const unsigned int type) {
  return_node_t *rnode = alloc(sizeof(return_node_t));
  if (!rnode) return 0;
  rnode->type = type;

  if (type == numeric) {
    rnode->val = alloc(sizeof(double));
    *(double *)(rnode->val) = *(double *)val;
  } else if (type == string) {
    rnode->val = alloc(512);
    strcpy((char *)rnode->val, (char *)val);
  } else {
    return 0;
  }

  return add(symtbl->retstack, rnode);
}