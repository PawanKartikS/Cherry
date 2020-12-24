#include "expr.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "node.h"
#include "token.h"

token_t *eval_expr(const token_t *lhs, const token_t *rhs, const char *op) {
  token_t *t = alloc(sizeof(token_t));
  t->tk = alloc(sizeof(double));
  t->type = numeric;

  if ((lhs && lhs->type != numeric) || (rhs && rhs->type != numeric)) {
    fprintf(stderr, "expr.c: non-numeric in exprtree\n");
    cleanup();
    _Exit(1);
  }

  double l = lhs ? *(double *)lhs->tk : 0;
  double r = rhs ? *(double *)rhs->tk : 0;

  if (!strcmp(op, "+"))
    *(double *)t->tk = l + r;
  else if (!strcmp(op, "-"))
    *(double *)t->tk = l - r;
  else if (!strcmp(op, "*"))
    *(double *)t->tk = l * r;
  else if (!strcmp(op, "/"))
    *(double *)t->tk = l / r;
  else
    assert(1 != 1);

  return t;
}

token_t *eval_exprtree(binary_node_t *node, const symtbl_t *symtbl) {
  if (!node) return 0;

  if (!node->lhs && !node->rhs) {
    if (node->val->type == identifier) {
      entry_t *e = get_symentry(symtbl, (char *)node->val->tk);
      if (!e) {
        fprintf(stderr, "expr.c: missing decl for sym in exprtree [%s]\n",
                (char *)node->val->tk);
        cleanup();
        _Exit(1);
      }

      node->val->tk = e->val;
      node->val->type = e->vtype;
    }

    return node->val;
  }

  token_t *l = eval_exprtree(node->lhs, symtbl);
  token_t *r = eval_exprtree(node->rhs, symtbl);

  return eval_expr(l, r, (char *)node->val->tk);
}

binary_node_t *init_bnode(list_t *operands, const token_t *top) {
  if (!operands || !top) return NULL;

  token_t *rhs = pop_last(operands);
  token_t *lhs = pop_last(operands);

  if (!lhs || !rhs) return NULL;

  binary_node_t *bnode = alloc(sizeof(binary_node_t));
  bnode->lhs = lhs;
  bnode->rhs = rhs;
  bnode->val = (token_t *)top;

  return bnode;
}

int is_operator(const token_t *tk) {
  if (tk->type == numeric) return 0;

  const char *t = tk->tk;
  return !strcmp(t, "+") || !strcmp(t, "-") || !strcmp(t, "*") ||
         !strcmp(t, "/");
}

int operator_assoc(const char *tk) {
  if (!strcmp(tk, "+") || !strcmp(tk, "-")) return 1;
  if (!strcmp(tk, "*") || !strcmp(tk, "/")) return 2;
  assert(1 != 1);
}

int to_exprtree(list_t *expr, void **buf, unsigned int *type) {
  int idf_seen = 0;
  if (!expr) return 0;

  list_t *operands = init_list();
  list_t *operators = init_list();

  token_t *peek = peek_front(expr);

  if (match_token(peek, "+") || match_token(peek, "-")) {
    double t = 0;
    token_t *itk = ptr_to_token(numeric, &t);

    assert(add_front(expr, itk));
    peek = peek_front(expr);
  }

  while (peek) {
    token_t *tk = peek;

    /* Opening pr, add to operators. */
    if (match_token(tk, "(")) {
      assert(add(operators, tk));

      /* Operand, add to operands. */
    } else if (tk->type == string || tk->type == numeric ||
               tk->type == identifier) {
      if (tk->type == identifier) idf_seen = 1;

      binary_node_t *bnode = alloc(sizeof(binary_node_t));
      bnode->val = tk;
      bnode->lhs = NULL;
      bnode->rhs = NULL;

      assert(add(operands, bnode));

      /* Operator */
    } else if (is_operator(tk)) {
      if (operators->size > 0) {
        token_t *top = peek_last(operators);

        /* Current operator is of lower associativity than top of the stack. */
        if (top->type != numeric && is_operator(top) &&
            operator_assoc(tk->tk) <= operator_assoc(top->tk)) {
          /**
           * Binary expression
           *           <op>
           *          /   \
           *         /     \
           *        /       \
           *     <lhs>     <rhs>
           */

          binary_node_t *bnode = init_bnode(operands, top);
          if (!bnode) return 0;

          assert(add(operands, bnode));
          pop_last(operators);
        }
      }

      assert(add(operators, tk));

      /* Closing pr */
    } else if (match_token(tk, ")")) {
      if (operators->size == 0) return 0;

      token_t *top = peek_last(operators);

      /* Construct binary expression till we get a closing brace. */
      while (!match_token(top, "(")) {
        /**
         * Binary expression
         *           <op>
         *          /   \
         *         /     \
         *        /       \
         *     <lhs>     <rhs>
         */
        binary_node_t *bnode = init_bnode(operands, top);
        if (!bnode) return 0;

        assert(add(operands, bnode));

        /* Pop current top of the stack. */
        pop_last(operators);
        if (operators->size == 0) break;

        /* New top of the stack. */
        top = peek_last(operators);
      }

      pop_last(operators);
    } else {
      /* Illegal token */
      break;
    }

    pop_front(expr);
    peek = peek_front(expr);
  }

  /* We're done processing the operands inside the parentheses. */
  while (operators->size != 0) {
    if (operands->size == 0) return 0;

    /**
     * Binary expression
     *           <op>
     *          /   \
     *         /     \
     *        /       \
     *     <lhs>     <rhs>
     */

    binary_node_t *bnode = init_bnode(operands, pop_last(operators));
    if (!bnode) return 0;

    assert(add(operands, bnode));
  }

  binary_node_t *root = (operands->size != 1 || operators->size != 0)
                            ? NULL
                            : peek_front(operands);

  if (!root) return 0;

  /* We've seen an identifier, cannot evaluate expression. */
  if (idf_seen) {
    *buf = root;
    *type = exprtree;
    return 1;
  }

  /* Apply constant folding */
  token_t *evalres = eval_exprtree(root, NULL);
  if (!evalres) return 0;

  *buf = evalres->tk;
  *type = evalres->type;
  return 1;
}