//
// Created by fang on 2024/1/3.
//

#ifndef QBDI_LINKERHANDLER_H
#define QBDI_LINKERHANDLER_H
#include "HookUtils.h"
void linkerHandler_init();
int resolve_symbol(const char *filename, const char *symname, intptr_t *symval);
intptr_t get_addr(const char *name);
#endif //QBDI_LINKERHANDLER_H
