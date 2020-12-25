#include "eval.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "expr.h"
#include "token.h"
#include "util.h"

int lno = 0, warns = 0;

return_node_t *get_fretval(eval_t *eval, const func_node_t *fnode);
int eval_node(const ast_node_t *node, eval_t *eval);
char *resolve_indx(const indx_node_t *ixnode, const symtbl_t *symtbl);
token_t *resolve_var(const symtbl_t *symtbl, const char *sym);

eval_t *init_eval(void) {
  eval_t *eval = alloc(sizeof(eval_t));
  assert(eval);

  eval->depth = 0;
  eval->tbl = init_symtbl();
  return eval;
}

/**
 * Do not resolve and over-write "buf" and "buf_type", rather return the
 * resolved value.
 *
 * Example -
 * Consider this situation:
 *
 * var i = 0
 * for i < 5
 *     print string[i: i + 3]
 * end
 *
 * If we resolve the lb & ub in the indx (when i = 0), we won't be able to
 * resolve it again for i = 1, 2, 3.. as "buf_type" is modified directly. In
 * that case lb & ub do not change with each iteration.
 */
token_t *resolve(void *buf, const unsigned int buf_type, const eval_t *eval) {
  if (buf_type == exprtree) {
    token_t *res = eval_exprtree(buf, eval->tbl);
    if (!res) return 0;

    return res;
  }

  if (buf_type == fretval) {
    return_node_t *fresult = get_fretval((eval_t *)eval, buf);
    if (!fresult) return 0;

    token_t *r = alloc(sizeof(token_t));
    r->tk = fresult->val;
    r->type = fresult->type;

    return r;
  }

  if (buf_type == identifier) return resolve_var(eval->tbl, buf);

  if (buf_type == indx) {
    indx_node_t *ixnode = buf;
    char *s = resolve_indx(ixnode, eval->tbl);
    if (!s) return 0;

    token_t *r = alloc(sizeof(token_t));
    r->tk = s;
    r->type = string;
    return r;
  }

  /* No need to resolve, so return buf & buf_type */
  token_t *t = alloc(sizeof(token_t));
  t->tk = buf;
  t->type = buf_type;

  return t;
}

char *resolve_indx(const indx_node_t *ixnode, const symtbl_t *symtbl) {
  assert(ixnode->ltype != -1 && ixnode->rtype != -1);

  token_t *arg = ixnode->arg;
  if (arg->type == identifier) {
    arg = resolve_var(symtbl, arg->tk);
    if (!arg) return 0;
  }

  if (arg->type != string) {
    fprintf(stderr, "eval.c: indexer cannot be applied to non-string\n");
    return NULL;
  }

  void *lb, *ub;
  unsigned int ltype, rtype;

  if (ixnode->ltype == exprtree) {
    token_t *t = eval_exprtree(ixnode->beg, symtbl);
    lb = t->tk;
    ltype = t->type;
  } else if (ixnode->ltype == identifier) {
    token_t *t = resolve_var(symtbl, ixnode->beg);
    if (!t) return NULL;

    lb = t->tk;
    ltype = t->type;
  } else {
    lb = ixnode->beg;
    ltype = ixnode->ltype;
  }

  if (ixnode->rtype == exprtree) {
    token_t *t = eval_exprtree(ixnode->end, symtbl);
    ub = t->tk;
    rtype = t->type;
  } else if (ixnode->rtype == identifier) {
    token_t *t = resolve_var(symtbl, ixnode->end);
    if (!t) return NULL;

    ub = t->tk;
    rtype = t->type;
  } else {
    ub = ixnode->end;
    rtype = ixnode->rtype;
  }

  if ((lb && ltype != numeric) || (ub && rtype != numeric)) {
    fprintf(stderr, "eval.c: indexer bounds must be numeric\n");
    return NULL;
  }

  double ubl = strlen((char *)arg->tk);
  double beg = lb ? *(double *)lb : +0;
  double end = ub ? *(double *)ub : ubl;
  end = ixnode->schar ? beg + 1 : end;

  if (end >= ubl) end = ubl;

  /* TODO: Better simplify this. */
  if (beg < 0 || end < 0 || end < beg || beg > ubl || end > ubl ||
      (ixnode->schar && beg >= ubl)) {
    fprintf(stderr,
            "eval.c: invalid indexer bounds; beg -> [%g] & end -> [%g]\n", beg,
            end);
    return NULL;
  }

  /* This part here does the magic - slicing */
  char *s = alloc(end - beg + 1);
  void *ptr = arg->tk;
  for (double i = 0; i < beg; i++) ptr++;

  memcpy(s, ptr, (end - beg) * sizeof(char));
  strcat(s, "\0");

  return s;
}

token_t *resolve_var(const symtbl_t *symtbl, const char *sym) {
  if (!symtbl) return NULL;
  entry_t *e = get_symentry(symtbl, sym);
  if (!e) return NULL;

  token_t *tk = alloc(sizeof(token_t));
  tk->tk = e->val;
  tk->type = e->vtype;

  return tk;
}

int compare(const token_t *lhs, const token_t *rhs, const char *op) {
  if (lhs->type != rhs->type && (lhs->type != none && rhs->type != none)) {
    fprintf(stderr,
            "eval.c: cannot compare for diff vals of [l/r]types in l[%d]\n",
            lno);
    return -1;
  }

  int null_check = (lhs->type == none || rhs->type == none);
  int buf_null = (!lhs->tk || !rhs->tk);

  if (buf_null && buf_null) {
    return 1;
  }

  if (lhs->type == numeric) {
    double a = *(double *)lhs->tk;
    double b = *(double *)rhs->tk;

    if (!strcmp(op, "<"))
      return a < b;
    else if (!strcmp(op, "<="))
      return a <= b;
    else if (!strcmp(op, "=="))
      return a == b;
    else if (!strcmp(op, "!="))
      return a != b;
    else if (!strcmp(op, ">"))
      return a > b;
    else if (!strcmp(op, ">="))
      return a >= b;
  }

  if (lhs->type == string) {
    char *a = (char *)lhs->tk;
    char *b = (char *)rhs->tk;

    if (!strcmp(op, "<"))
      return strcmp(a, b) < 0;
    else if (!strcmp(op, "<="))
      return strcmp(a, b) <= 0;
    else if (!strcmp(op, "=="))
      return strcmp(a, b) == 0;
    else if (!strcmp(op, "!="))
      return strcmp(a, b) != 0;
    else if (!strcmp(op, ">"))
      return strcmp(a, b) > 0;
    else if (!strcmp(op, ">="))
      return strcmp(a, b) >= 0;
  }

  return -1;
}

int eval_cond(const ast_node_t *node, const eval_t *eval) {
  cnode_t *cnode = node->ch;

  token_t *lhs = resolve(cnode->lhs, cnode->ltype, eval);
  token_t *rhs = resolve(cnode->rhs, cnode->rtype, eval);

  if (!lhs || !rhs) return -1;
  return compare(lhs, rhs, cnode->op);
}

int eval_decl(const ast_node_t *node, eval_t *eval) {
  decl_node_t *dnode = node->ch;

  token_t *rhs = resolve(dnode->rhs, dnode->rtype, eval);
  return register_sym(eval->tbl, dnode->lhs, rhs->tk, rhs->type,
                      dnode->is_const);
}

int eval_func(eval_t *eval, const char *func, const func_node_t *fnode) {
  if (!eval) return 0;

  fsig_t *sig = get_fsig(eval->tbl, func);
  if (!sig) return eval_builtin(fnode, eval->tbl);

  if (!strcmp(func, "main")) {
    assert(init_frame(eval->tbl));
    goto skipargs_init;
  }

  if (!fnode) goto skipargs_init;
  const int k_required = sig->args->size;
  const int k_have = fnode->args->size;

  if (k_required != k_have) {
    fprintf(stderr, "eval.c: %s() requires [%d] args, got [%d]\n", func,
            k_required, k_have);
    return 0;
  }

  if (!init_funcargs(eval->tbl, sig->args, fnode->args)) return 0;

skipargs_init:;
  /* TODO: move to global scope */
  assert(init_globals(eval->tbl));

  for (node_t *node = ((ast_node_t *)(sig->node))->lch->head; node;
       node = node->next) {
    const int ntype = ((ast_node_t *)(node->data))->type;
    int res = eval_node(node->data, eval);

    /**
     * eval_xxx() return -
     * +1 - all ok
     * +0 - err, but if/for return 0 if condition fails, for return 0 is okay
     *      because if we returned 1, we couldn't halt.
     * -1 - err, for return.
     */
    if (res <= 0) {
      if ((ntype == cond || ntype == floop || ntype == rettype) && res == 0) {
        /* TODO: recursion fix (bug!) */
        if (node->next && warns)
          fprintf(stderr, "eval.c: unreachable code in %s()\n", func);
        break;
      } else
        /* -ve return value, always an err */
        return 0;
    }
  }

  /* Evaluate deferred functions. */
  frame_t *lframe = peek_last(eval->tbl->frames);
  func_node_t *dfnode =
      lframe->defer_stack->size != 0 ? pop_last(lframe->defer_stack) : NULL;
  while (dfnode) {
    if (!eval_func(eval, dfnode->func, dfnode)) return 0;

    dfnode =
        lframe->defer_stack->size != 0 ? pop_last(lframe->defer_stack) : NULL;
  }

  return pop_frame(eval->tbl);
}

return_node_t *get_fretval(eval_t *eval, const func_node_t *fnode) {
  const unsigned int before = eval->tbl->retstack->size;
  if (!eval_func(eval, fnode->func, fnode)) {
    fprintf(stderr, "eval.c: could not call %s()\n", fnode->func);
    return 0;
  }

  if (eval->tbl->retstack->size == before) {
    fprintf(stderr, "eval.c: %s() did not return anything\n", fnode->func);
    return NULL;
  }

  return pop_last(eval->tbl->retstack);
}

int eval_print(const ast_node_t *node, eval_t *eval) {
  print_node_t *pnode = node->ch;
  token_t *arg = resolve(pnode->arg, pnode->type, eval);
  if (!arg) return 0;

  if (arg->type != numeric)
    printf("'%s'\n", (char *)arg->tk);
  else
    printf("%g\n", *(double *)arg->tk);

  return 1;
}

int eval_read(const ast_node_t *node, eval_t *eval) {
  read_node_t *rnode = node->ch;
  char *buf = alloc(512);
  memset(buf, 0, 512);

  scanf("%s", buf);
  return register_sym(eval->tbl, rnode->arg, buf, string, 0);
}

int eval_return(const ast_node_t *node, eval_t *eval) {
  return_node_t *rnode = node->ch;
  if (!rnode->val) return 0;

  token_t *arg = resolve(rnode->val, rnode->type, eval);
  if (!arg) {
    fprintf(stderr, "eval.c: could not execute return\n");
    return -1;
  }

  rnode->val = arg->tk;
  rnode->type = arg->type;

  if (!add(eval->tbl->retstack, rnode)) return -1;
  return 0;
}

int eval_unary(const ast_node_t *node, eval_t *eval,
               const unsigned int unary_type) {
  unary_node_t *unode = node->ch;
  entry_t *e = get_symentry(eval->tbl, unode->arg);

  if (!e) {
    fprintf(stderr, "eval.c: missing decl for sym [%s]\n", unode->arg);
    return 0;
  }

  if (e->vtype != numeric) {
    fprintf(stderr, "eval.c: unary cannot be used on non-numeric types [%s]\n",
            unode->arg);
    return 0;
  }

  switch (unary_type) {
    case post_dec:
      (*(double *)e->val) -= 1;
      break;
    case post_inc:
      (*(double *)e->val) += 1;
      break;
    default: {
      fprintf(stderr, "eval.c: eval_unary() fail, invalid unary_type\n");
      return 0;
    }
  }

  return register_sym(eval->tbl, unode->arg, e->val, e->vtype, 0);
}

int eval_node(const ast_node_t *node, eval_t *eval) {
  lno = node->lno;
  const char *kwd = node->kwd;

  switch (node->type) {
    case floop:
    case cond: {
      eval->depth++;
      eval->tbl->depth = eval->depth;

      int ccvars = 0;
      if (((cnode_t *)(node->ch))->ltype != exprtree &&
          ((cnode_t *)(node->ch))->ltype != fretval &&
          ((cnode_t *)(node->ch))->rtype != exprtree &&
          ((cnode_t *)(node->ch))->rtype != fretval && warns) {
        ccvars = 1;
        puts(
            "eval.c: condition depends on values that don't change at runtime");
      }

    eval:;
      const int evalres = eval_cond(node, eval);
      if (evalres < 0) {
        fprintf(stderr, "eval.c: could not eval if\n");
        return 0;
      }

      if (evalres && ccvars && !strcmp(kwd, "for")) {
        fprintf(stderr, "eval.c: for loop results in infinite loop\n");
        return 0;
      }

      if (evalres == 0 && !strcmp(kwd, "for")) goto gquit;
      node_t *ch = evalres == 1 ? node->lch->head : node->rch->head;

      for (; ch; ch = ch->next) {
        int res = eval_node(ch->data, eval);

        /**
         * Anything > than 0 is okay. 0 & -ve vals mean there might be something
         * wrong.
         */
        if (res <= 0) return res;
      }

      if (!strcmp(kwd, "for")) goto eval;

    gquit:;
      eval->depth--;
      eval->tbl->depth = eval->depth;
      if (!scope_cleanup(eval->tbl)) return 0;

      return 1;
    }

    case vdecl:
      return eval_decl(node, eval);

    case cin:
      return eval_read(node, eval);

    case cout:
      return eval_print(node, eval);

    case fcall: {
      if (!eval_func(eval, kwd, node->ch)) {
        fprintf(stderr, "eval.c: could not call %s()\n", kwd);
        cleanup();
        _Exit(1);
      }

      return 1;
    }

    case fdefer: {
      frame_t *lframe = peek_last(eval->tbl->frames);
      return add(lframe->defer_stack, node->ch);
    }

    case post_dec:
    case post_inc:
      return eval_unary(node, eval, node->type);

    case rettype:
      return eval_return(node, eval);

    default: {
      fprintf(stderr, "eval.c: could not eval [%s]\n", kwd);
      return 0;
    }
  }

  return 1;
}

int eval_prog(ast_t *ast, eval_t *eval) {
  if (!ast) return 0;
  if (eval_func(eval, "main", NULL) == 0) {
    fprintf(stderr, "main.c: error in line %d, program halted\n", lno);
    return 0;
  }

  return 1;
}