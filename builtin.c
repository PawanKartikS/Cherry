#include "builtin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

struct method {
  char *func;
  int n_args;
  int argtypes[2];
  int (*fptr)(const token_t **fargs, symtbl_t *symtbl,
              const unsigned int arglen);
};

/* str related */
int __cmp(const token_t **args, symtbl_t *symtbl, const unsigned int arglen);
int __len(const token_t **args, symtbl_t *symtbl, const unsigned int arglen);
int __idx(const token_t **args, symtbl_t *symtbl, const unsigned int arglen);
int __put(const token_t **args, symtbl_t *symtbl, const unsigned int arglen);
int __rev(const token_t **args, symtbl_t *symtbl, const unsigned int arglen);

/* */
int __exit(const token_t **args, symtbl_t *symtbl, const unsigned int arglen);
int __gcms(const token_t **args, symtbl_t *symtbl, const unsigned int arglen);
int __type(const token_t **args, symtbl_t *symtbl, const unsigned int arglen);

static struct method builtins[] = {{"cmp", +2, {string, string}, __cmp},
                                   {"len", +1, {string}, __len},
                                   {"idx", +2, {string, string}, __idx},
                                   {"put", -1, {}, __put},
                                   {"rev", +1, {string}, __rev},
                                   {"exit", +1, {numeric}, __exit},
                                   {"gc", +1, {-1}, __gcms},
                                   {"type", +1, {-1}, __type}};

/**
 * Checks if the supplied args match the req args (count vs type of args) and
 * invokes the specified function.
 */
int eval_builtin(const func_node_t *fnode, symtbl_t *symtbl) {
  char *func = fnode->func;
  list_t *args = fnode->args;

  const long unsigned int size = sizeof(builtins) / sizeof(struct method);

  for (int i = 0; i < size; i++) {
    if (!strcmp(builtins[i].func, func)) {
      struct method builtin = builtins[i];

      /* if n_args is set to -1, it means that this function takes vargs. */
      unsigned int n_args = builtin.n_args != -1 ? builtin.n_args : args->size;
      const token_t *fargs[n_args];

      /* varg function */
      if (builtin.n_args == -1) {
        for (unsigned int j = 0; j < n_args; j++) {
          fargs[j] = pop_arg(args, symtbl, -1);
        }

        return builtins[i].fptr(fargs, symtbl, n_args);
      }

      if (builtin.n_args != args->size) {
        fprintf(stderr, "builtin.c: %s() takes %d args\n", func,
                builtin.n_args);
        return 0;
      }

      for (unsigned int j = 0; j < n_args; j++)
        fargs[j] = pop_arg(args, symtbl, builtin.argtypes[j]);
      return builtins[i].fptr(fargs, symtbl, n_args);
    }
  }

  return 0;
}

int __cmp(const token_t **args, symtbl_t *symtbl, const unsigned int arglen) {
  double c = strcmp((char *)args[0]->tk, (char *)args[1]->tk);
  return ret_res(symtbl, &c, numeric);
}

int __len(const token_t **args, symtbl_t *symtbl, const unsigned int arglen) {
  double len = strlen((char *)args[0]->tk);
  return ret_res(symtbl, &len, numeric);
}

int __idx(const token_t **args, symtbl_t *symtbl, const unsigned int arglen) {
  char *ptr = strstr((char *)args[0]->tk, (char *)args[1]->tk);
  double idx = !ptr ? -1 : (char *)ptr - (char *)args[0]->tk;
  return ret_res(symtbl, &idx, numeric);
}

int __put(const token_t **args, symtbl_t *symtbl, const unsigned int arglen) {
  for (unsigned int i = 0; i < arglen; i++) {
    const token_t *arg = args[i];

    if (!arg) {
      fprintf(stderr, "builtin.c: fatal, arg is (null)\n");
      return 0;
    }

    switch (arg->type) {
      case string:
        printf("%s ", (char *)arg->tk);
        break;
      case numeric:
        printf("%g ", *(double *)arg->tk);
        break;

        /**
         * Guaranteed to not hit this, __parse_arglist() is good enough to
         * filter out invalid args.
         */
      default:
        fprintf(stderr, "builtin.c: in put() invalid arg at [%d]\n", i + 1);
        return 0;
    }
  }

  printf("\n");
  return 1;
}

int __rev(const token_t **args, symtbl_t *symtbl, const unsigned int arglen) {
  char *s = (char *)args[0]->tk;
  const unsigned int len = strlen(s);
  const unsigned int mid = len / 2;

  for (unsigned int i = 0; i < mid; i++) {
    char c = s[i];
    s[i] = s[len - i - 1];
    s[len - i - 1] = c;
  }

  return ret_res(symtbl, s, string);
}

int __exit(const token_t **args, symtbl_t *symtbl, const unsigned int arglen) {
  double exitcode = *(double *)args[0]->tk;
  cleanup();
  _Exit(exitcode);
}

int __gcms(const token_t **args, symtbl_t *symtbl, const unsigned int arglen) {
  const token_t *e = args[0];
  mark_free((void *)e->tk);
  return 1;
}

int __type(const token_t **args, symtbl_t *symtbl, const unsigned int arglen) {
  const token_t *arg = args[0];
  double d = arg->type;
  return ret_res(symtbl, &d, numeric);
}