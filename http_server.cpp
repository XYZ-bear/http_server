#include "http_server.h"

struct mg_serve_http_opts http_server::s_http_server_opts;

//�����¼�����
void http_server::mgEvHandler(struct mg_connection *nc, int ev, void *p) {
    //����request
    if (ev == MG_EV_HTTP_REQUEST) {
		if (struct http_message *msg = (struct http_message *)p) {
			connection con;
			con.connect = nc;
			con.msg = msg;
			string uri(msg->uri.p, msg->uri.len);
			bool res = false;
			auto iter = module_api_mgr::modules_.find(uri);
			if (iter != module_api_mgr::modules_.end()) {
				res = iter->second.deal(con);
			}
			if (!res) {
				con.send("error api");
			}

			////body����
			//char* body = new char[msg->body.len + 1];
			//memset(body, 0, msg->body.len + 1);
			//memcpy(body, msg->body.p, msg->body.len);

			////����body��Ϣ
			//mgSendBody(nc, "body content");

			////���������ļ�
			////mgSendFile("�����s_http_server_opts.document_root���ļ�·��");

			//delete body;
		}
    }
}

//����body��Ϣ
void http_server::mgSendBody(struct mg_connection *nc, const char *content) {
    mg_send_head(nc, 200, strlen(content), "Content-Type: text/plain\r\nConnection: close");
    mg_send(nc, content, strlen(content));
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

//�����ļ����ļ���λ���������s_http_server_opts.document_root��·��
void http_server::mgSendFile(struct mg_connection *nc, struct http_message *hm, const char* filePath) {
    mg_http_serve_file(nc, hm, filePath, mg_mk_str("text/plain"), mg_mk_str(""));
}

//��ʼ��������
bool http_server::start(const char *port) {
    struct mg_mgr mgr;
    struct mg_connection *nc;

    mg_mgr_init(&mgr, NULL);
    printf("Starting web server on port %s\n", port);
    nc = mg_bind(&mgr, port, mgEvHandler);
    if (nc == NULL) {
        printf("Failed to create listener\n");
        return false;
    }

    // Set up HTTP server parameters
    mg_set_protocol_http_websocket(nc);
    s_http_server_opts.document_root = ".";  //�ļ����·�� Serve current directory
    s_http_server_opts.enable_directory_listing = "yes";

    for (;;) {
        mg_mgr_poll(&mgr, 500); //1s��ѵһ��
    }
    mg_mgr_free(&mgr);

    return true;
}
