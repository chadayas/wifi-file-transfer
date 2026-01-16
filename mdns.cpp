#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<iostream>


int main(){
	
	sockaddr_in group_sock;
	in_addr mul_ips;
	int socket_test = socket(AF_INET, SOCK_DGRAM, 0);
	if( socket_test < 0){
		std::cout << "[ERROR] Socket connection BAD" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] SOCKET IS GOOD....." << std::endl;
	}
	group_sock.sin_family = AF_INET;
	group_sock.sin_port = htons(5555);
	group_sock.sin_addr.s_addr = inet_addr("225.1.1.1");
	
	char buffer[1024] = "WElcome";
	mul_ips.s_addr = INADDR_ANY; 
	int sock_op = setsockopt(socket_test, IPPROTO_IP, IP_MULTICAST_IF, 
	(struct in_addr*)&mul_ips, sizeof(mul_ips));
	if (sock_op < 0){
		std::cout << "[ERROR] Socket OPTIONS BAD" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] SOCKET OPTIONS GOOD....." << std::endl;
			}
	int send_fn = sendto(socket_test, buffer, sizeof(buffer),
		0, (struct sockaddr*)&group_sock, sizeof(group_sock));
	
	if (send_fn < 0){
		std::cout << "[ERROR] SENDING ERROR" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] MESSAGE SENT....." << std::endl;
			}
		

}
