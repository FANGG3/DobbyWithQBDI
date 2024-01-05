//
// Created by fang on 2023/12/29.
//

#ifndef QBDI_SOCKETUTILS_H
#define QBDI_SOCKETUTILS_H
#define MODE_SOCKET
#include "mongoose.h"
#include "logger.h"
#include <pthread.h>
class socketUtils {


public:
    static socketUtils* getInstance(){
        if (instance == nullptr){
            instance = new socketUtils();
        }
        return instance;
    };

    bool startServer();
    static bool sendMsg(struct mg_connection *c,std::string data);
    static bool sendMsg(struct mg_connection *c,void *buff, size_t len);

    void close();
private:
    static socketUtils* instance ;
    static mg_connection* connection;
    static struct mg_mgr mgr;

};
void sendAllClient(const char* data,size_t len);
void sendAllClient(const char* data);
void sendAllClient_sn(const char* fmt,...);


#endif //QBDI_SOCKETUTILS_H
