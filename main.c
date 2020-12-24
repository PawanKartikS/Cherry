/**
 * main.c
 * Entry point for the interpreter (Cherry).
 */

#include <stdio.h>

#include "ast.h"
#include "eval.h"
#include "lex.h"
#include "node.h"
#include "token.h"
#include "util.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: ./cherry <sourcefile>\n");
    return 1;
  }

  /* Set to 1 if there's an error */
  int ret = 0;
  char buf[512];

  FILE *fd = fopen(argv[1], "r");
  if (!fd) {
    fprintf(stderr, "main.c: could not open [%s]\n", argv[1]);
    return 1;
  }

  ast_t *ast = init_ast();
  eval_t *eval = init_eval();

  int lno = 0;
  while (fgets(buf, sizeof buf, fd)) {
    lno++;
    list_t *tokens = lex(buf);

    /**
     * There are 3 cases we need to watch out for -
     * 1. tokens is NULL   - lexer failed but assume that the error was written
     * to stderr.
     * 2. tokens is empty  - not a valid statement, e.g, comment line.
     * 3. toknes is filled - lexer successfully lexed the line.
     */
    if (!tokens) {
      ret = 1;
      goto cleanup;
    }
    
    if (tokens->size == 0) continue;

    const char *kwd = ((token_t *)(tokens->head->data))->tk;
    ast_node_t *node = init_node(tokens);

    /**
     * init_node() calls parse() which routes the call to parse_xxx(). If node
     * is NULL, there was a parsing error.
     */
    if (!node) {
      fprintf(stderr, "main.c: init_node() fail [%s] L[%d]\n", kwd, lno);
      ret = 1;
      goto cleanup;
    }

    node->lno = lno;

    /**
     * add_node() returns 0 if the node couldn't be inserted into AST. This is
     * when the program structure is invalid, for e.g, missing "end" or "else"
     * stacked onto "def" etc.
     */
    if (!add_node(ast, node, eval->tbl)) {
      fprintf(stderr, "main.c: add_node() fail [%s]\n", kwd);
      ret = 1;
      goto cleanup;
    }
  }

  /* Verify that the AST is valid */
  if (ast->auxstack->size != 0 || ast->stack->size != 0 || ast->in_func) {
    fprintf(stderr,
            "main.c: please verify that the program is valid - missing end?\n");
    ret = 1;
    goto cleanup;
  }

  /* exec */
  if (!eval_prog(ast, eval)) ret = 1;
  fclose(fd);

/* Clean up all the heap allocs and return the error code set by the program. */
cleanup:
  cleanup();
  return ret;
}