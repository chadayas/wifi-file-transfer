#ifndef GUARD_SHARED_H
#define GUARD_SHARED_H

#include<mutex>
#include<map>
#include<string>

struct SharedState {
	std::mutex mtx;
	std::map<std::string,std::string> devices;
};

#endif
