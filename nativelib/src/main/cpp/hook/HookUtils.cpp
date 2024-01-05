//
// Created by fang on 2024/1/3.
//
#include <climits>
#include <cstring>
#include <cstdio>
#include <asm-generic/unistd.h>
#include <unistd.h>
#include <unwind.h>
#include <dlfcn.h>
#include <vector>
#include <string>
#include <sstream>

#include "logger.h"
#include "HookUtils.h"

MapItemInfo getSoBaseAddress(const char *name) {
    MapItemInfo info{0};
    if (name == nullptr) {
        return info;
    }
    size_t start = 0;
    size_t end = 0;
    bool isFirst = true;
    size_t len = 0;
    char buffer[PATH_MAX];
    memset(buffer, 0, PATH_MAX);


    //找不到用原始文件
    FILE *fp = fopen("/proc/self/maps", "r");

    if (fp == nullptr) {
        LOGD("找不到用原始文件");
        return info;
    }

    char *line = nullptr;
    while (getline(&line, &len, fp) != -1) {
        //LOGD("%s",line);
        if (line != nullptr && strstr(line, name)) {
            sscanf(line, "%lx-%lx", &start, &end);
//            LOGD("%s",line);
            //start 只有第一次赋值
            if (isFirst) {
                info.start = start;
                isFirst = false;
            }
        }
    }
    info.end = end;
    syscall(__NR_close, fp);
    //LOGE("get maps info start -> 0x%zx  end -> 0x%zx ",info.start,info.end);
    return info;
}

static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context* context, void* arg)
{
    std::vector<_Unwind_Word> &stack = *(std::vector<_Unwind_Word>*)arg;
    stack.push_back(_Unwind_GetIP(context));
    return _URC_NO_REASON;
}

void callstackDump(std::string &dump) {
    std::vector<_Unwind_Word> stack;
    _Unwind_Backtrace(unwindCallback, (void*)&stack);
    dump.append("                                                               \n"
                "*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***\n"
                "pid: 17980, tid: 17980, name: callstack.dump  >>> callstack.dump <<<\n"
                "signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 0x0\n"
                "r0 00000000  r1 00000000  r2 00000001  r3 00000001\n"
                "r4 e8efe008  r5 e0537b99  r6 ff970b88  r7 ff970a98\n"
                "r8 ff970de0  r9 e7904400  sl e790448c  fp ff970b14\n"
                "ip e8ef985c  sp ff970a60  lr e8eca00f  pc e0537d86  cpsr 200b0030\n"
                "backtrace:\n");

    char buff[256];
    for (size_t i = 0; i < stack.size(); i++) {
        Dl_info info;
        if (!dladdr((void*)stack[i], &info)) {
            continue;
        }
        int addr = (char*)stack[i] - (char*)info.dli_fbase - 1;
        if (info.dli_sname == NULL || strlen(info.dli_sname) == 0) {
            sprintf(buff, "#%02x pc %08x  %s\n", i, addr, info.dli_fname);
        } else {
            sprintf(buff, "#%02x pc %08x  %s (%s+00)\n", i, addr, info.dli_fname, info.dli_sname);
        }
        dump.append(buff);
    }
}

void callstackLogcat(const char* tag) {
    std::string dump;
    callstackDump(dump);
    __android_log_print(ANDROID_LOG_DEBUG, tag, "%s", dump.c_str());
}

char* appName = nullptr;
char* getAppName(){
    if (appName != NULL){
        LOGD("get appName %s",appName);
        return appName;
    }
    FILE* f = fopen("/proc/self/cmdline","r");
    size_t len;
    char* line = nullptr;
    if(getline(&line,&len,f)==-1){
        perror("can't get app name");
    }
    appName = line;
    LOGD("get appName %s",appName);
    return appName;
}

char privatePath[PATH_MAX];
char* getPrivatePath(){
    if (privatePath[0] != 0 ){
        return privatePath;
    }
//    std::string ret= std::string("/data/data");
//    ret.append(getAppName());
    sprintf(privatePath,"%s%s%s","/data/data/",getAppName(),"/");
    return privatePath;
}