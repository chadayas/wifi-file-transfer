#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<iostream>


int main(){
	
	sockaddr_in group_sock;
	sockaddr_in bind_addr;	
	in_addr mul_ips;
	ip_mreq group;
	
	int socket_test = socket(AF_INET, SOCK_DGRAM, 0);
	if( socket_test < 0){
		std::cout << "[ERROR] Socket connection BAD" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] SOCKET IS GOOD....." << std::endl;
	}
	group_sock.sin_family = AF_INET;
	group_sock.sin_port = htons(5555);
	group_sock.sin_addr.s_addr = inet_addr("225.1.1.1");
	
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(5555);
	bind_addr.sin_addr.s_addr = inet_addr("225.1.1.1");

	group.imr_multiaddr.s_addr =  inet_addr("225.1.1.1");
	group.imr_interface.s_addr = INADDR_ANY;
	
	char buffer[1024] = "Welcome";
	mul_ips.s_addr = INADDR_ANY; 
	int sock_op = setsockopt(socket_test, IPPROTO_IP, IP_MULTICAST_IF, 
	(struct in_addr*)&mul_ips, sizeof(mul_ips));
	
	setsockopt(socket_test, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group));
	bind(socket_test, (struct sockaddr*)&group, sizeof(group));

	socklen_t group_len = sizeof(group_sock);	
	int bytes = recvfrom(socket_test, buffer, sizeof(buffer), 0,
		(struct sockaddr*)&group_sock, &group_len);
		
	int send_fn = sendto(socket_test, buffer, sizeof(buffer),
		0, (struct sockaddr*)&group_sock, sizeof(group_sock));
	if (send_fn < 0){
		std::cout << "[ERROR] SENDING ERROR" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] MESSAGE SENT....." << std::endl;
	}
	
	if (bytes > 0){
		std::cout << "Yes this is working" << std::endl;
		std::cout << buffer << std::endl;
		running = false;
		}		
	else
		std::cout << "Bytes: " << bytes << std::endl;
	return 0;
}
