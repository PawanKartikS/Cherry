#include "ast.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

ast_t *init_ast(void) {
  ast_t *ast = alloc(sizeof(ast_t));
  assert(ast);

  ast->body = init_list();
  ast->stack = init_list();
  ast->auxstack = init_list();

  ast->atl = 1;
  ast->in_func = 0;
  return ast;
}

int should_branch(const char *kwd) {
  return !strcmp(kwd, "def") || !strcmp(kwd, "if") || !strcmp(kwd, "for") ||
         !strcmp(kwd, "else") || !strcmp(kwd, "end");
}

int add_node(ast_t *ast, const ast_node_t *node, symtbl_t *symtbl) {
  if (!ast || !node) return 0;

  const char *kwd = node->kwd;
  if (!strcmp(kwd, "def")) {
    if (ast->in_func) {
      fprintf(stderr, "ast.c: unexpected def, did you forget to place end?\n");
      return 0;
    }

    func_node_t *fnode = node->ch;
    fsig_t *sig = get_fsig(symtbl, fnode->func);
    if (sig) {
      fprintf(stderr, "ast.c: function redeclaration [%s]\n", fnode->func);
      return 0;
    }

    assert(register_func(symtbl, fnode->func, fnode->args, node));
    ast->in_func = 1;
  }

  if (strcmp(kwd, "def") && ast->in_func == 0) {
    fprintf(stderr, "ast.c: dangling statement [%s]\n", kwd);
    return 0;
  }

  /**
   * Since we do not have anything on the stack, adding this node to AST should
   * be straight forward.
   * However, if this is to be branched, we'll need to push node to stack (as a
   * ref.) for next incoming node.
   */
  if (ast->stack->size == 0) {
    assert(add(ast->body, node));
    if (!should_branch(kwd)) return 1;

    /* TODO: else and end shouldn't come here */
    ast->atl = 1;
    assert(add(ast->stack, node));
    return 1;
  }

  /**
   * Stack is not empty and this is an if statement.
   * This means that this is a nested if and must be appended to the TOS and
   * also must be pushed to the stack so that it acts as a reference to the next
   * incoming node.
   */
  if (!strcmp(kwd, "def") || !strcmp(kwd, "if") || !strcmp(kwd, "for")) {
    ast_node_t *top = peek_last(ast->stack);
    assert(add((ast->atl == 1 ? top->lch : top->rch), node));

    /* We need to transition to the new state. So we save our current state. */
    int *atlval = alloc(sizeof(int));
    *atlval = ast->atl;
    assert(add(ast->auxstack, atlval));

    /* Begin transition. */
    ast->atl = 1;
    assert(add(ast->stack, node));
    return 1;
  }

  /* Else is just a toggle switch. */
  if (!strcmp(kwd, "else")) {
    ast_node_t *top = peek_last(ast->stack);
    if (strcmp(top->kwd, "if")) {
      fprintf(stderr, "ast.c: else is invalid on top of [%s]\n", top->kwd);
      return 0;
    }

    ast->atl = 0;
    return 1;
  }

  /* This is the end of the block we're currently in. */
  if (!strcmp(kwd, "end")) {
    /**
     * If the auxiliary stack is not empty, it means that we did not have any
     * state saved. If not, we need to restore our previous state.
     */
    ast->atl = ast->auxstack->size > 0 ? *(int *)(pop_last(ast->auxstack)) : 1;

    /* TOS is no longer a reference to the next incoming node. */
    ast_node_t *top = pop_last(ast->stack);
    if (strcmp(top->kwd, "def") && strcmp(top->kwd, "if") &&
        strcmp(top->kwd, "for")) {
      fprintf(stderr, "ast.c: end is invalid for [%s]\n", top->kwd);
      return 0;
    }

    if (!strcmp(top->kwd, "def")) {
      assert(ast->in_func);
      ast->in_func = 0;
    }

    return 1;
  }

  /**
   * Non-branching statement that happens to fall either under if's main body or
   * it's alternate body.
   */
  ast_node_t *top = peek_last(ast->stack);
  assert(add((ast->atl == 1 ? top->lch : top->rch), node));
  return 1;
}