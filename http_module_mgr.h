#pragma once
#include <map>
#include <functional>
#include <string>
#include "mongoose.h"
using namespace std;

#define Module(class_name) \
extern const char class_name##name[] = #class_name; \
class class_name :public module_base<class_name,class_name##name>

#define API(fun_name,param) \
private:\
	static bool fun_name##_deal(connection &con,void *obj){\
		instance;\
		param p;\
		p.unserialize(con.msg->body.p,con.msg->body.len);\
		module_type *m = (module_type*)obj;\
		m->fun_name(con,p);\
		return true;\
	}\
	collector collect_##fun_name{ this,name,#fun_name,&module_type::fun_name##_deal };\
public:\
	void fun_name(connection &con, param &p)

enum method_t
{
	NONE,
	GET,
	POST,
};

class connection {
public:
	mg_connection *connect;
	http_message *msg;
	method_t method;

	method_t get_method() {
		if (method != method_t::NONE)
			return method;
		switch (*msg->message.p)
		{
		case 'P':
		case 'p': {
			method = method_t::POST;
			break;
		}
		case 'G':
		case 'g': {
			method = method_t::GET;
			break;
		}
		default:
			break;
		}
		return method;
	}



	void send(const char* content) {
		if (connect) {
			//mg_send_head(connect, 200, strlen(content), "Content-Type: text/plain\r\nConnection: close\r\nAccess-Control-Allow-Origin:*");
			//mg_send(connect, content, strlen(content));
			//connect->flags |= MG_F_SEND_AND_CLOSE;

			mg_printf(connect, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
			// 以json形式返回
			mg_printf_http_chunk(connect, content);
			// 发送空白字符快，结束当前响应
			mg_send_http_chunk(connect, "", 0);
		}
	}

	template<class JSON>
	void send(JSON& json) {
		std::string str;
		json.serialize(str);
		//mg_send_head(connect, 200, str.size(), "Content-Type: application/json\r\nConnection: close\r\nAccess-Control-Allow-Origin:*");
		//mg_send(connect, str.c_str(), str.size());
		//connect->flags |= MG_F_SEND_AND_CLOSE;

		mg_printf(connect, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
		// 以json形式返回
		mg_printf_http_chunk(connect, str.c_str());
		// 发送空白字符快，结束当前响应
		mg_send_http_chunk(connect, "", 0);
	}

};

struct module_api_bridge {
	void * obj = nullptr;
	bool(*api)(connection &con, void *obj_);
	bool deal(connection &con) {
		return api(con, obj);
	}
};

class module_api_mgr {
public:
	static map<string, module_api_bridge> modules_;
};

class collector {
public:
	collector(void *obj, const char* module_name, const char* name, bool(*api)(connection&, void*)) {
		string rest("/");
		rest += module_name;
		rest += "/";
		rest += name;
		module_api_mgr::modules_[rest] = { this,api };
	}
};

template<class M,const char* N>
class module_base {
public:
	typedef M module_type;
	typedef void(M::*api)(connection &con);

	module_base() {
	}
public:
	const char* name = N;
	static M instance;
};
template<class M,const char* N>
M module_base<M, N>::instance;
