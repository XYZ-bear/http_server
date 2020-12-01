#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "http_server.h"

int main(int argc, char **argv) {
    http_server *httpService = new http_server();
    httpService->start("8000");
	delete httpService;
    return 0;
}