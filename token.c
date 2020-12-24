#include "token.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

double parse_numeric(const char *numeric) {
  errno = 0;
  char *endptr;
  double num = strtod(numeric, &endptr);

  assert(errno == 0 && endptr == (numeric + strlen(numeric)));
  return num;
}

token_t *init_token(const char *tk, const unsigned int type) {
  token_t *token = alloc(sizeof(token_t));
  token->type = type;

  if (token->type != numeric) {
    token->tk = alloc(512);
    strcpy(token->tk, tk);
  } else {
    token->tk = alloc(sizeof(double));
    *(double *)token->tk = parse_numeric((char *)tk);
  }

  return token;
}

int is_reserved(const char *kwd) {
  static const char *reserved[] = {/* From ast.c */
                                   "def", "if", "for", "else", "end",
                                   /* Keywords - from eval.c */
                                   "defer", "var", "print", "read", "return",
                                   /* Built-in - from builtin.c */
                                   "cmp", "len", "idx", "put", "rev", "exit",
                                   "gc"};

  const unsigned int len = sizeof(reserved) / sizeof(reserved[0]);
  for (unsigned int i = 0; i < len; i++) {
    if (!strcmp(reserved[i], kwd)) return 1;
  }

  return 0;
}

int match_token(const token_t *tk, const char *val) {
  if (!tk || tk->type == numeric) return 0;
  return strcmp((char *)tk->tk, val) == 0;
}

/* Wrapper to prevent repeated pop_front() and casts. */
token_t *pop_token(list_t *tokens, const unsigned int exprm) {
  /**
   * exprm -
   * 0 -> [+, 1]
   * 1 -> [+1]
   */

  token_t *ftk = pop_front(tokens);
  if (!ftk) {
    fprintf(stderr, "parse.c: pop_front() yields NULL\n");
    return NULL;
  }

  if (!match_token(ftk, "+") && !match_token(ftk, "-")) return ftk;
  if (exprm == 0) return ftk;

  token_t *stk = peek_front(tokens);
  assert(stk);
  if (stk->type != numeric) {
    return ftk;
  }

  if (match_token(ftk, "+")) {
    pop_front(tokens);
    return stk;
  } else {
    *(double *)stk->tk *= -1;
  }

  pop_front(tokens);
  return stk;
}

token_t *ptr_to_token(const unsigned int type, const void *buf) {
  if ((type != string && type != numeric) || !buf) {
    return NULL;
  }

  token_t *tk = alloc(sizeof(token_t));
  tk->type = type;
  if (type == string) {
    tk->tk = alloc(512);
    strcpy((char *)tk->tk, (char *)buf);
  } else {
    tk->tk = alloc(sizeof(double));
    *(double *)tk->tk = *(double *)buf;
  }

  return tk;
}
