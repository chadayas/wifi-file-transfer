#include "mdns.hpp"
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<iostream>
#include<sys/time.h>
#include<vector>
#include<map>


std::string decode_name(const std::vector<unsigned char>& pkt, size_t& pos) {
	std::string name;
	while (pkt[pos] != 0) {
		if ((pkt[pos] & 0xC0) == 0xC0) {
			size_t offset = ((pkt[pos] & 0x3F) << 8) | pkt[pos + 1];
			pos += 2;
			name += decode_name(pkt, offset);
			return name;
		}
		int len = pkt[pos++];
		for (int i = 0; i < len; i++) {
			name += pkt[pos++];
		}
		name += '.';
	}
	pos++;
	if (!name.empty()) name.pop_back();
	return name;
}

void parse_response(std::vector<unsigned char>& pkt, int bytes,
                    std::map<std::string,std::string>& devices) {
	if (bytes < 12) return;

	int ancount = (pkt[6] << 8) | pkt[7];
	size_t pos = 12;
	int qdcount = (pkt[4] << 8) | pkt[5];
	for (int i = 0; i < qdcount; i++) {
		while (pkt[pos] != 0) pos++;
		pos += 5;
	}

	for (int i = 0; i < ancount; i++) {
		std::string name = decode_name(pkt, pos);
		int type = (pkt[pos] << 8) | pkt[pos + 1];
		pos += 8;
		int rdlen = (pkt[pos] << 8) | pkt[pos + 1];
		pos += 2;
		size_t rdata_start = pos;

		if (type == 0x01 && rdlen == 4) { // A record
			std::string ip = std::to_string(pkt[pos]) + "." +
			                std::to_string(pkt[pos+1]) + "." +
			                std::to_string(pkt[pos+2]) + "." +
			                std::to_string(pkt[pos+3]);
			devices[ip] = name;
			std::cout << "Found A: " << name << " -> " << ip << std::endl;
		} else if (type == 0x0C) { // PTR record
			size_t rdata_pos = rdata_start;
			std::string instance = decode_name(pkt, rdata_pos);
			std::cout << "Found PTR: " << name << " -> " << instance << std::endl;
		}
		pos = rdata_start + rdlen;
	}
}

void encode_name(std::vector<unsigned char> &p, const std::string &name){
	size_t start = 0;
	size_t pos;	
	while ((pos = name.find('.', start)) != std::string::npos){
		size_t len = pos - start;
		p.push_back(static_cast<unsigned char>(len));
		for (size_t i = start; i < pos; ++i){
			p.push_back(name[i]);
		}
		start = pos + 1;	
	}
	size_t len = name.size() - start;
	p.push_back(static_cast<unsigned char>(len));
	for (size_t i = start; i < name.size(); ++i){
		p.push_back(name[i]);
	}
	p.push_back(0x00); // null term	
}

static auto make_a_record(){
	std::vector<unsigned char> packet;
	packet.push_back(0x00);packet.push_back(0x00);//ID
	packet.push_back(0x84);packet.push_back(0x00); // Flags
	packet.push_back(0x00);packet.push_back(0x00);// QDCOUNT
	packet.push_back(0x00);packet.push_back(0x01); // ANCOUNT
	packet.push_back(0x00);packet.push_back(0x00); // NSCOUNT
	packet.push_back(0x00);packet.push_back(0x00); // ARCOUNT

	encode_name(packet, LOCAL_NAME + SERVICE); // name
	packet.push_back(0x00);packet.push_back(0x01); // type A
	packet.push_back(0x80);packet.push_back(0x01); // class

	packet.push_back(0x00);packet.push_back(0x00); // TTL
	packet.push_back(0x00);packet.push_back(0x64); // TTL
	packet.push_back(0x00);packet.push_back(0x04); // RDlength (4 bytes for IP)

	packet.push_back(0xc0);packet.push_back(0xa8); // ip 192.168.
	packet.push_back(0x01);packet.push_back(0xaa); // 1.170

	return packet;
}

static auto make_ptr_record(){
	std::vector<unsigned char> packet;
	packet.push_back(0x00);packet.push_back(0x00);// ID
	packet.push_back(0x84);packet.push_back(0x00); // Flags (response)
	packet.push_back(0x00);packet.push_back(0x00);// QDCOUNT
	packet.push_back(0x00);packet.push_back(0x01); // ANCOUNT
	packet.push_back(0x00);packet.push_back(0x00); // NSCOUNT
	packet.push_back(0x00);packet.push_back(0x00); // ARCOUNT

	encode_name(packet, SERVICE); // _filetransfer._tcp.local
	packet.push_back(0x00);packet.push_back(0x0C); // type PTR
	packet.push_back(0x80);packet.push_back(0x01); // class

	packet.push_back(0x00);packet.push_back(0x00); // TTL
	packet.push_back(0x00);packet.push_back(0x64); // TTL

	// RDATA is the instance name
	std::vector<unsigned char> rdata;
	encode_name(rdata, LOCAL_NAME + SERVICE);

	packet.push_back(0x00);packet.push_back(static_cast<unsigned char>(rdata.size())); // RDlength
	for (auto b : rdata) packet.push_back(b);

	return packet;
}

static auto make_query(){
	std::vector<unsigned char> packet;	
	packet.push_back(0x00); packet.push_back(0x00); // ID 
	packet.push_back(0x00); packet.push_back(0x00);; // Flags	
	packet.push_back(0x00); packet.push_back(0x01); // QD
	packet.push_back(0x00); packet.push_back(0x00); // AN
	packet.push_back(0x00); packet.push_back(0x00); // NS
	packet.push_back(0x00); packet.push_back(0x00); //AR
	
	encode_name(packet, SERVICE);
	packet.push_back(0x00);packet.push_back(0x0c); //type
	packet.push_back(0x00);packet.push_back(0x01); // class
	return packet;
}

static sockaddr_in make_mdnsaddr(){
	sockaddr_in a{};
	a.sin_family = AF_INET;
	a.sin_port = htons(MDNS_PORT);
	a.sin_addr.s_addr = inet_addr(MDNS_IP);
	
	return a;
}

static sockaddr_in make_bindaddr(){
	sockaddr_in a{};
	a.sin_family = AF_INET;
	a.sin_port = htons(MDNS_PORT);
	a.sin_addr.s_addr = INADDR_ANY;
	
	return a;
}

static ip_mreq make_groups(){
	ip_mreq g{};
	g.imr_multiaddr.s_addr =  inet_addr(MDNS_IP);
	g.imr_interface.s_addr = INADDR_ANY;
	return g;
}

MDNSService::MDNSService(){
	socket_fd = -1;	
	running = false;
}

MDNSService::~MDNSService(){
	stop();
}
void MDNSService::start(){
	running = true;	
	std::vector<unsigned char> buffer(1024);
	
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	std::cout << "Socket result: " << socket_fd << std::endl; 	
	
	auto mdns_addr = make_mdnsaddr(); 
	auto bind_addr = make_bindaddr();
	auto groups = make_groups();
	
	int opt = setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
			(char *)&groups, sizeof(groups));
	
	std::cout << "Joining result: " << opt << std::endl; 	
	int reuse = 1;	
	int opt2 = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
	std::cout << "Reuse Port result: " << opt2 << std::endl;
	int opt3 = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	std::cout << "Reuse address result: " << opt3 << std::endl;
	int bi = bind(socket_fd, (struct sockaddr*)&bind_addr, sizeof(bind_addr));
	std::cout << "Bind result: " << bi << std::endl; 	
	
	socklen_t size = sizeof(mdns_addr);	
	socklen_t *size_ptr = &size;	
	
	while(running){	
	int bytes = recvfrom(socket_fd, buffer.data(), buffer.size(),
			0, (struct sockaddr*)&mdns_addr, size_ptr);
		if (bytes > 0){
			parse_response(buffer, bytes, devices);
		} else if(bytes == -1){
			std::cout << "." << std::flush;
			send_announcement();	
			send_query();
		}	
	}
}
void MDNSService::stop(){
	running = false;
	if (socket_fd >= 0) close(socket_fd);
}
void MDNSService::send_announcement(){
	auto mdns_addr = make_mdnsaddr(); 
	auto ptr_record = make_ptr_record();	
	auto a_record = make_a_record();	
	sendto(socket_fd, ptr_record.data(), ptr_record.size(), 0,
			(struct sockaddr*)&mdns_addr,sizeof(mdns_addr));
	sendto(socket_fd, a_record.data(), a_record.size(), 0,
			(struct sockaddr*)&mdns_addr,sizeof(mdns_addr));
}

void MDNSService::send_query(){
	auto query = make_query();	
	auto mdns_addr = make_mdnsaddr(); 
	sendto(socket_fd, query.data(), query.size(), 0,
		(struct sockaddr*)&mdns_addr,sizeof(mdns_addr));
}
int main(){
	
	sockaddr_in group_sock;
	sockaddr_in bind_addr;	
	in_addr mul_ips;
	ip_mreq group;
	timeval tv;	
	
	int socket_test = socket(AF_INET, SOCK_DGRAM, 0);
	if ( socket_test < 0){
		std::cout << "[ERROR] Socket connection BAD" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] SOCKET IS GOOD....." << std::endl;
	}
	
	group_sock.sin_family = AF_INET;
	group_sock.sin_port = htons(5353);
	group_sock.sin_addr.s_addr = inet_addr("224.0.0.251");
	
	socklen_t group_len = sizeof(group_sock);	
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(5353);
	bind_addr.sin_addr.s_addr = INADDR_ANY;

	group.imr_multiaddr.s_addr =  inet_addr("224.0.0.251");
	group.imr_interface.s_addr = INADDR_ANY;
	
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	std::vector<unsigned char> buffer(1024);
	
	int opt = setsockopt(socket_test, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
			(char *)&group, sizeof(group));
	std::cout << "Joining result: " << opt << std::endl; 	
	int reuse = 1;	
	int opt2 = setsockopt(socket_test, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
	std::cout << "Reuse Port result: " << opt2 << std::endl;
	
	int opt3 = setsockopt(socket_test, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	std::cout << "Reuse address result: " << opt3 << std::endl;
	
	int bi = bind(socket_test, (struct sockaddr*)&bind_addr, sizeof(bind_addr));
	std::cout << "Bind result: " << bi << std::endl; 	
	
	setsockopt(socket_test, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	socklen_t size = sizeof(group_sock);	
	socklen_t *size_ptr = &size;	
	
	bool running = true;
	std::map<std::string,std::string> devices;

	auto a_record = make_a_record();
	auto ptr_record = make_ptr_record();
	auto query = make_query();

	int send1 = sendto(socket_test, ptr_record.data(), ptr_record.size(), 0,
			(struct sockaddr*)&group_sock,sizeof(group_sock));
	std::cout << "[SERVER] Send PTR announcement: " << send1 << std::endl;

	int send2 = sendto(socket_test, a_record.data(), a_record.size(), 0,
			(struct sockaddr*)&group_sock,sizeof(group_sock));
	std::cout << "[SERVER] Send A announcement: " << send2 << std::endl;

	int send3 = sendto(socket_test, query.data(), query.size(), 0,
			(struct sockaddr*)&group_sock,sizeof(group_sock));
	std::cout << "[SERVER] Send query: " << send3 << std::endl;	

	
	while(running){	
	int bytes= recvfrom(socket_test, buffer.data(), buffer.size(),
		0, (struct sockaddr*)&group_sock, size_ptr);
		if (bytes > 0){
			parse_response(buffer, bytes, devices);
		} else if(bytes == -1){
			std::cout << "." << std::flush;
			sendto(socket_test, ptr_record.data(), ptr_record.size(), 0,
			(struct sockaddr*)&group_sock,sizeof(group_sock));
			sendto(socket_test, a_record.data(), a_record.size(), 0,
			(struct sockaddr*)&group_sock,sizeof(group_sock));
			sendto(socket_test, query.data(), query.size(), 0,
			(struct sockaddr*)&group_sock,sizeof(group_sock));
		} 
			
	}
	return 0;
}
