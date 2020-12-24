#pragma once
#include "node.h"

enum {
  bitwise, br, operator, pr, sqbr, syntax, string, numeric, identifier, fretval,
  glist, gstack, unknown };

typedef struct token {
  void *tk;
  unsigned int type;
} token_t;

token_t *init_token(const char *tk, const unsigned int type);
int is_reserved(const char *tk);
int match_token(const token_t *tk, const char *val);
token_t *pop_token(list_t *tokens, const unsigned int exprm);
token_t *ptr_to_token(const unsigned int type, const void *buf);