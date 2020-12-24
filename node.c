#include "node.h"

#include <stdio.h>
#include <string.h>

#include "parse.h"
#include "token.h"
#include "util.h"

ast_node_t *init_node(list_t *list) {
  if (!list) return NULL;

  ast_node_t *node = alloc(sizeof(ast_node_t));
  if (!node) return NULL;

  node->kwd = ((token_t *)list->head->data)->tk;
  node->lch = init_list();
  node->rch = init_list();
  node->tokens = list;
  node->type = parse(&node->ch, list);

  ast_node_t *res = node->type < 0 ? NULL : node;
  if (res && list->size) {
    fprintf(stderr, "node.c: excess tokens at the end for [%s]\n", node->kwd);
    return NULL;
  }

  return res;
}
