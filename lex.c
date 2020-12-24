#include "lex.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "token.h"
#include "util.h"

void trimline(char **l, const unsigned int trimsize) {
  if (!strlen(*l)) return;

  *l += trimsize;
}

char *trimstr(char *s) {
  char *nstr = alloc(strlen(s) + 1);
  assert(s && nstr);

  memset(nstr, 0, strlen(s) + 1);
  if ((s[0] == '"' || s[0] == '\'') && (s[0] == s[strlen(s) - 1])) {
    memcpy(nstr, (char *)&s[1], strlen(s));
    nstr[strlen(nstr) - 1] = '\0';
  } else {
    strcpy(nstr, s);
  }

  return nstr;
}

int is_bitwise(const char c) { return c == '^' || c == '~'; }

int is_br(const char c) { return c == '{' || c == '}'; }

int is_opr(const char c) {
  return c == '=' || c == '!' || c == '<' || c == '>' || c == '+' || c == '-' ||
         c == '*' || c == '/' || c == '%';
}

int is_pr(const char c) { return c == '(' || c == ')'; }

int is_sqbr(const char c) { return c == '[' || c == ']'; }

int is_syntax(const char c) {
  return is_bitwise(c) || is_br(c) || is_opr(c) || is_pr(c) || is_sqbr(c) ||
         c == '.' || c == ',' || c == ':' || c == ';' || c == '!';
}

int extract_literal(const char *l, char *buf, const unsigned int size) {
  assert(l[0] == '"' || l[0] == '\'');

  int i = 1;
  int b_sl = -1;
  char beg = l[0];
  const int k_end = strlen(l);

  /**
   * Stop when we come across the quote that's same as opening one.
   * This is to correctly extract the literal inside a literal -
   * '"foo" "bar"'
   */
  for (i = 1; i < k_end; i++) {
    if (l[i] == '\\') b_sl = i;
    if (l[i] == beg && i != b_sl + 1) break;
  }

  if (i == k_end) {
    fprintf(stderr, "lex.c: missing closing quote for string literal.\n");
    return -1;
  }

  i++;
  assert((sizeof(char) * i) <= size);
  for (int j = 0, k = 0; j < i; j++) {
    if (l[j] != '\\') buf[k++] = l[j];
  }

  return i;
}

int extract_numeric(const char *l, char *buf, const unsigned int size) {
  assert(isdigit(l[0]));

  int i = 0;
  const int k_end = strlen(l);

  /* We don't bother if the number is valid or not. Let the parser verify. */
  for (i = 0; i < k_end; i++)
    if (!isdigit(l[i]) && l[i] != '.' && l[i] != ',') break;

  if (l[i - 1] == ',') i--;

  assert((sizeof(char) * i) <= size);
  memcpy(buf, l, (sizeof(char)) * i);
  return i;
}

/**
 * Extracts the identifier from the line. This is when all the other options
 * have been tried. We guess that the token is probably an identifier.
 */
int extract(const char *l, char *buf, const unsigned int size) {
  int i = 0;
  const int k_end = strlen(l);

  for (i = 0; i < k_end; i++)
    if (isspace(l[i]) || is_syntax(l[i])) break;

  assert((sizeof(char) * i) <= size);
  memcpy(buf, l, (sizeof(char)) * i);
  return i;
}

list_t *lex(char *l) {
  const unsigned int end = strlen(l);
  if (l[end - 1] == '\n') l[end - 1] = '\0';

  list_t *tokens = init_list();
  assert(tokens != NULL);

  while (strlen(l)) {
    if (isspace(l[0])) {
      trimline(&l, 1);
      continue;
    }

    if (l[0] == '#') return tokens;

    char buf[512];
    int type = unknown;
    memset(buf, 0, sizeof buf);

    if (is_bitwise(l[0]))
      type = bitwise;
    else if (is_br(l[0]))
      type = br;
    else if (is_pr(l[0]))
      type = pr;
    else if (is_sqbr(l[0]))
      type = sqbr;

    if (type != unknown) {
      memcpy(buf, l, 1);
      add(tokens, init_token(buf, type));
      trimline(&l, 1);
      continue;
    }

    if (is_opr(l[0])) {
      int trimsize = 1;
      if (strlen(l) >= 2 && is_opr(l[1])) trimsize = 2;

      memcpy(buf, l, trimsize);
      add(tokens, init_token(buf, operator));
      trimline(&l, trimsize);
      continue;
    }

    /**
     * Must always come after is_opr() as is_opr() combines operators.
     * (pre/post increment for ex.).
     * Otherwise is_syntax() breaks them up into 2 different tokens.
     */
    if (is_syntax(l[0])) {
      memcpy(buf, l, 1);
      add(tokens, init_token(buf, syntax));
      trimline(&l, 1);
      continue;
    }

    int idx = -1;
    type = unknown;

    if (l[0] == '"' || l[0] == '\'') {
      type = string;
      idx = extract_literal(l, buf, sizeof buf);
    } else if (isdigit(l[0])) {
      type = numeric;
      idx = extract_numeric(l, buf, sizeof buf);
    } else {
      type = identifier;
      idx = extract(l, buf, sizeof buf);
    }

    if (idx > 0 && type != unknown) {
      add(tokens, init_token(type == string ? trimstr(buf) : buf, type));
      trimline(&l, idx);
      continue;
    } else {
      fprintf(stderr, "fatal: could not lex - [%s]\n", l);
      return NULL;
    }
  }

  return tokens;
}