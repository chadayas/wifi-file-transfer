#ifndef GUARD_MDNS_H
#define GUARD_MDNS_H

#include "shared.hpp"
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<vector>
#include<iostream>
#include<sys/time.h>
#include<map>
#include<unistd.h>
#include<thread>
#include<mutex>
#include<ifaddrs.h>

#define MDNS_IP "224.0.0.251"
#define MDNS_PORT 5353
#define SERVICE std::string("_filetransfer._tcp.local")
#define DNS std::string("_services._dns-sd._udp.local")
#define LOCAL_NAME std::string("MyLaptop.")



typedef std::vector<unsigned char> vec_uc;

std::string decode_name(const vec_uc& pkt, size_t& pos);
void parse_response(std::vector<unsigned char>& pkt, int bytes,
		std::map<std::string,ServiceInfo>& devices);
std::string get_ip();
void encode_name(vec_uc& p, const std::string &name);


class MDNSService{
	public:
		MDNSService(SharedState& s) : shared(s) {}
		~MDNSService();
			
		void start();
		void stop();
		auto get_devices(SharedState &s);	
	private:	
		bool running;
		int socket_fd;
	private:
		void send_query();
		void send_announcement();
		SharedState& shared;
};



#endif
