//
// Created by fang on 23-12-19.
//

#ifndef QBDIRECORDER_VM_H
#define QBDIRECORDER_VM_H
#include "QBDI.h"
#include "QBDI/State.h"
#include "dobby.h"
#include "mongoose.h"
//#include "socketUtils.h"
#include "fileRecord.h"

#include "json.hpp"
#include "jsonbean.h"
#define STACK_SIZE  0x800000
void syn_regs(DobbyRegisterContext *ctx,QBDI::GPRState *state,bool D2Q);
class vm {

public:
    QBDI::VM init(void* address);
private:

};


#define VM_BEFORE(addr) \
void vm_handle_##addr(void* address,DobbyRegisterContext *ctx,addr_t * relocated_addr){\
    LOGD("vm address %p ",address);\
    DobbyDestroy(address);\
    auto vm_ = new vm();\
    auto qvm = vm_->init(address);\
    auto state = qvm.getGPRState();\
    QBDI::rword retval;\
    syn_regs(ctx,state, true);                                                         \
    uint8_t *fakestack; \
    QBDI::allocateVirtualStack(state, STACK_SIZE, &fakestack); \



#define VM_AFTER \
    qvm.call(nullptr, (uint64_t) address);\
    syn_regs(ctx,state, false);           \
    QBDI::alignedFree(fakestack);\


#define VM_END(addr)                 \
    DobbyInstrumentQBDI(address,vm_handle_##addr);\
}\

#endif //QBDIRECORDER_VM_H
