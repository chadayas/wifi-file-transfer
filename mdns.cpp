#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<iostream>
#include<sys/time.h>

int main(){
	
	sockaddr_in group_sock;
	sockaddr_in bind_addr;	
	in_addr mul_ips;
	ip_mreq group;
	timeval tv;	
	
	int socket_test = socket(AF_INET, SOCK_DGRAM, 0);
	if( socket_test < 0){
		std::cout << "[ERROR] Socket connection BAD" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] SOCKET IS GOOD....." << std::endl;
	}
	group_sock.sin_family = AF_INET;
	group_sock.sin_port = htons(4321);
	group_sock.sin_addr.s_addr = ("225.1.1.1");
	
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(4321);
	bind_addr.sin_addr.s_addr = INADDR_ANY;

	group.imr_multiaddr.s_addr =  inet_addr("225.1.1.1");
	group.imr_interface.s_addr = INADDR_ANY;
	
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	char buffer[1024] = {0};
	mul_ips.s_addr = INADDR_ANY; 
	
	int sock_op = setsockopt(socket_test, IPPROTO_IP, IP_MULTICAST_IF, 
	(struct in_addr*)&mul_ips, sizeof(mul_ips));
	if (sock_op <0){
		std::cout << "[ERROR:SERVER] Socket OPTION 1 BAD" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] SOCKET OPTION 1 GOOD....." << std::endl;
	}

		
	int sock_op2 = setsockopt(socket_test, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
			(char *)&group, sizeof(group));
	if (sock_op2 <0){
		std::cout << "[ERROR:SERVER] Socket OPTION 2 BAD" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] SOCKET OPTION 2 GOOD....." << std::endl;
	}
	
	int bindd = bind(socket_test, (struct sockaddr*)&bind_addr, sizeof(bind_addr));
	if (bindd < 0){
		std::cout << "[ERROR:SERVER] BIND BAD" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] BIND IS GOOD....." << std::endl;
	}
	
	setsockopt(socket_test, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	socklen_t group_len = sizeof(group_sock);	
	int bytes = recvfrom(socket_test, buffer, sizeof(buffer), 0,
		(struct sockaddr*)&group_sock, &group_len);
	if (bytes < 0)
		std::cout << "[ERROR:SERVER] NO DATA RECIEVED" << std::endl;
	else
		std::cout << "[WORKING:SERVER] GOOD RECIEVE" << std::endl;

	int send_fn = sendto(socket_test, buffer, sizeof(buffer),
		0, (struct sockaddr*)&group_sock, sizeof(group_sock));
	if (send_fn < 0){
		std::cout << "[ERROR] SENDING ERROR" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] MESSAGE SENT....." << std::endl;
	}
	
	/*if (bytes > 0){
		std::cout << "Yes this is working" << std::endl;
		std::cout << buffer << std::endl;
		}		
	else
		std::cout << "Bytes: " << bytes << std::endl;
	*/return 0;
}
