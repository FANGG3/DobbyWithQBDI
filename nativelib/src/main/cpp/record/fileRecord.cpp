//
// Created by fang on 2024/1/4.
//

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <unistd.h>
#include "fileRecord.h"
#include "logger.h"
using namespace std;
static fstream* mFs ;


void fileRecordOpen(){

    if (mFs != nullptr) return;

    char* private_path = getPrivatePath();
    string p = private_path;
    p.append("record.txt");
    char* recode_path = const_cast<char *>(p.c_str());
    if(access(recode_path,F_OK) !=-1 ){
        remove(recode_path);
    }
    static fstream fs(recode_path,ios_base::app);
    LOGD("File record path: %s  %p %d",recode_path,&fs,fs.is_open());
    mFs = &fs;
}

void write(fstream* fs_, char* buff,size_t len){

    *fs_ << buff << endl;
    //fs_->flush();
}

void recordToFile(const char *fmt, ...) {
    if (mFs == nullptr) fileRecordOpen();
    va_list args;
    size_t len;
    int LEN = 1024;
    char *dst = (char *) malloc(LEN);
    memset(dst, 0, LEN);
    va_start(args, fmt);
    len = vsnprintf(dst, LEN - 1, fmt, args);
    va_end(args);
    LOGD("%p %d %s",mFs, len, dst);
    write(mFs,dst,len);
    free(dst);
}



