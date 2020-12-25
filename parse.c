#include "parse.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expr.h"
#include "util.h"

int parse_func(void **buf, list_t *tokens);
int parse_indx(void **buf, list_t *tokens);

list_t *parse_arglist(list_t *tokens, const unsigned int onlyvar) {
  /* Set onlyvar > 0 if only vars must be in the arglist. */

  list_t *args = init_list();
  token_t *lpr = pop_token(tokens, 0);

  assert(match_token(lpr, "("));

  int idx = 0;
  token_t *arg;
  while (tokens->size) {
    arg = pop_token(tokens, 1);
    if (!strcmp(arg->tk, ",")) {
      if (idx % 2 != 1) {
        fprintf(stderr, "parse.c: invalid position in arglist [,]\n");
        cleanup();
        _Exit(1);
      }
    } else if (strcmp(arg->tk, ")")) {
      switch (arg->type) {
        case identifier:
          break;
        case string:
          if (onlyvar) goto invarg;
          break;
        case numeric:
          if (onlyvar) goto invarg;
          break;

        default:
          goto invarg;
      }

      assert(add(args, arg));
    } else if (!strcmp(arg->tk, ")"))
      break;

    idx++;
  }

  if (idx % 2 != 1 && args->size > 0) {
    fprintf(stderr, "parse.c: invalid position in arglist [)] [%d]\n", idx);
    cleanup();
    _Exit(1);
  }

  return args;

invarg:;
  fprintf(stderr, "parse.c: invalid arg in arglist [%s]\n", (char *)arg->tk);
  cleanup();
  _Exit(1);
}

int parse_next(list_t *tokens, void **buf, unsigned int *type) {
  token_t *ft = peek_front(tokens);
  token_t *la = tokens->size > 1 ? lookahead(tokens) : NULL;

  if (ft->type == identifier && la && !strcmp(la->tk, "(")) {
    if (parse_func(buf, tokens) >= 0) {
      *type = fretval;
      return 1;
    }
  }

  if ((ft->type == identifier || ft->type == string) &&
      (la && !strcmp(la->tk, "["))) {
    if (parse_indx(buf, tokens) >= 0) {
      *type = indx;
      return 1;
    }
  }

  const unsigned int size = tokens->size;
  int ret = to_exprtree(tokens, buf, type);

  /**
   * If only 1 token was consumed; reduce un-necessary calls to eval_exptree().
   */
  if (size - tokens->size == 1) {
    *buf = ft->tk;
    *type = ft->type;

    if (match_token(ft, "none")) {
      *buf = NULL;
      *type = none;
    }
  }

  return ret;
}

int parse_cond(const char *kwd, void **buf, list_t *tokens) {
  /**
   * if <lhs> <op> <rhs>
   * lhs -> [idf/str/numeric/function result]
   * rhs -> [idf/str/numeric/function result]
   * op  -> [<, >, ==, !=, <=, >=]
   */

  pop_front(tokens);
  cnode_t *cnode = alloc(sizeof(cnode_t));

  /* Parse LHS */
  if (!parse_next(tokens, &cnode->lhs, &cnode->ltype)) {
    fprintf(stderr, "parse.c: could not parse LHS for condition\n");
    return -1;
  }

  /* Parse operator */
  token_t *op = pop_token(tokens, 0);

  if (op->type != operator) {
    fprintf(stderr, "parse.c: invalid operator to condition\n");
    return -1;
  }

  cnode->op = op->tk;

  const char *vops[] = {"<", ">", "==", "!=", ">=", "<="};
  int i = 0;
  for (i = 0; i < 6; i++) {
    if (!strcmp(cnode->op, vops[i])) break;
  }

  if (i >= 6) {
    fprintf(stderr, "parse.c: invalid operator to condition [%s]\n", cnode->op);
    return -1;
  }

  /* Parse RHS */
  if (!parse_next(tokens, &cnode->rhs, &cnode->rtype)) {
    fprintf(stderr, "parse.c: could not parse RHS for condition\n");
    return -1;
  }

  *buf = cnode;
  return strcmp(kwd, "if") == 0 ? cond : floop;
}

/**
 * Caution!
 * If parse_xxx() fails, *buf remains NULL. Do not dereference it!
 * Check the return value first!
 */

int parse_decl(const char *kwd, void **buf, list_t *tokens) {
  /**
   * var idf = val
   * val -> [idf/str/numeric/function result]
   */

  pop_front(tokens);
  const int is_const = strcmp(kwd, "const") == 0;
  if (is_const) pop_front(tokens);

  decl_node_t *decl = alloc(sizeof(decl_node_t));

  token_t *lhs = pop_token(tokens, 0);
  if (lhs->type != identifier) {
    fprintf(stderr, "parse.c: LHS to var must be an identifier\n");
    return -1;
  }

  decl->lhs = (char *)lhs->tk;

  decl->is_const = is_const;
  token_t *op_node = pop_front(tokens);

  /* Simple decl */
  if (!strcmp(op_node->tk, "=")) {
    if (!parse_next(tokens, &decl->rhs, &decl->rtype)) {
      fprintf(stderr, "parse.c: could not parse RHS for var [%d]\n",
              decl->rtype);
      return -1;
    }
  } else if (!strcmp(op_node->tk, ":")) {
    /* No init available */
    token_t *rtype = pop_token(tokens, 0);
    if (!rtype) {
      fprintf(stderr, "parse.c: RHS to var (init) is null\n");
      return -1;
    }

    if (!strcmp(rtype->tk, "int")) {
      decl->rtype = numeric;
      decl->rhs = alloc(sizeof(double));
      *(double *)decl->rhs = 0;
    } else if (!strcmp(rtype->tk, "str")) {
      decl->rtype = string;
      decl->rhs = alloc(512);
      memset(decl->rhs, 0, 512);
    } else if (!strcmp(rtype->tk, "glist")) {
      decl->rtype = glist;
      decl->rhs = init_list();
    } else if (!strcmp(rtype->tk, "gstack")) {
      decl->rtype = gstack;
      decl->rhs = init_list();
    } else {
      fprintf(stderr, "parse.c: invalid rtype for RHS (init)\n");
      return -1;
    }

    *buf = decl;
    return vdecl;
  } else {
    fprintf(stderr, "parse.c: invalid operator for var [%s]\n",
            (char *)op_node->tk);
    return -1;
  }

  *buf = decl;
  return vdecl;
}

int parse_func(void **buf, list_t *tokens) {
  int defer = 0;
  unsigned int ret = -1;

  token_t *func;
  token_t *kwd = peek_front(tokens);
  if (!strcmp(kwd->tk, "defer")) {
    defer = 1;
    ret = fdefer;
    pop_front(tokens);

    kwd = peek_front(tokens);
  }

  if (!strcmp(kwd->tk, "def")) {
    if (defer) {
      fprintf(stderr, "parse.c: unexpected defer keyword\n");
      return -1;
    }

    ret = fdecl;
    pop_front(tokens);
    func = pop_front(tokens);
  } else {
    func = kwd;
    if (ret == -1) ret = fcall;

    pop_front(tokens);
  }

  if (func->type != identifier) {
    fprintf(stderr, "parse.c: invalid function identifier\n");
    return -1;
  }

  if (!strcmp(func->tk, "defer")) {
    fprintf(stderr, "parse.c: unexpected defer keyword\n");
    return -1;
  }

  func_node_t *fnode = alloc(sizeof(func_node_t));
  if (!fnode) return -1;

  fnode->func = func->tk;
  fnode->args = parse_arglist(tokens, ret == fdecl);
  if (fnode->args == NULL) return -1;

  *buf = fnode;
  return ret;
}

int parse_indx(void **buf, list_t *tokens) {
  indx_node_t *ixnode = alloc(sizeof(indx_node_t));
  ixnode->schar = 0;
  ixnode->beg = NULL;
  ixnode->end = NULL;

  token_t *arg = pop_front(tokens);
  switch (arg->type) {
    case identifier:
    case string:
      break;
    default:
      fprintf(stderr, "parse.c: invalid token before indexer\n");
      return 0;
  }

  ixnode->arg = arg;
  assert(match_token(pop_front(tokens), "["));

  token_t *tk = peek_front(tokens);

  /* lower bound specified */
  if (tk->type != syntax && tk->type != sqbr) {
    if (!parse_next(tokens, &ixnode->beg, &ixnode->ltype)) {
      fprintf(stderr, "parse.c: could not parse lb@indexer\n");
      return 0;
    }

    tk = peek_front(tokens);
  }

  if (tk->type == syntax) {
    ixnode->schar = 0;
    assert(match_token(tk, ":"));
    /* Consume colon */
    pop_front(tokens);
  } else {
    ixnode->schar = 1;
  }

  tk = peek_front(tokens);

  /* upper bound specified */
  if (tk->type != sqbr) {
    if (!parse_next(tokens, &ixnode->end, &ixnode->rtype)) {
      fprintf(stderr, "parse.c: could not parse ub@indexer\n");
      return 0;
    }

    tk = peek_front(tokens);
  }

  assert(match_token(tk, "]"));
  /* Consume sqbr */
  pop_front(tokens);

  *buf = ixnode;
  return indx;
}

int parse_print(void **buf, list_t *tokens) {
  /**
   * print arg
   * arg -> [idf/str/numeric/function result]
   */

  pop_front(tokens);
  print_node_t *pnode = alloc(sizeof(print_node_t));

  if (!parse_next(tokens, &pnode->arg, &pnode->type)) {
    fprintf(stderr, "parse.c: could not parse arg to print\n");
    return -1;
  }

  *buf = pnode;
  return cout;
}

int parse_read(void **buf, list_t *tokens) {
  /**
   * read arg
   * arg -> [var]
   */

  pop_front(tokens);
  token_t *arg = pop_token(tokens, 0);
  if (arg->type != identifier) {
    fprintf(stderr, "parse.c: could not parse arg to read\n");
    return -1;
  }

  read_node_t *read_node = alloc(sizeof(read_node_t));
  assert(read_node);

  read_node->arg = arg->tk;

  *buf = read_node;
  return cin;
}

int parse_return(void **buf, list_t *tokens) {
  /**
   * return val
   * val -> [idf/str/numeric/function result]
   */

  pop_front(tokens);
  return_node_t *rnode = alloc(sizeof(return_node_t));

  if (tokens->size == 0) {
    rnode->val = NULL;
    *buf = rnode;
    return rettype;
  }

  if (!parse_next(tokens, &rnode->val, &rnode->type)) {
    fprintf(stderr, "parse.c: could not parse arg to return\n");
    return -1;
  }

  *buf = rnode;
  return rettype;
}

int parse_skwd(void **buf, list_t *tokens) {
  token_t *kwd = pop_token(tokens, 0);
  if (!strcmp(kwd->tk, "else") || !strcmp(kwd->tk, "end")) {
    return nreq;
  }

  return cbd;
}

int parse_unary(void **buf, list_t *tokens, const unsigned int unarytype) {
  token_t *arg = pop_token(tokens, 0);
  if (arg->type != identifier) {
    fprintf(stderr, "parse.c: invalid arg to unary operator\n");
    return -1;
  }

  pop_token(tokens, 0);

  unary_node_t *unode = alloc(sizeof(unary_node_t));

  /* TODO: replace post_inc with last enum! */
  if (!unode || unarytype < post_dec || unarytype > post_inc) return -1;

  unode->arg = arg->tk;
  *buf = unode;
  return unarytype;
}

int parse(void **buf, list_t *tokens) {
  const char *kwd = ((token_t *)(tokens->head->data))->tk;
  if (!strcmp(kwd, "const") || !strcmp(kwd, "var"))
    return parse_decl(kwd, buf, tokens);
  if (!strcmp(kwd, "def")) return parse_func(buf, tokens);
  if (!strcmp(kwd, "defer")) return parse_func(buf, tokens);
  if (!strcmp(kwd, "for")) return parse_cond(kwd, buf, tokens);
  if (!strcmp(kwd, "if")) return parse_cond(kwd, buf, tokens);
  if (!strcmp(kwd, "print")) return parse_print(buf, tokens);
  if (!strcmp(kwd, "read")) return parse_read(buf, tokens);
  if (!strcmp(kwd, "return")) return parse_return(buf, tokens);
  if (!strcmp(kwd, "else") || !strcmp(kwd, "end"))
    return parse_skwd(buf, tokens);

  token_t *tk = lookahead(tokens);
  if (!tk) goto pfail;

  if (!strcmp(tk->tk, "(")) return parse_func(buf, tokens);
  if (!strcmp(tk->tk, "--")) return parse_unary(buf, tokens, post_dec);
  if (!strcmp(tk->tk, "++")) return parse_unary(buf, tokens, post_inc);

pfail:;
  fprintf(stderr, "parse.c: could not parse for kwd [%s]\n", kwd);
  return -1;
}