#include<iostream>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<vector>
void encode_name(std::vector<unsigned char> &p, const std::string name){
	auto start = 0;
	size_t pos;	
	while ((pos = name.find('.', start)) != std::string::npos){
		auto len = pos - start;
		p.push_back(static_cast<unsigned char>(len));
		for(size_t i = start; i < pos; ++i ){
			p.push_back(name[i]);
		}
		start = pos + 1;	
	}	
		
	size_t len = name.size() - start;	
	p.push_back(static_cast<unsigned char>(len));
	for (size_t i = start; i < name.size(); i++){
		p.push_back(name[i]);
	 }
	p.push_back(0x00);
}	


auto make_query(){
	std::vector<unsigned char> packet;	
	packet.push_back(0x00); packet.push_back(0x00); // ID 
	packet.push_back(0x00); packet.push_back(0x00);; // Flags	
	packet.push_back(0x00); packet.push_back(0x01); // QD
	packet.push_back(0x00); packet.push_back(0x00); // AN
	packet.push_back(0x00); packet.push_back(0x00); // NS
	packet.push_back(0x00); packet.push_back(0x00); //AR
	
	encode_name(packet, "_filetransfer._tcp.local");
	packet.push_back(0x01);
	packet.push_back(0x01);
	return packet;
}

int main(){
	sockaddr_in group_sock;	
	struct ip_mreq group;	
	in_addr multicast_if;
	sockaddr_in bind_addr;
	
	int c_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (c_sock < 0)
		std::cout << "[ERROR] SOCKET IS BAD";
	else
		std::cout << "[WORKING:CLIENT] SOCKET IS GOOD..." << std::endl;
	
	group_sock.sin_family = AF_INET;
	group_sock.sin_port = htons(5353);
	group_sock.sin_addr.s_addr = inet_addr("224.0.0.251");
	
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(5353);
	bind_addr.sin_addr.s_addr = INADDR_ANY; 

	multicast_if.s_addr = INADDR_ANY;

	bind(c_sock, (struct sockaddr*)&bind_addr, sizeof(bind_addr));	
	
	group.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
	group.imr_interface.s_addr = INADDR_ANY;

	setsockopt(c_sock, IPPROTO_IP, IP_MULTICAST_IF,
		(char*)&multicast_if, sizeof(multicast_if));	
	
	setsockopt(c_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,(char*)&group, sizeof(group));
	auto packet = make_query();
	int sent = sendto(c_sock, packet.data(), packet.size(),0,
			(struct sockaddr*)&group_sock, sizeof(group_sock));	
		
	if (sent < 0)
		std::cout << "[ERROR] BAD SEND";
	else{
		std::cout << "[WORKING:CLIENT] SEND GOOD" << std::endl;
		std::cout << "[WORKING:CLIENT] Sent " << sent << " Bytes" << std::endl;	
	}	


}
