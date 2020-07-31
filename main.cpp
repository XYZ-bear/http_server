/**
 * @brief main
 * @author Peng Chaowen
 * @web www.pcwen.top
 * @version 1.0
 * @date 2019/06/19
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "HttpService.h"

Module(Login) {
	API(login, int) {
		con.send("login yes");
		cout << "yes" << endl;
	}
};

Module(Operation) {
	API(del, int) {
		cout << "del" << endl;
	}
};

//void Login::login(connection &c,int &a) {
//	cout << "yes" << endl;
//}
enum MyEnum
{
	cc,
	bb,
	kk
};
int main(int argc, char **argv) {
	MyEnum sf = MyEnum(4);
	//connection cc;
	//string l = "login";
	//module_mgr::modules_["Login"].deal(cc, l);
	//l = "del";
	//module_mgr::modules_["Operation"].deal(cc, l);
    HttpService *httpService = new HttpService();
    httpService->start("8000");

    return 0;
}