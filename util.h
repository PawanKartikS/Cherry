#pragma once

#include "list.h"

void *alloc(const unsigned long size);
void cleanup(void);
void mark_free(const void *fptr);