#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#include <reoul/logger.h>

#include "Server.h"

using namespace std;


int main()
{
	std::locale::global(std::locale("Korean"));
	LogInit();

	Server server;
	server.Start();

	return 0;
}
