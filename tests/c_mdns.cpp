#include<iostream>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<vector>

int main(){
	sockaddr_in group_sock;	
	struct ip_mreq group;	
	in_addr multicast_if;
	sockaddr_in bind_addr;
	std::string msg = "WE ARE BACK";
	std::vector<char> buffer(1024);
	
	int c_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (c_sock < 0)
		std::cout << "[ERROR] SOCKET IS BAD";
	else
		std::cout << "[WORKING:CLIENT] SOCKET IS GOOD..." << std::endl;
	
	group_sock.sin_family = AF_INET;
	group_sock.sin_port = htons(4321);
	group_sock.sin_addr.s_addr = inet_addr("225.1.1.1");
	
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(4321);
	bind_addr.sin_addr.s_addr = INADDR_ANY; 

	multicast_if.s_addr = INADDR_ANY;

	bind(c_sock, (struct sockaddr*)&bind_addr, sizeof(bind_addr));	
	
	group.imr_multiaddr.s_addr = inet_addr("225.1.1.1");
	group.imr_interface.s_addr = INADDR_ANY;

	setsockopt(c_sock, IPPROTO_IP, IP_MULTICAST_IF,
		(char*)&multicast_if, sizeof(multicast_if));	
	
	setsockopt(c_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,(char*)&group, sizeof(group));

	int sent = sendto(c_sock, buffer.data(), buffer.size(),0,
			(struct sockaddr*)&group_sock, sizeof(group_sock));	
		
	if (sent < 0)
		std::cout << "[ERROR] BAD SEND";
	else{
		std::cout << "[WORKING:CLIENT] SEND GOOD" << std::endl;
		std::cout << "[WORKIN:CLIENT] Sent " << sent << " Bytes" << std::endl;	
	}	


}
