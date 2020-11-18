#include "login_module.h"

void Login::del(connection &con, dynamic_json &p) {
	//cout << (int)p["d"] << endl;
	con.send(p);
}