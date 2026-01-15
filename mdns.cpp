#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<iostream>


int main(){
	int socket_test = socket(AF_INET, SOCK_DGRAM, 0 );
	sockaddr_in domain_sockets;
	domain_sockets.sin_family = AF_INET;
	domain_sockets.sin_port = htons(5555);
	domain_sockets.sin_addr = inet_addr("224.1.1.1");
	
	int test = bind(socket_test, domain_sockets.sin_addr
			, sizeof(domain_sockets.sin_addr));
	
}
