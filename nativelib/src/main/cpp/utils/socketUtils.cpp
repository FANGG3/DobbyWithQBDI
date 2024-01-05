//
// Created by fang on 2023/12/29.
//


#include <string>
#include "socketUtils.h"
#include <list>
#include <vector>
#include <sstream>
#include <strstream>

using namespace std;
socketUtils *socketUtils::instance = nullptr;
mg_connection *socketUtils::connection = nullptr;
struct mg_mgr socketUtils::mgr;
static vector<mg_connection *> clients;

void send(mg_connection *c, const char *data, size_t len) {
    mg_ws_send(c, data, len, WEBSOCKET_OP_TEXT);
}

void sendAllClient(const char *data) {
    string d = data;
    sendAllClient(data, d.length());
}

bool isTryWS = false;
void sendAllClient_sn(const char *fmt, ...) {
//    if (clients.size() == 0 && isTryWS == false){
//        isTryWS = true;
//        socketUtils::getInstance()->startServer();
//        sleep(5);
//    }

    LOGD("client size:%lu",clients.size());
    va_list args;
    size_t len;
    //char dst [512];
    int LEN = 1024;
    char *dst = (char *) malloc(LEN);
    memset(dst, 0, LEN);
    va_start(args, fmt);
    len = vsnprintf(dst, LEN - 1, fmt, args);
    va_end(args);
    LOGD("%d %s", len, dst);

    sendAllClient(dst, len);
    free(dst);
}


void sendAllClient(const char *data, size_t len) {

    try {
        if (clients.size()!=0){
            for (mg_connection *c: clients) {
                if (c->is_accepted) {
                    send(c, data, len);
                }
            }
        }else{


        }

    } catch (exception &e) {

        LOGE("%s", e.what());
    }
}




void *eventloop(void *mgr) {

    for (;;) mg_mgr_poll((mg_mgr *) mgr, 1000);

}

char log_buf[256];
int log_index = 0;

void log_fn(char ch, void *param) {
    log_buf[log_index] = ch;
    log_index++;
    if (ch == '\0' || ch == '\n' || log_index == 255) {
        LOGD("%s", log_buf);
        log_index = 0;
        memset(log_buf, '\0', 256);
    }

}

static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_OPEN) {
        // c->is_hexdumping = 1;
    } else if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        if (mg_http_match_uri(hm, "/websocket")) {
            // Upgrade to websocket. From now on, a connection is a full-duplex
            // Websocket connection, which will receive MG_EV_WS_MSG events.
            mg_ws_upgrade(c, hm, NULL);
        }
    } else if (ev == MG_EV_WS_MSG) {
        // Got websocket frame. Received data is wm->data. Echo it back!
        struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
        string msg = string((const char *) wm->data.ptr, wm->data.len);
        LOGD("socket recv %s", msg.c_str());
        mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
    } else if (ev == MG_EV_WS_OPEN) {
        clients.emplace_back(c);
        LOGD("client conn size: %lu",clients.size());
        mg_ws_send(c, "conn", 4, WEBSOCKET_OP_TEXT);
    }
    (void) fn_data;
}

bool socketUtils::startServer() {

    mg_log_set(MG_LL_DEBUG);
    mg_log_set_fn(log_fn, nullptr);
    mg_mgr_init(&mgr);

    socketUtils::connection = mg_http_listen(&mgr, "ws://localhost:8765", fn,
                                             nullptr);  // Setup listener
    if (socketUtils::connection == nullptr) {
        LOGE("socket creat fail");
        return false;
    }
    pthread_t socket_t;
    int ret = pthread_create(&socket_t, nullptr, reinterpret_cast<void *(*)(void *)>(eventloop),
                             &mgr);
    LOGD("socket_t %d", ret);
    return true;
}

bool socketUtils::sendMsg(struct mg_connection *c, void *buff, size_t len) {
    //mg_send(c,buff,len);
    mg_ws_send(c, buff, len, WEBSOCKET_OP_TEXT);
}

bool socketUtils::sendMsg(struct mg_connection *c, const std::string data) {
    //mg_send(c,data.c_str(),data.length());
    mg_ws_send(c, data.c_str(), data.length(), WEBSOCKET_OP_TEXT);

}

void socketUtils::close() {
    mg_mgr_free(&mgr);
}
