#ifndef GUARD_MDNS_H

#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<vector>
#include<iostream>
#include<sys/time.h>
#include<map>

#define MDNS_IP "224.0.0.251"
#define MDNS_PORT 5353
#define SERVICE std::string("_filetransfer._tcp.local")
#define DNS std::string("_services._dns-sd._udp.local")
#define LOCAL_NAME std::string("MyLaptop.")



typedef std::vector<unsigned char> vec_uc;

std::string decode_name(const vec_uc& pkt, size_t& pos);
void parse_response(std::vector<unsigned char>& pkt, int bytes,
		std::map<std::string,std::string>& devices);

void encode_name(vec_uc& p, const std::string &name);

sockaddr_in make_mdnsaddr();
sockaddr_in make_bindaddr();
ip_mreq make_groups();

class MDNSSerivce{
	public:
		MDNSService();
		~MDNSService();
		
		void start();
		void stop();
		std::map<std::string, std::string> get_devices();	
	private:	
		bool running;
		int socket_fd;
		std::map<std::string, std::string> devices;	
	private:
		void send_query();
		void send_announcement();
		void parse_response(vec_uc &pkt,int bytes);
};



#endif
