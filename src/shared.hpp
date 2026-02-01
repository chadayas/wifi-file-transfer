#ifndef GUARD_SHARED_H
#define GUARD_SHARED_H

#include<mutex>
#include<map>
#include<string>

#define RECV_PORT 9090

enum class DiscoverySource { MDNS, ARP };

struct ServiceInfo {
	std::string ip;
	std::string target_host;
	uint16_t port = RECV_PORT;
	DiscoverySource source = DiscoverySource::MDNS;
};

struct SharedState {
	std::mutex mtx;
	std::map<std::string,ServiceInfo> devices;
};

#endif
