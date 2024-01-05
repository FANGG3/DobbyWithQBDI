//
// Created by fang on 2023/12/22.
//

#ifndef QBDI_NATIVELIB_H
#define QBDI_NATIVELIB_H
#include <unistd.h>
#include "logger.h"
#include "dobby.h"
#include "QBDI.h"
#include "socketUtils.h"
#include "BionicLinkerUtil/bionic_linker_util.h"
#include "HookUtils.h"
#include "linkerHandler.h"
extern JavaVM *mVm;
extern JNIEnv *mEnv;

extern "C"{
#include "mongoose.h"

};


#endif //QBDI_NATIVELIB_H
