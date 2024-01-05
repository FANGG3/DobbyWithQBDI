//
// Created by fang on 2024/1/4.
//

#include "HookInfo.h"
#include <stddef.h>


static ModuleInfo moduleInfo;

ModuleInfo HookInfo::get_module() {
    return moduleInfo;
}

void HookInfo::set_module(const char* name,size_t base, size_t end) {
    moduleInfo.name = name;
    moduleInfo.end = end;
    moduleInfo.base = base;
}
