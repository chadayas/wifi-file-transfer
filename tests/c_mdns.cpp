#include<iostream>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<netinet/in.h>
#include<netinet/ip.h>

int main(){
	sockaddr_in group_sock;	
	struct ip_mreq group;	
	in_addr multicast_if;
	sockaddr_in bind_addr;

	char message[1024] = "Where is the photo transfer"; 
	char buffer[1024] = {0}; 
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

	int reuse = 1;	
	/*int sock_op = setsockopt(c_sock, SOL_SOCKET, SO_REUSEADDR, 
			(char *) &reuse, sizeof(reuse));
	if (sock_op < 0)
		std::cout << "[ERROR:CLIENT] SOCK OPTIONS 1 BAD" << std::endl;
	else
		std::cout << "[WORKING:CLIENT] SOCK OPTIONS 1 GOOD" << std::endl;
	*/	
	int b_fn = bind(c_sock, (struct sockaddr*)&bind_addr, sizeof(bind_addr));	
	
	group.imr_multiaddr.s_addr = inet_addr("225.1.1.1");
	group.imr_interface.s_addr = INADDR_ANY;

	int sock_opp = setsockopt(c_sock, IPPROTO_IP, IP_MULTICAST_IF,
			(char*)&multicast_if, sizeof(multicast_if));	
	
	int sock_op2 = setsockopt(c_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		(char*)&group, sizeof(group));
	if (sock_op2 < 0)
		std::cout << "[ERROR:CLIENT] SOCK OPTIONS 2 BAD" << std::endl;
	else
		std::cout << "[WORKING:CLIENT] SOCK OPTIONS 2 GOOD" << std::endl;
	

	int sent = sendto(c_sock, message, sizeof(message),0,
			(struct sockaddr*)&group_sock, sizeof(group_sock));	
		
	if (sent < 0)
		std::cout << "[ERROR] BAD SEND";
	else{
		std::cout << "[WORKING:CLIENT] SEND GOOD" << std::endl;
		std::cout << "[WORKIN:CLIENT] Sent " << sent << " Bytes" << std::endl;	
	}	

	int recieve = recv(c_sock, buffer, sizeof(buffer), 0);
	std::cout << buffer << std::endl;

}
