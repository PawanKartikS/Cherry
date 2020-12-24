#pragma once

#include "list.h"
#include "node.h"
#include "symtbl.h"

typedef struct ast {
  list_t *body, *stack, *auxstack;
  unsigned int atl, in_func;
} ast_t;

ast_t *init_ast(void);
int add_node(ast_t *ast, const ast_node_t *node, symtbl_t *symtbl);