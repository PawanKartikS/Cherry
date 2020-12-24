#pragma once

#include "list.h"
#include "symtbl.h"
#include "token.h"

token_t *pop_arg(list_t *args, const symtbl_t *symtbl, const int reqtype);
int ret_res(symtbl_t *symtbl, const void *val, const unsigned int type);