//
// Created by fang on 2024/1/4.
//

#ifndef QBDI_FILERECORD_H
#define QBDI_FILERECORD_H
#define MODE_FILE
#include "HookUtils.h"
void fileRecordOpen();
void recordToFile(const char *fmt, ...);
#endif //QBDI_FILERECORD_H
