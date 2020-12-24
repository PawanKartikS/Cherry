#pragma once

#include "list.h"

enum {
  cond = 31,
  vdecl,
  exprtree,
  fdecl,
  fcall,
  fdefer,
  floop,
  cin,
  cout,
  indx,
  post_dec,
  post_inc,
  rettype,
  nreq,
  cbd
};

typedef struct token token_t;

typedef struct bnode {
  token_t *val;
  void *lhs, *rhs;
} binary_node_t;

typedef struct cnode {
  char *op;
  void *lhs, *rhs;
  unsigned int ltype, rtype;
} cnode_t;

typedef struct decl_node {
  char *lhs;
  void *rhs;
  unsigned int ltype, rtype, is_const;
} decl_node_t;

typedef struct func_node {
  int defer;
  char *func;
  list_t *args;
} func_node_t;

typedef struct {
  token_t *arg;
  void *beg, *end;
  unsigned int schar, ltype, rtype;
} indx_node_t;

typedef struct print_node {
  void *arg;
  unsigned int type;
} print_node_t;

typedef struct read_node {
  char *arg;
} read_node_t;

typedef struct return_node {
  void *val;
  unsigned int type;
} return_node_t;

typedef struct unary_node {
  char *arg;
} unary_node_t;

typedef struct ast_node {
  char *kwd;
  void *ch;
  int type, lno;
  list_t *tokens, *lch, *rch;
} ast_node_t;

ast_node_t *init_node(list_t *list);