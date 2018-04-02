#pragma once

#include <string>

namespace client {
	int ipc_client(const std::string &path);
	int rpc_client(const std::string &addr, int port);

	void start_ipc_client(int fd);
	void start_rpc_client(int fd);
}