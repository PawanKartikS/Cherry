#pragma once

#include "args.h"
#include "list.h"
#include "symtbl.h"

int eval_builtin(const func_node_t *fnode, symtbl_t *symtbl);