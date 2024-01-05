//
// Created by fang on 2023/12/29.
//

#include "jsonbean.h"
#include <thread>
#include <sstream>

static uint64_t getThreadID()
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return std::stoull(ss.str());
}

jsonbean::jsonbean(jsonbean::TYPE type) {
    json["type"] = type;

    json["tid"] = getThreadID();
}

jsonbean jsonbean::setData(string data) {
    json["data"] = data;
    return *this;
}

jsonbean jsonbean::setData(nlohmann::ordered_json data) {
    json["data"] = data;
    return *this;
}

string jsonbean::dump(){
    return json.dump();
}
nlohmann::ordered_json jsonbean::getJson(){
    return json;
}


