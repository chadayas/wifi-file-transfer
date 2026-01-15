#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<iostream>


int main(){
	int socket_test = socket(AF_INET, SOCK_DGRAM, 0 );
	sockaddr_in group_sock;
	group_sock.sin_family = AF_INET;
	group_sock.sin_port = htons(5555);
	group_sock.sin_addr = inet_addr("225.1.1.1");
	
	std::cout << group_sock->sin_addr << std::endl;	
}
