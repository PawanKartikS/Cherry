#pragma once

#include "symtbl.h"
#include "token.h"
#include "util.h"

token_t *eval_exprtree(binary_node_t *node, const symtbl_t *symtbl);
int to_exprtree(list_t *expr, void **buf, unsigned int *type);