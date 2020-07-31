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
	void fun_name_##deal(connection &con){\
		instance;\
		param p;\
		fun_name(con,p);\
	}\
	collector collect{ this,#fun_name,&module_type::fun_name_##deal };\
public:\
	void fun_name(connection &con, param &p)

enum method_t
{
	GET,
	POST,
};

class connection {
public:
	mg_connection *connect;
	http_message *msg;
	method_t method;

	void send(const char* content) {
		if (connect) {
			mg_send_head(connect, 200, strlen(content), "Content-Type: text/plain\r\nConnection: close\r\nAccess-Control-Allow-Origin:*");
			mg_send(connect, content, strlen(content));
			connect->flags |= MG_F_SEND_AND_CLOSE;
		}
	}
};

class collector {
public:
	template<class O>
	collector(O *obj, const char* name,void(O::*api)(connection&)) {
		if (obj)
			obj->regist_api(name,api);
	}
};

struct module_api_bridge {
	void * obj = nullptr;
	void(*api)(connection &con, string &api_, void *obj_);
	void deal(connection &con, string &api_) {
		if (obj && api)
			api(con, api_, obj);
	}
};

class module_mgr {
public:
	static map<string, module_api_bridge> modules_;
};

template<class M,const char* N>
class module_base {
	typedef M module_type;
	typedef void(M::*api)(connection &con);
public:
	module_base() {
		module_mgr::modules_[name] = { this,module_api_deal };
	}
	void regist_api(const char* name_,api api_) {
		apis[name_] = api_;
	}
	void api_deal(string &api_, connection &con) {
		auto iter = apis.find(api_);
		if (iter != apis.end()) {
			M* real_obj = (M*)this;
			(real_obj->*(iter->second))(con);
		}
	}
	static void module_api_deal(connection &con, string &api_, void *obj_) {
		M* real_obj = (M*)obj_;
		real_obj->api_deal(api_, con);
	}
public:
	const char* name = N;
	static M instance;
private:
	map<string, api> apis;
};
template<class M,const char* N>
M module_base<M, N>::instance;
