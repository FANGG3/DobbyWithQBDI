//
// Created by fang on 2023/12/29.
//

#ifndef QBDI_JSONBEAN_H
#define QBDI_JSONBEAN_H
#include "json.hpp"
using namespace  std;
class jsonbean {
public:

    enum TYPE{
        REG =  1,
        MEM =  2,
        ASM =  3
    };
    jsonbean(TYPE type);
    jsonbean setData(string);
    jsonbean setData(nlohmann::ordered_json);
    string dump();
    nlohmann::ordered_json getJson();

private:
    nlohmann::ordered_json json;


};
#endif //QBDI_JSONBEAN_H
