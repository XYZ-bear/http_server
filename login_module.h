#pragma once

#include "HttpService.h"

Json(NTest)
{
public:
	double N(d);
};

Module(Login) {
	API(login, NTest) {
		con.send(p);
	}

	API(del, dynamic_json);
};