#ifndef GUARD_SHARED_H
#define GUARD_SHARED_H

#include<mutex>
#include<map>
#include<string>

struct ServiceInfo {
	std::string ip;
	uint16_t port;
};

struct SharedState {
	std::mutex mtx;
	std::map<std::string,ServiceInfo> devices;
};

#endif
