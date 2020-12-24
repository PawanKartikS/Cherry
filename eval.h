#pragma once

#include "ast.h"
#include "symtbl.h"

typedef struct eval {
  symtbl_t *tbl;
  unsigned int depth;
} eval_t;

eval_t *init_eval(void);
int eval_prog(ast_t *ast, eval_t *eval);