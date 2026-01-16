#include<iostream>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<netinet/in.h>
#include<netinet/ip.h>
int main(){
	sockaddr_in group_sock;	
	struct ip_mreq group;	
	char buffer[1024] = {0}; 
	int c_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (c_sock < 0)
		std::cout << "[ERROR] SOCKET IS BAD";
	else
		std::cout << "[WORKING:CLIENT] SOCKET IS GOOD..." << std::endl;
	
	group_sock.sin_family = AF_INET;	
	group_sock.sin_port = htons(5555);
	group_sock.sin_addr.s_addr = INADDR_ANY;

	int reuse = 1;	
	int sock_op = setsockopt(c_sock, SOL_SOCKET, SO_REUSEADDR, 
			(char *) &reuse, sizeof(reuse));
	if (sock_op < 0)
		std::cout << "[ERROR] SOCK OPTIONS BAD";
	else
		std::cout << "[WORKING:CLIENT] SOCK OPTIONS GOOD...."<<std::endl;
	int b_fn = bind(c_sock, (struct sockaddr*)&group_sock, sizeof(group_sock));	
	if (b_fn < 0)
		std::cout << "[ERROR] BINDING BAD";
	else
		std::cout << "[WORKING:CLIENT] BINDING GOOD...." << std::endl;
	
	group.imr_multiaddr.s_addr = inet_addr("225.1.1.1");
	group.imr_interface.s_addr = INADDR_ANY;


	int sock_op2 = setsockopt(c_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		(char*)&group, sizeof(group));
	if (sock_op2 < 0)
		std::cout << "[ERROR] SOCK OP 2 BAD";
	else
		std::cout << "[WORKING:CLIENT] SOCK OP 2 GOOD...." << std::endl;
	int recieve = recv(c_sock, buffer, sizeof(buffer), 0);
	if (recieve < 0)
		std::cout << "[ERROR] BAD READ";
	else
		std::cout << "[WORKING:CLIENT] READ GOOD....";
		std::cout << "MESSAGE FROM SERVER: " << buffer << std::endl;
}
