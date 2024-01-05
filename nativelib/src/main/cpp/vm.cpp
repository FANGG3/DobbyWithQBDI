//
// Created by fang on 23-12-19.
//



#include <iostream>
#include <iomanip>
#include <cassert>
#include <sstream>
#include <QBDI/Callback.h>
#include "vm.h"
#include "logger.h"
#include "dobby_symbol_resolver.h"
#include "HookInfo.h"

using namespace std;
using namespace QBDI;


string prinitGPR(QBDI::GPRState *gprState){
    stringstream info;
    int i = 0;
    nlohmann::ordered_json j;
    auto bean = new jsonbean(jsonbean::TYPE::REG);
    for (string reg_name: QBDI::GPR_NAMES) {

        j[reg_name] = QBDI_GPR_GET(gprState,i);
        info << reg_name<<"=" << QBDI_GPR_GET(gprState,i) << setbase(16)<< " | ";
        i++;
    }
    bean->setData(j);
    //LOGD("%s",info.str().c_str());
    return  bean->dump();
    return info.str();
}
#define IFNULL_P(obj,d) (obj == nullptr)? d : obj
#define IFNULL_I(obj,d) (obj == 0)? d : obj
string printASM(const QBDI::InstAnalysis *instAnalysis){
    auto bean = new jsonbean(jsonbean::ASM);
    nlohmann::ordered_json data;
    data["symbol"] = IFNULL_P(instAnalysis->symbol,"");
    data["address"] = IFNULL_I(instAnalysis->address - HookInfo::getInstance().get_module().base,0);
    data["symbolOffset"] = IFNULL_I(instAnalysis->symbolOffset,0);
    data["disassembly"] = IFNULL_P(instAnalysis->disassembly,"");
    data["module"] = IFNULL_P(instAnalysis->module,"");
    bean->setData(data);
    return bean->dump();
}

string printMemAcc(QBDI::MemoryAccess memoryAccess){
    auto bean = new jsonbean(jsonbean::MEM);
    nlohmann::ordered_json data;
    data["accessAddress"] = memoryAccess.accessAddress;
    data["type"] = memoryAccess.type;
    data["instAddress"] = memoryAccess.instAddress;
    data["value"] = memoryAccess.value;
    data["size"] = memoryAccess.size;
    bean->setData(data);
    return bean->dump();
}



QBDI::VMAction showInstruction(QBDI::VM *vm, QBDI::GPRState *gprState, QBDI::FPRState *fprState, void *data) {

    const QBDI::InstAnalysis *instAnalysis = vm->getInstAnalysis(QBDI::ANALYSIS_INSTRUCTION
            | QBDI::ANALYSIS_SYMBOL
            | QBDI::ANALYSIS_DISASSEMBLY
            | QBDI::ANALYSIS_OPERANDS
            );
    LOGD("%s[0x%x]:%lx %-16s",instAnalysis->symbol,instAnalysis->symbolOffset, instAnalysis->address, instAnalysis->disassembly);

    //sendAllClient_sn("%s[0x%x]:%lx %-16s",instAnalysis->symbol,instAnalysis->symbolOffset, instAnalysis->address, instAnalysis->disassembly);

#ifdef MODE_SOCKET
    sendAllClient_sn("%s", printASM(instAnalysis).c_str());
    sendAllClient_sn("%s",prinitGPR(gprState).c_str());
#endif
#ifdef MODE_FILE
    recordToFile("%s",printASM(instAnalysis).c_str());
    recordToFile("%s", prinitGPR(gprState).c_str());
#endif

//    if (instAnalysis->mayLoad || instAnalysis->mayStore){
//        vector<QBDI::MemoryAccess> memAccVector = vm->getInstMemoryAccess();
//        for (int i = 0; i < memAccVector.size(); ++i) {
//
//        #ifdef MODE_SOCKET
//            sendAllClient_sn("%s",printMemAcc(memAccVector.at(i)).c_str());
//        #endif
//
//
//        }
//    }



    return QBDI::VMAction::CONTINUE;
}
QBDI::VMCbLambda event_cb = [](VMInstanceRef vm, const VMState *vmState,
                              GPRState *gprState, FPRState *fprState){



    return VMAction::CONTINUE;
};

QBDI::VM vm::init(void* address) {
    uint32_t cid;
    QBDI::GPRState *state;
    QBDI::VM qvm{};
    // Get a pointer to the GPR state of the VM
    state = qvm.getGPRState();
    assert(state != nullptr);
    qvm.recordMemoryAccess(QBDI::MEMORY_READ_WRITE);
    cid = qvm.addCodeCB(QBDI::PREINST, showInstruction, nullptr);
    //qvm.addVMEventCB(QBDI::VMEvent::EXEC_TRANSFER_CALL,)
    assert(cid != QBDI::INVALID_EVENTID);
    bool ret = qvm.addInstrumentedModuleFromAddr(reinterpret_cast<QBDI::rword>(address));
//    bool ret = qvm.instrumentAllExecutableMaps();
    assert(ret == true);

    return qvm;
}
void syn_regs(DobbyRegisterContext *ctx,QBDI::GPRState *state,bool D2Q){
    if (D2Q){
        for(int i = 0 ; i < 29; i++){
            QBDI_GPR_SET(state,i,ctx->general.x[i]);
        }
        state->lr = ctx->lr;
        state->x29 = ctx->fp;
        state->sp = ctx->sp;

//        state->sp = ctx->sp;

    }else{
        for(int i = 0 ; i < 29; i++){
            //QBDI_GPR_SET(state,i,ctx->general.x[i]);
            ctx->general.x[i] = QBDI_GPR_GET(state,i);
        }
        ctx->lr = state->lr;
        ctx->fp = state->x29;
        ctx->sp = state->sp;

    }
    //state->sp = ctx->sp;
}









