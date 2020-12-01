#pragma once

/*
Http服务
*/

/**
 * @brief http_server
 * @author Peng Chaowen
 * @web www.pcwen.top
 * @version 1.0
 * @date 2019/06/19
 */

#ifdef _WIN32
#include <winsock2.h>
#include <stdio.h>
#pragma comment(lib,"ws2_32.lib")
#endif

#include "mongoose.h"
#include "http_module_mgr.h"
#include "static_json.h"
#include "dynamic.h"

class http_server {
  public:
    bool start(const char *port);
  private:
    static void mgEvHandler(struct mg_connection *nc, int ev, void *p);
    static void mgSendBody(struct mg_connection *nc, const char *content); //发送body信息
    static void mgSendFile(struct mg_connection *nc, struct http_message *hm, const char* filePath);
    static struct mg_serve_http_opts s_http_server_opts;
};