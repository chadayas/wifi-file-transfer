#ifndef GUARD_RECEIVER_H
#define GUARD_RECEIVER_H

#include "shared.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <filesystem>

#define RECV_PORT 9090

class ReceiverService {
public:
	ReceiverService(SharedState& s) : shared(s), running(false), socket_fd(-1) {}
	~ReceiverService();

	void start();
	void stop();

private:
	bool recv_exact(int fd, void* buf, size_t len);
	std::string save_dir();

	SharedState& shared;
	bool running;
	int socket_fd;
};

#endif
