#include <jni.h>
#include <string>
#include <iostream>
#include <dlfcn.h>
#include "vm.h"
#include "nativelib.h"
#include "HookUtils.h"
#include "HookInfo.h"

using namespace std;
#define HOOK_ADDR_DOBBY(func)  \
  DobbyHook((void*)func,  (void*) new_##func, (void**) &orig_##func); \


void aaa();

extern "C" JNIEXPORT jstring JNICALL
Java_com_f_nativelib_NativeLib_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    auto a = env->NewStringUTF("123123");
    env->DeleteLocalRef(a);
    //aaa();
    return env->NewStringUTF(hello.c_str());
}

int sub(int a, int b) {
    return a + b;
}

int add(int a, int b) {
    int c = sub(5, 6);
    return a + b + c;
}



//void add_handle2(void* address,DobbyRegisterContext *ctx,addr_t * relocated_addr){
//    LOGD("address %p ",address);
//    LOGD("Java_com_f_nativelib_NativeLib_stringFromJNI %p ,lr %lx ",Java_com_f_nativelib_NativeLib_stringFromJNI,ctx->lr);
//    LOGD("%lx %lx",ctx->dmmpy_0,ctx->dmmpy_1);
//    LOGD("relocated_addr %p",relocated_addr);
//    DobbyDestroy(address);
//    auto vm_ = new vm();
//    auto qvm = vm_->init(address);
//    auto state = qvm.getGPRState();
//    QBDI::rword retval;
//    syn_regs(ctx,state, true);
//
//    qvm.call(&retval, (uint64_t) address);
//    syn_regs(ctx,state, false);
//    DobbyInstrumentQBDI(address,add_handle);
//}


VM_BEFORE(add)
    LOGD("VM_BEFORE %lx", ctx->general.x[0]);
    LOGD("VM_BEFORE %lx", ctx->general.x[1]);
    VM_AFTER
    LOGD("VM_AFTER %lx", ctx->general.x[0]);
VM_END(add)


void aaa() {
    DobbyInstrumentQBDI((void *) (add), vm_handle_add);
    LOGD("%d", add(1, 2));
    LOGD("%d", add(1, 2));
    LOGD("%d", add(1, 2));
}


void init() {
    linkerHandler_init();
}

JavaVM *mVm;
JNIEnv *mEnv;

jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reversed) {
    LOGD("JNI_ONLOAD start");
    mVm = vm;
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) == JNI_OK) {
        mEnv = env;

        //init();
        return JNI_VERSION_1_6;
    }
    return JNI_ERR;
}


// you can do something here
VM_BEFORE(sig1)

    LOGD("ctx->lr %zx",ctx->lr);

VM_AFTER
    LOGD("vm end");
}


extern "C" void _init(void) {
    LOGD("getPrivatePath %s", getPrivatePath());
    //aaa();
    init();

}

__unused __attribute__((constructor)) void init_main() {
    // add hook
    DobbyInstrumentQBDI((void*)Java_com_f_nativelib_NativeLib_stringFromJNI, vm_handle_sig1);
}